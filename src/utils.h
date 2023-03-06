#ifndef FREQ_SRC_UTILS_H
#define FREQ_SRC_UTILS_H

#include <sys/stat.h>
#include <thread>
#include "../libs/unordered_dense.h"

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

struct HeteroStringHash {
  using is_transparent = std::true_type;

  auto operator()(std::string const &str) const noexcept -> uint64_t {
      return wyhash::hash(str.data(), sizeof(char) * str.size());
  }

  auto operator()(std::string_view const &str) const noexcept -> uint64_t {
      return wyhash::hash(str.data(), sizeof(char) * str.size());
  }
};

using FreqMap = ankerl::unordered_dense::map<std::string, size_t, HeteroStringHash, std::equal_to<void>>;

static bool is_delim(char c) {
    return !std::isalpha(c);
};

#endif //FREQ_SRC_UTILS_H
