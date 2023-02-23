#include <iostream>
#include "freq.h"

int main() {
    const auto filename = "../test.txt";
    const auto &data = read_data(filename);

    std::cout << data.size() << '\n';

    return 0;
}
