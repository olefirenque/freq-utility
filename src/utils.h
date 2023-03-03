#ifndef FREQ_SRC_UTILS_H
#define FREQ_SRC_UTILS_H

#include <sys/stat.h>
#include <thread>

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

static bool is_delim(char c) {
    return !std::isalpha(c);
};

#endif //FREQ_SRC_UTILS_H
