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

    // TODO: apply sort by freq and lexicographically
    const FreqMap &data = process_file(input_file);

    std::ofstream output;
    output.open(output_file);
    for (const auto &[word, count] : data) {
        output << word << ' ' << count << '\n';
    }
    output.close();

    return 0;
}
