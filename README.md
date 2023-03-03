# freq - A Word Frequency Counter

*freq* is a command-line tool that counts the frequency of words in a file and writes the results to an output file.

## Usage

To use *freq*, simply run the executable with the name of the input file and output file as arguments:

```
./freq [input_file] [output_file]
```


## Building

To build *freq*, you must have a C++ compiler and CMake installed on your system. You can then build *freq* using the following command:
```
mkdir build
cd build
cmake ..
make
```