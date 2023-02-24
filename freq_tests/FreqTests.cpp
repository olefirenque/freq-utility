#include "gtest/gtest.h"

#include "../src/freq.h"
#include "../src/dummy/freq_dummy.h"

void base_test(const std::string &test_dir) {
    for (const std::string &test : {
        "test-1000.txt",
        "test-10000.txt",
        "test-100000.txt",
        "test-1000000.txt",
        "test-10000000.txt",
    }) {
        const std::string filename(test_dir + test);
        auto actual = process_file(filename);
        auto x = process_file_dummy(filename);
        auto expected = FreqMap(x.begin(), x.end());

        EXPECT_EQ(actual, expected);
    }
}

TEST(freq_test, dict_words_test) {
    base_test("../test_cases/dict_words/");
}

TEST(freq_test, single_word_test) {
    base_test("../test_cases/single_word/");
}

TEST(freq_test, unique_words_test) {
    base_test("../test_cases/unique_words/");
}

TEST(freq_test, one_word_dict_test) {
    base_test("../test_cases/one_word_dict/");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
