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
                if power % 2 == 0:
                    print(end=" ", file=test_file)
                    print(random.choice(WORDS), end="", file=test_file, flush=False)
                else:
                    print(random.choice(WORDS), end=" ", file=test_file, flush=False)


def generate_single_word_tests():
    TEST_DIR = "single_word"

    if not os.path.exists(TEST_DIR):
        os.makedirs(TEST_DIR)

    for power in range(3, 8):
        test_size = 10 ** power
        with open(f"{TEST_DIR}/test-{test_size}.txt", "w") as test_file:
            print("a" * test_size, end="", file=test_file)


def generate_unique_words_tests():
    TEST_DIR = "unique_words"

    if not os.path.exists(TEST_DIR):
        os.makedirs(TEST_DIR)

    import secrets

    for power in range(3, 8):
        test_size = 10 ** power
        with open(f"{TEST_DIR}/test-{test_size}.txt", "w") as test_file:
            for i in range(int(test_size / 10)):
                if power % 2 == 0:
                    print(end=" ", file=test_file)
                    print(secrets.token_hex(6), end="", file=test_file, flush=False)
                else:
                    print(secrets.token_hex(6), end=" ", file=test_file, flush=False)


def generate_one_word_dict_tests():
    TEST_DIR = "one_word_dict"

    if not os.path.exists(TEST_DIR):
        os.makedirs(TEST_DIR)

    for power in range(3, 8):
        test_size = 10 ** power
        with open(f"{TEST_DIR}/test-{test_size}.txt", "w") as test_file:
            for i in range(test_size):
                print("aaaaa", end="", file=test_file, flush=False)


def generate_words40k_offset_tests():
    TEST_DIR = "40k_offset"

    if not os.path.exists(TEST_DIR):
        os.makedirs(TEST_DIR)

    import secrets

    for power in range(3, 8):
        test_size = 10 ** power
        with open(f"{TEST_DIR}/test-{test_size}.txt", "w") as test_file:
            for i in range(int(test_size / 1000)):
                if power % 2 == 0:
                    print(end=" ", file=test_file)
                    print(secrets.token_hex(40000), end="", file=test_file, flush=False)
                else:
                    print(secrets.token_hex(40000), end=" ", file=test_file, flush=False)


if __name__ == '__main__':
    generate_dict_words_tests()
    generate_single_word_tests()
    generate_unique_words_tests()
    generate_one_word_dict_tests()
    generate_words40k_offset_tests()
