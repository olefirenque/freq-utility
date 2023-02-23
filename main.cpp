#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <filesystem>
#include <thread>
#include <ranges>
#include <algorithm>
#include "threadpool.h"
#include "unordered_dense.h"

struct FreqConfig {
  FreqConfig(const FreqConfig &root) = delete;
  FreqConfig &operator=(const FreqConfig &) = delete;

  static FreqConfig &instance() {
      static FreqConfig fc;
      return fc;
  }

  [[nodiscard]] size_t get_processor_count() const {
      return processor_count;
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

using namespace ankerl::unordered_dense::detail;

struct HashedStringView {
  std::string_view string;
  uint64_t hash;

  explicit HashedStringView(std::string_view str) {
      string = str;
      hash = wyhash::hash(str.data(), sizeof(char) * str.size());
  }
};

struct HashedString {
  std::string string;
  uint64_t hash;

  explicit HashedString(const HashedStringView &str) : string(str.string), hash(str.hash) {
  }

  explicit HashedString(const std::string_view &str) {
      string = str;
      hash = wyhash::hash(str.data(), sizeof(char) * str.size());
  }

  explicit HashedString(const std::string &str) {
      string = str;
      hash = wyhash::hash(str.data(), sizeof(char) * str.size());
  }
};

struct UnwrapHash {
  using is_transparent = std::true_type;
  auto operator()(HashedString const &str) const noexcept -> uint64_t {
      return str.hash;
  }

  auto operator()(HashedStringView const &str) const noexcept -> uint64_t {
      return str.hash;
  }
};

struct HashedStringEquals {
  using is_transparent = std::true_type;

  bool operator()(HashedString const &lhs, HashedString const &rhs) const {
      return lhs.string == rhs.string && lhs.hash == rhs.hash;
  }

  bool operator()(HashedString const &lhs, HashedStringView const &rhs) const {
      return lhs.string == rhs.string && lhs.hash == rhs.hash;
  }

  bool operator()(HashedStringView const &rhs, HashedString const &lhs) const {
      return lhs.string == rhs.string && lhs.hash == rhs.hash;
  }
};

std::vector<char> read_data(const std::string &filename) {
    const auto &config = FreqConfig::instance();
    const size_t file_size = std::filesystem::file_size(filename);
    const size_t chunks = (file_size + config.get_disk_page_size() - 1) / config.get_disk_page_size();

    std::vector<char> data(file_size);
    std::vector<std::pair<const char *, const char *>> chunk_edges(chunks);

    struct PerThreadData {
      std::ifstream file;
      ankerl::unordered_dense::map<HashedString, size_t, UnwrapHash, HashedStringEquals> frequency;
    };

    std::vector<PerThreadData> per_thread(config.get_processor_count());
    for (auto &tld : per_thread) {
        tld.file.open(filename);
    }

    {
        auto thread_pool = ThreadPool(config.get_processor_count());

        for (size_t i = 0; i < chunks; i++) {
            thread_pool.enqueue([&, i, file_size](size_t thread_index) {
              size_t chunk_size = config.get_disk_page_size();
              size_t start_pos = i * chunk_size;
              size_t end_pos = (i == chunks - 1)
                               ? file_size
                               : start_pos + chunk_size;
              size_t size = end_pos - start_pos;

              auto &tld = per_thread[thread_index];

              tld.file.seekg(static_cast<int>(start_pos));
              tld.file.read(static_cast<char *>(data.data()) + start_pos, static_cast<int>(size));

              static constexpr std::string_view delim{" \f\n\r\t\v!\"#$%&'()*+,-./:;<=>?@[\\]^_{|}~"};

              auto from = data.begin() + static_cast<std::ptrdiff_t>(start_pos);
              auto to = data.begin() + static_cast<std::ptrdiff_t>(end_pos);
              auto reversed_from = std::reverse_iterator(to);
              auto reversed_to = std::reverse_iterator(from);

              auto start = std::find_if(from, to, [](char c) {
                return delim.contains(c);
              });

              auto end = std::find_if(reversed_from, reversed_to, [](char c) {
                return delim.contains(c);
              });

              // It is possible that the word would occupy a whole chunk.
              char *p1 = nullptr;
              char *p2 = nullptr;

              if (start != to)
                  p1 = start.base();
              if (end != reversed_to)
                  p2 = end.base().base();

              chunk_edges[start_pos / chunk_size] = {p1, p2};

              for (const auto word : std::views::split(std::string_view(data.data()), delim)) {
                  const auto &x = HashedStringView(std::string_view(word));
                  const auto &[it, emplaced] = tld.frequency.try_emplace(x, 1);
                  if (!emplaced) {
                      ++it->second;
                  }
              }
            });
        }
    }

    ankerl::unordered_dense::map<HashedString, size_t, UnwrapHash, HashedStringEquals> edges_words_frequency;

    const char *left = chunk_edges.begin()->first;
    const char *right = chunk_edges.begin()->second;
    bool file_is_one_word = true;

    // Process words have been split by chunks edges
    for (auto chunk_it = chunk_edges.begin(); chunk_it != chunk_edges.end() - 1; ++chunk_it) {
        auto &[curr_left, curr_right] = *chunk_it;
        auto &[next_left, next_right] = *(chunk_it + 1);

        if (left == nullptr && curr_right != nullptr) {
            left = curr_right;
        }
        if (right == nullptr && next_left != nullptr) {
            right = next_left;
        }

        if (left != nullptr && right != nullptr) {
            file_is_one_word = false;
            const auto &x = HashedStringView(std::string_view(left, right - left));
            const auto &[it, emplaced] = edges_words_frequency.try_emplace(x, 1);
            if (!emplaced) {
                ++it->second;
            }
            left = right = nullptr;
        }
    }

    // Process the case of a huge single word
    if (file_is_one_word) {
        const auto &x = HashedStringView(std::string_view(data.data(), data.size()));
        const auto &[it, emplaced] = edges_words_frequency.try_emplace(x, 1);
        if (!emplaced) {
            ++it->second;
        }
    }

    return data;
}

int main() {
    const auto filename = "../test.txt";
    const auto &data = read_data(filename);

    std::cout << data.size() << '\n';

    return 0;
}
