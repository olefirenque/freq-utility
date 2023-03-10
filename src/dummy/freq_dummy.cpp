#include <filesystem>
#include <vector>
#include <fstream>
#include <algorithm>

#include "freq_dummy.h"
#include "../utils.h"

FreqMap process_file_dummy(const std::string &filename) {
    const size_t file_size = std::filesystem::file_size(filename);
    std::vector<char> data(file_size);

    std::ifstream file;
    file.open(filename);

    file.seekg(0);
    file.read(data.data(), static_cast<int>(file_size));

    FreqMap result;
    result.reserve(file_size / 3);

    std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) { return std::tolower(c); });
    auto start_it = std::find_if_not(data.begin(), data.end(), is_delim);
    auto end_it = data.end();

    while (start_it < end_it) {
        auto word_end = std::find_if(start_it, end_it, is_delim);
        const auto &x = std::string(start_it.base(), word_end - start_it);
        const auto &[it, emplaced] = result.try_emplace(x, 1);
        if (!emplaced) {
            ++it->second;
        }
        start_it = std::find_if_not(word_end, end_it, is_delim);
    }

    return result;
}
