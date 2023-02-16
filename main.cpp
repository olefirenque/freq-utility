#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <filesystem>
#include <thread>
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
  }

  [[nodiscard]] size_t get_disk_page_size() const {
      return disk_page_size;
  }

 private:
  FreqConfig() {
      struct stat fi{};
      stat("/", &fi);

      processor_count = std::thread::hardware_concurrency();
      disk_page_size = fi.st_blksize;
  };

  size_t processor_count;
  size_t disk_page_size;
};

int main() {
    const auto filename = "test.txt";

    const auto &config = FreqConfig::instance();

    const size_t file_size = std::filesystem::file_size(filename);

    std::vector<char> data(file_size);

    std::cout << "Reading " << file_size / config.get_disk_page_size() << " chunks\n";
    std::cout << "Using " << config.get_processor_count() << " threads\n";

    std::vector<std::ifstream> file_per_thread(config.get_processor_count());
    for (auto &file : file_per_thread) {
        file.open(filename);
    }

    auto thread_pool = ThreadPool(config.get_processor_count());

    const size_t chunks = (file_size + config.get_disk_page_size() - 1) / config.get_disk_page_size();
    for (size_t i = 0; i < chunks; i++) {
        thread_pool.enqueue([i, &data, &config, file_size, &file_per_thread, chunks](size_t thread_index) {
          size_t start_pos = i * config.get_disk_page_size();
          size_t end_pos = (i == chunks - 1)
                           ? file_size
                           : start_pos + config.get_disk_page_size();
          size_t size = end_pos - start_pos;

          file_per_thread[thread_index].seekg(static_cast<int>(start_pos));
          file_per_thread[thread_index].read(static_cast<char *>(data.data()) + start_pos, static_cast<int>(size));
        });
    }

    return 0;
}
