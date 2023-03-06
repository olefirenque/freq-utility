#include <iostream>
#include <fstream>
#include "freq.h"
#include "utils.h"
#include "dummy/freq_dummy.h"

typedef FreqMap(*ProcessMethodType)(const std::string &filename);

ProcessMethodType get_method() {
    auto &config = FreqConfig::instance();

    if (config.get_processor_count() > 1) {
        return process_file_blocking_read;
    }
#ifdef HAS_LIBAIO
    return process_file_aio;
#else
    return process_file_dummy;
#endif
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " [input_file] [output_file]" << std::endl;
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    const auto &data = get_method()(input_file);

    std::vector<std::pair<std::string, size_t>> word_freq_pairs(
        std::move_iterator(data.begin()),
        std::move_iterator(data.end())
    );
    std::sort(word_freq_pairs.begin(), word_freq_pairs.end(), [](const auto &p1, const auto &p2) {
      auto &[word1, freq1] = p1;
      auto &[word2, freq2] = p2;
      return std::tie(freq2, word1) < std::tie(freq1, word2);
    });

    std::ofstream output;
    output.open(output_file);
    for (const auto &[word, count] : word_freq_pairs) {
        output << count << ' ' << word << '\n';
    }
    output.close();

    return 0;
}
