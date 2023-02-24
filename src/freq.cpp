#include "freq.h"

#include <fstream>
#include <sys/stat.h>
#include <filesystem>
#include <thread>
#include <ranges>
#include <algorithm>
#include "threadpool.h"

struct FreqConfig {
  FreqConfig(const FreqConfig &root) = delete;
  FreqConfig &operator=(const FreqConfig &) = delete;

  static FreqConfig &instance() {
      static FreqConfig fc;
      return fc;
  }

  [[nodiscard]] size_t get_processor_count() const {
      return processor_count;
//      return 1;
  }

  [[nodiscard]] size_t get_disk_page_size() const {
      return disk_page_size;
  }

 private:
  FreqConfig() {
      struct stat fi{};
      stat("/", &fi);

      processor_count = std::thread::hardware_concurrency();
#ifndef    NO_OLDNAMES
      disk_page_size = fi.st_size;
#elif
      disk_page_size = fi.st_blksize;
#endif
  };

  size_t processor_count;
  size_t disk_page_size;
};

void validate(const std::string_view &x) {
    struct non_alpha {
      bool operator()(char c) {
          return !std::iswalpha(c) && c != '\'' && c != '`';
      }
    };

    bool contains_non_alpha = std::find_if(x.begin(), x.end(), non_alpha()) != x.end();

    if (x[0] == '\0' || x.empty() || contains_non_alpha) {
        throw std::logic_error("invalid string");
    }
}

FreqMap process_file(const std::string &filename) {
    const auto &config = FreqConfig::instance();
    const size_t file_size = std::filesystem::file_size(filename);
    const size_t chunk_size = std::max(config.get_disk_page_size(), file_size / (config.get_processor_count() * 3));
    const size_t chunks = (file_size + chunk_size - 1) / chunk_size;

    std::vector<char> data(file_size + 1);
    data[file_size] = '\0';

    std::vector<std::pair<const char *, const char *>> chunk_edges(chunks);

    static auto is_delim = [](char c) {
      return !std::iswalpha(c) && !std::isdigit(c) && c != '\'';
    };

    struct PerThreadData {
      std::ifstream file;
      FreqMap frequency;
    };

    std::vector<PerThreadData> per_thread(config.get_processor_count());
    for (auto &tld : per_thread) {
        tld.file.open(filename);
        tld.frequency.reserve(chunk_size / 5);
    }

    {
        auto thread_pool = ThreadPool(config.get_processor_count());

        for (size_t i = 0; i < chunks; i++) {
            thread_pool.enqueue([&, i, file_size](size_t thread_index) {
              size_t start_pos = i * chunk_size;
              size_t end_pos = (i == chunks - 1)
                               ? file_size
                               : start_pos + chunk_size;
              size_t size = end_pos - start_pos;

              auto &tld = per_thread[thread_index];

              tld.file.seekg(static_cast<int>(start_pos));
              tld.file.read(data.data() + start_pos, static_cast<int>(size));

              // Handle chunk
              auto from = data.begin() + static_cast<std::ptrdiff_t>(start_pos);
              auto to = data.begin() + static_cast<std::ptrdiff_t>(end_pos);
              auto from_rev = std::reverse_iterator(to);
              auto to_rev = std::reverse_iterator(from);

              // Looking for the first delimiter
              auto start_it = std::find_if(from, to, is_delim);

              // Looking for the last delimiter
              auto end_it_rev = std::find_if(from_rev, to_rev, is_delim);

              auto end_it = end_it_rev.base();

              // It is possible that the word would occupy a whole chunk.
              // nullptr means no delimiter found in chunk
              char *start = nullptr;
              char *end = nullptr;

              if (start_it != to)
                  start = start_it.base();
              if (end_it_rev != to_rev)
                  end = end_it.base();
              chunk_edges[start_pos / chunk_size] = {start, end};

              start_it = std::find_if_not(start_it, end_it, is_delim);
              while (start_it < end_it) {
                  auto word_end = std::find_if(start_it, end_it, is_delim);
                  const auto &x = std::string_view(start_it.base(), word_end - start_it);
                  validate(x);
                  const auto &[it, emplaced] = tld.frequency.try_emplace(x, 1);
                  if (!emplaced) {
                      ++it->second;
                  }
                  start_it = std::find_if_not(word_end, end_it, is_delim);
              }
            });
        }
    }

    // Firstly used for counting words at the chunk edges,
    // then accumulating all counters to that map
    FreqMap result;
    result.reserve(data.size() / 10);

    // Chunk edges handling
    auto &[fs, fe] = chunk_edges.front();
    bool file_is_one_word = true;

    const char *left = chunk_edges.begin()->first;
    const char *right = nullptr;

    if (left == nullptr) {
        left = data.data();
    }

    // Process words have been split by chunks edges
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

        if (left != nullptr && right != nullptr) {
            file_is_one_word = false;
            const auto &x = std::string_view(left, right - left);
            validate(x);
            const auto &[it, emplaced] = result.try_emplace(x, 1);
            if (!emplaced) {
                ++it->second;
            }
            left = right = nullptr;
        }
    }

    // Process the case of a huge single word
    if (file_is_one_word) {
        auto end_pos = std::find_if_not(data.rbegin(), data.rend(), is_delim).base().base();
        const auto &x = std::string_view(data.data(), end_pos - data.data());
        validate(x);
        const auto &[it, emplaced] = result.try_emplace(x, 1);
        if (!emplaced) {
            ++it->second;
        }
    } else {
        // Left edge of the first chunk
        if (fs != nullptr) {
            const auto &x = std::string_view(data.data(), fs);
            validate(x);
            const auto &[it, emplaced] = result.try_emplace(x, 1);
            if (!emplaced) {
                ++it->second;
            }
        }

        // Right edge of the last chunk
        auto pos = chunk_edges.back().second;
        if (pos != nullptr && pos != &data.back()) {
            const auto &x = std::string_view(pos, data.end().base() - pos);
            validate(x);
            const auto &[it, emplaced] = result.try_emplace(x, 1);
            if (!emplaced) {
                ++it->second;
            }
        }
    }

    for (auto &tld : per_thread) {
        for (auto &[key, value] : tld.frequency) {
            result[key] += value;
        }
    }

    return result;
}
