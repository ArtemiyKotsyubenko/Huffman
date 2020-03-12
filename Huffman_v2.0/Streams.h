#ifndef HUFFMAN_V2_0_STREAMS_H
#define HUFFMAN_V2_0_STREAMS_H

#include <fstream>
#include <iostream>

class Ofstream_wrap {
public:
    std::ofstream file;

    explicit Ofstream_wrap(const char *path) : file(path) {
        if (file.fail()) {
            std::cerr << "Error opening file \n";
            exit(1);
        }
    }

    ~Ofstream_wrap() {
        file.close();
    }

};

class Ifstream_wrap {
public:
    std::ifstream file;

    explicit Ifstream_wrap(const char *path) : file(path) {
        if (file.fail()) {
            std::cerr << "Error opening file \n";
            exit(1);
        }
    }

    ~Ifstream_wrap() {
        file.close();
    }
};

#endif //HUFFMAN_V2_0_STREAMS_H
