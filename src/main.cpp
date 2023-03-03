#include <iostream>
#include <fstream>
#include "freq.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " [input_file] [output_file]>" << std::endl;
        std::exit(1);
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    const auto &data = process_file_blocking_read(input_file);

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
