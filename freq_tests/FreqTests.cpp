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
        auto x = process_file(filename);
        auto y = process_file_dummy(filename);
        auto actual = std::map(x.begin(), x.end());
        auto expected = std::map(y.begin(), y.end());
        decltype(actual) actual_minus_expected;
        decltype(actual) expected_minus_actual;

        std::set_difference(actual.begin(),
                            actual.end(),
                            expected.begin(),
                            expected.end(),
                            std::inserter(actual_minus_expected, actual_minus_expected.begin()));
        std::set_difference(expected.begin(),
                            expected.end(),
                            actual.begin(),
                            actual.end(),
                            std::inserter(expected_minus_actual, expected_minus_actual.begin()));

        EXPECT_EQ(actual_minus_expected, decltype(actual_minus_expected){});
        EXPECT_EQ(expected_minus_actual, decltype(expected_minus_actual){});
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

TEST(freq_test, l40k_offset_test) {
    base_test("../test_cases/40k_offset/");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
