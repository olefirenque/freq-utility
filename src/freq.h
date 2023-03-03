#ifndef FREQ_SRC_FREQ_H
#define FREQ_SRC_FREQ_H

#include <string>
#include "../libs/unordered_dense.h"

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

FreqMap process_file_blocking_read(const std::string &filename);
#ifdef ENABLE_PROCESS_MMAPED_FILE
FreqMap process_mmaped_file(const std::string &filename);
#endif
#ifdef HAS_LIBAIO
FreqMap process_file_aio(const std::string &filename);
#endif

#endif //FREQ_SRC_FREQ_H
