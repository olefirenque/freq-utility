#ifndef FREQ_SRC_FREQ_H
#define FREQ_SRC_FREQ_H

#include <string>
#include "utils.h"

FreqMap process_file_blocking_read(const std::string &filename);
#ifdef ENABLE_PROCESS_MMAPED_FILE
FreqMap process_mmaped_file(const std::string &filename);
#endif
#ifdef HAS_LIBAIO
FreqMap process_file_aio(const std::string &filename);
#endif

#endif //FREQ_SRC_FREQ_H
