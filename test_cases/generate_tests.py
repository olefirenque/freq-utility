import os
import random


def generate_dict_words_tests():
    word_file = "/usr/share/dict/words"
    WORDS = open(word_file).read().splitlines()

    TEST_DIR = "dict_words"

    if not os.path.exists(TEST_DIR):
        os.makedirs(TEST_DIR)

    for power in range(3, 8):
        test_size = 10 ** power
        with open(f"{TEST_DIR}/test-{test_size}.txt", "w") as test_file:
            for i in range(test_size):
                print(random.choice(WORDS), end=" ", file=test_file, flush=False)


def generate_single_word_tests():
    TEST_DIR = "single_word"

    if not os.path.exists(TEST_DIR):
        os.makedirs(TEST_DIR)

    for power in range(3, 8):
        test_size = 10 ** power
        with open(f"{TEST_DIR}/test-{test_size}.txt", "w") as test_file:
            print("a" * test_size, file=test_file)


if __name__ == '__main__':
    generate_dict_words_tests()
    generate_single_word_tests()
