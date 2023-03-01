#include <iostream>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <algorithm>
#include <sys/mman.h>
#include <sys/param.h>
#include <fcntl.h>
#include <span>
#include <aio.h>
#include <libaio.h>
#include <unistd.h>
#include <cstdio>

#include "freq.h"
#include "threadpool.h"
#include "utils.h"

static void count_word(FreqMap &freq, const char *begin, const char *end) {
    const auto &x = std::string_view(begin, end - begin);
    const auto &[it, emplaced] = freq.try_emplace(x, 1);
    if (!emplaced) {
        ++it->second;
    }
}

static FreqMap process_edges(const std::span<char> &data,
                             const std::vector<std::pair<const char *, const char *>> &chunk_edges) {
    FreqMap result;
    result.reserve(data.size() / 10);

    const auto &[first_word_start, first_chunk_last_delim] = chunk_edges.front();
    // Single chunk case is processed in else branch of var condition.
    bool file_is_one_word = chunk_edges.size() > 1;

    const char *left = first_word_start;
    const char *right = nullptr;

    if (left == nullptr) {
        // There were no delimiters in first chunk,
        // hence whole chunk was a word, and it starts
        // in the beginning of data vector
        left = data.data();
    }

    // Processing of words that have been split by chunk edges
    for (auto chunk_it = chunk_edges.begin(); chunk_it != chunk_edges.end() - 1; ++chunk_it) {
        auto &[curr_left, curr_right] = *chunk_it;
        auto &[next_left, next_right] = *(chunk_it + 1);

        // If current chunk is a whole word, then preserve start of the word
        if (curr_right != nullptr) {
            left = curr_right;
        }

        // If there was no end of the word, then set first correct end
        if (right == nullptr && next_left != nullptr && left < next_left) {
            right = next_left;
        }

        // Count word when found both start and end of the word
        if (left != nullptr && right != nullptr) {
            file_is_one_word = false;
            count_word(result, left, right);
            left = right = nullptr;
        }
    }

    if (file_is_one_word) {
        // Processing the case of a huge single word.
        const char *start_pos;
        if (first_word_start != nullptr) {
            start_pos = first_word_start + 1;
        } else {
            start_pos = std::find_if_not(data.begin(), data.end(), is_delim).base();
        }
        auto end_pos = std::find_if_not(data.rbegin(), data.rend(), is_delim).base().base();
        count_word(result, start_pos, end_pos);
    } else {
        // Left edge of the first chunk
        // first_word_start is null in case there were no delimiters in the chunk.
        // first_word_start == data.data() in case file is starting with delimiter (no word in the beginning).
        if (first_word_start != nullptr && first_word_start > data.data()) {
            auto start_pos = std::find_if_not(data.begin(), data.end(), is_delim).base();
            count_word(result, start_pos, first_word_start);
        }

        auto start_pos = chunk_edges.back().second;
        // Right edge of the last chunk
        // start_pos is null in case of there were no delimiters in chunk.
        if (start_pos != nullptr && start_pos < &data.back()) {
            count_word(result, start_pos, data.end().base());
        } else if (start_pos == nullptr && left != nullptr && right == nullptr) {
            count_word(result, left, data.end().base());
        }
    }

    return result;
}

static void process_chunk(const std::span<char> &data,
                          FreqMap &freq_per_thread,
                          std::vector<std::pair<const char *, const char *>> &chunk_edges,
                          size_t start_pos, size_t end_pos, size_t chunk_size) {
    auto from = data.begin() + static_cast<std::ptrdiff_t>(start_pos);
    auto to = data.begin() + static_cast<std::ptrdiff_t>(end_pos);
    auto from_rev = std::reverse_iterator(to);
    auto to_rev = std::reverse_iterator(from);

    // Looking for the first delimiter to cut off a word in the beginning.
    auto start_it = std::find_if(from, to, is_delim);

    // Looking for the last delimiter to cut off a word in the ending.
    auto end_it_rev = std::find_if(from_rev, to_rev, is_delim);
    auto end_it = end_it_rev.base();

    // In this case nullptr means no delimiter was found in the chunk
    // (the whole chunk is part of the word).
    char *start = nullptr;
    char *end = nullptr;

    if (start_it != to) {
        start = start_it.base();
        if (end_it_rev != to_rev) {
            end = end_it.base();
        }

        chunk_edges[start_pos / chunk_size] = {start, end};

        start_it = std::find_if_not(start_it, end_it, is_delim);
        while (start_it < end_it) {
            auto word_end = std::find_if(start_it, end_it, is_delim);
            count_word(freq_per_thread, start_it.base(), word_end.base());
            start_it = std::find_if_not(word_end, end_it, is_delim);
        }
    }
}

FreqMap process_file_blocking_read(const std::string &filename) {
    const auto &config = FreqConfig::instance();
    const size_t file_size = std::filesystem::file_size(filename);

    const size_t chunk_size = std::max(
        config.get_disk_page_size(),
        file_size / (config.get_processor_count() * 2)
    ) / config.get_disk_page_size() * config.get_disk_page_size();

    const size_t chunks = (file_size + chunk_size - 1) / chunk_size;

    std::vector<char> data(file_size);

    // Words can lie on the boundaries of chunks, so it is
    // necessary to memorize parts of words on the boundaries.
    // chunk_edges stores the first and last delimiter position in each chunk.
    std::vector<std::pair<const char *, const char *>> chunk_edges(chunks);

    struct PerThreadData {
      std::ifstream file;
      FreqMap frequency;
    };

    std::vector<PerThreadData> per_thread(config.get_processor_count());
    for (auto &tld : per_thread) {
        // To prevent sharing of position state, it is necessary
        // to allocate a unique file descriptor (fd) per thread.
        tld.file.open(filename, std::ifstream::binary);
        tld.frequency.reserve(chunk_size / 5);
    }

    {
        auto thread_pool = ThreadPool(config.get_processor_count());

        for (size_t i = 0; i < chunks; i++) {
            thread_pool.enqueue([&, i, file_size](const size_t thread_index) {
              const size_t start_pos = i * chunk_size;
              const size_t end_pos = (i == chunks - 1)
                               ? file_size
                               : start_pos + chunk_size;
              const size_t size = end_pos - start_pos;

              auto &tld = per_thread[thread_index];

              tld.file.seekg(static_cast<int>(start_pos));
              tld.file.read(data.data() + start_pos, static_cast<int>(size));

              process_chunk(data, tld.frequency, chunk_edges, start_pos, end_pos, chunk_size);
            });
        }
    }

    FreqMap result = process_edges(std::span(data), chunk_edges);

    for (auto &tld : per_thread) {
        for (auto &[key, value] : tld.frequency) {
            result[key] += value;
        }
    }

    return result;
}

FreqMap process_mmaped_file(const std::string &filename) {
    const auto &config = FreqConfig::instance();
    const size_t file_size = std::filesystem::file_size(filename);

    int fd = open(filename.c_str(), O_RDONLY | O_DIRECT | O_NOATIME);
    char *mmaped = static_cast<char *>(
        mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0)
    );

    if (mmaped == MAP_FAILED) {
        std::cerr << "mmap failed: " << std::strerror(errno) << std::endl;
        close(fd);
        std::exit(EXIT_FAILURE);
    }

    const std::span<char> data(mmaped, file_size);

    const size_t chunk_size = std::max(
        config.get_disk_page_size(),
        file_size / (config.get_processor_count() * 2)
    ) / config.get_disk_page_size() * config.get_disk_page_size();

    const size_t chunks = (file_size + chunk_size - 1) / chunk_size;

    // Words can lie on the boundaries of chunks, so it is
    // necessary to memorize parts of words on the boundaries.
    // chunk_edges stores the first and last delimiter position in each chunk.
    std::vector<std::pair<const char *, const char *>> chunk_edges(chunks);

    struct PerThreadData {
      FreqMap frequency;
    };

    std::vector<PerThreadData> per_thread(config.get_processor_count());
    for (auto &tld : per_thread) {
        tld.frequency.reserve(chunk_size / 5);
    }

    {
        auto thread_pool = ThreadPool(config.get_processor_count());

        for (size_t i = 0; i < chunks; i++) {
            thread_pool.enqueue([&, i, file_size](const size_t thread_index) {
              const size_t start_pos = i * chunk_size;
              const size_t end_pos = (i == chunks - 1)
                                     ? file_size
                                     : start_pos + chunk_size;

              auto &tld = per_thread[thread_index];
              process_chunk(data, tld.frequency, chunk_edges, start_pos, end_pos, chunk_size);
            });
        }
    }

    FreqMap result = process_edges(data, chunk_edges);

    for (auto &tld : per_thread) {
        for (auto &[key, value] : tld.frequency) {
            result[key] += value;
        }
    }

    munmap(mmaped, file_size);
    close(fd);

    return result;
}

static void io_error(const char *func, int rc) {
    if (rc == -ENOSYS) {
        std::cerr << "AIO not in this kernel\n";
    } else {
        std::cerr << func << ": error " << rc << '\n';
    }
    std::exit(EXIT_FAILURE);
}

static void validate_event(const io_event &event) {
    if (event.res2 != 0) {
        io_error("aio read", static_cast<int>(event.res2));
    }
    if (event.res != event.obj->u.c.nbytes) {
        std::stringstream err;
        err << "read missing bytes expect " << event.obj->u.c.nbytes << " got " << event.res;
        throw std::runtime_error(err.str());
    }
}

FreqMap process_file_aio(const std::string &filename) {
    const auto &config = FreqConfig::instance();
    const size_t file_size = std::filesystem::file_size(filename);

    const size_t chunk_size = std::max(
        config.get_disk_page_size(),
        file_size / (config.get_processor_count() * 2)
    ) / config.get_disk_page_size() * config.get_disk_page_size();

    const size_t AIO_BLKSIZE = chunk_size;
    constexpr size_t AIO_MAXIO = 512;

    std::vector<char> data(file_size);

    struct PerThreadData {
      FreqMap frequency;
    };

    std::vector<PerThreadData> per_thread(config.get_processor_count());
    for (auto &tld : per_thread) {
        tld.frequency.reserve(file_size / 5);
    }

    size_t length, offset = 0;

    int fd;
    if ((fd = open(filename.c_str(), O_RDONLY | O_DIRECT | O_NOATIME)) < 0) {
        perror(filename.c_str());
        std::exit(EXIT_FAILURE);
    }
    length = file_size;

    io_context_t ctx{};
    io_queue_init(AIO_MAXIO, &ctx);
    size_t busy = 0;
    size_t chunks = howmany(length, AIO_BLKSIZE);

    std::vector<std::pair<const char *, const char *>> chunk_edges(chunks);

    {
        iocb iocbs[AIO_MAXIO];
        io_event events[AIO_MAXIO];

        iocb *iocbs_ptrs[AIO_MAXIO];
        for (size_t i = 0; i < AIO_MAXIO; ++i) {
            iocbs_ptrs[i] = &iocbs[i];
        }

        auto thread_pool = ThreadPool(config.get_processor_count());
        while (chunks > 0) {
            int ret;
            int n = MIN(MIN(AIO_MAXIO - busy, AIO_MAXIO / 16),
            howmany(length - offset, AIO_BLKSIZE));
            if (n > 0) {
                for (size_t i = 0; i < n; i++) {
                    size_t size = std::min(length - offset, AIO_BLKSIZE);
                    char *buf = data.data() + offset;
                    io_prep_pread(&iocbs[i], fd, buf, size, static_cast<off_t>(offset));
                    offset += size;
                }
                ret = io_submit(ctx, n, iocbs_ptrs);
                if (ret < 0) {
                    io_error("io_submit", ret);
                }
                busy += n;
            }

            static timespec timeout = {0, 0};
            while ((ret = io_getevents(ctx, 1, n, events, &timeout)) > 0) {
                for (int i = 0; i < ret; ++i) {
                    const io_event &event = events[i];
                    validate_event(event);

                    iocb *iocb = event.obj;
                    auto start_pos = iocb->u.c.offset;
                    auto size = iocb->u.c.nbytes;

                    --busy;
                    --chunks;
                    thread_pool.enqueue([&per_thread, &data, &chunk_edges, start_pos, size, AIO_BLKSIZE](const size_t thread_index) {
                      auto end_pos = start_pos + size;
                      auto &tld = per_thread[thread_index];
                      process_chunk(data, tld.frequency, chunk_edges, start_pos, end_pos, AIO_BLKSIZE);
                    });
                }
            }

            if (ret < 0) {
                io_error("io_getevents", ret);
            }
        }
        close(fd);
    }

    FreqMap result = process_edges(data, chunk_edges);

    for (auto &tld : per_thread) {
        for (auto &[key, value] : tld.frequency) {
            result[key] += value;
        }
    }

    return result;
}
