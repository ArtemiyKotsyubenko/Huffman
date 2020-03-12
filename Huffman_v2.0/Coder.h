#ifndef HUFFMAN_V2_0_CODER_H
#define HUFFMAN_V2_0_CODER_H

#include <vector>
#include <stack>
#include <queue>
#include "Streams.h"
#include "Huffman_Tree.h"

class Coder {
private:

    Ifstream_wrap Fin;
    Ofstream_wrap Fout;

protected:
    struct counter {
        unsigned char i = 0;
        const unsigned char mod;

        void operator++() {
            ++i %= mod;
        }

        operator unsigned char() { return i; }

        counter(const unsigned char mod_) : mod(mod_) {}
    };

    Huffman_Tree tree;
    std::ifstream &fin;
    std::ofstream &fout;

public:
    Coder(const char *input_file, const char *output_file) :
            Fin(input_file),
            Fout(output_file),
            fin(Fin.file),
            fout(Fout.file) {}

    ~Coder() = default;

};

class Encoder : protected Coder {
private:
    class Bit_writer {
    private:
        std::queue<unsigned char> bit_buffer;
        unsigned char bit_to_write = 0;
        counter cnt;
        std::ofstream &fout;


        void write() {
            while (!bit_buffer.empty()) {
                if (cnt < 8) {
                    bit_to_write <<= 1;
                    bit_to_write |= bit_buffer.front();
                    bit_buffer.pop();
                } else {
                    fout << bit_to_write;
                }
                ++cnt;
            }
        }

    public:
        Bit_writer(std::ofstream &fout_) : fout(fout_), cnt(9) {}

        //awaits std::vector<unsigned char> with elements 0 , 1
        void push_back(const std::vector<unsigned char> &code) {
            for (auto &it : code) {
                bit_buffer.push(it);
            }
            write();
        }

        void end_of_stream_notifier() {
            while (cnt < 8) {
                bit_to_write <<= 1;
                ++cnt;
            }
            fout << bit_to_write;
        }

        ~Bit_writer() = default;
    } bit_writer;

    std::vector<unsigned char> get_base_symbol_code(unsigned char symbol) {
        std::vector<unsigned char> result;
        std::stack<unsigned char> st;

        for (int i = 0; i < 8; ++i) {
            st.push(symbol & 1);
            symbol >>= 1;
        }

        while (!st.empty()) {
            result.push_back(st.top());
            st.pop();
        }
        return result;
    }


public:

    Encoder(const char *input_file, const char *output_file) : Coder(input_file, output_file), bit_writer(fout) {
        if (fin.peek() != EOF) {
            for (char ch = 0; fin.get(ch);) {
                unsigned char c = ch;
                if (tree.exists(c)) {
                    bit_writer.push_back(tree.get_symbol_code(c));
                } else {
                    bit_writer.push_back(tree.get_delim_code());
                    bit_writer.push_back(get_base_symbol_code(c));
                }
                tree.insert(c);
            }
            bit_writer.end_of_stream_notifier();
        }
    }

    ~Encoder() = default;
};

class Decoder : protected Coder {
private:
    Huffman_Tree_adapter adapter;
    bool delimiter_reached = false;
    bool stream_is_active = true;
    unsigned char position_mask = 128;// = 2^7 - position of first bit
    counter cnt;
    unsigned char current_byte = 0;

    bool next_bit() {
        bool bit = current_byte & position_mask;

        if (cnt < 7) {
            position_mask >>= 1; // 2^(n-1) - position of next bit

        } else {
            char c;
            if (fin.get(c)) {
                current_byte = c;
                position_mask = 128;
            } else {
                stream_is_active = false;
            }
        }
        ++cnt;
        return bit;
    }

public:
    Decoder(const char *input_file, const char *output_file) : Coder(input_file, output_file), adapter(tree), cnt(8) {
        /*
        * read byte
        * read its bits and move tree ptr.
        * if symbol reached - write it
        * if delimiter reached - read next 8 bits like ASKII symbol code
        * insert symbol in the tree
        */
        if (fin.peek() != EOF) {
            {
                char c;
                fin.get(c);
                current_byte = c;
                tree.insert(current_byte);
                fout << current_byte;
            }
            if (fin.peek() != EOF) {
                current_byte = fin.get();
                while (stream_is_active) {
                    if (adapter.move(next_bit())) {
                        unsigned char symbol = 0;
                        if (adapter.is_symbol_code()) {
                            symbol = adapter.get_symbol();
                            fout << symbol;
                        } else {//if delimiter reached -
                            for (unsigned char i = 0; i < 8; ++i) {// read next 8 bits like ASKII symbol code
                                symbol <<= 1;
                                symbol |= next_bit();
                            }
                            if (symbol != 0) {// encoder writes 'empty' bits like 0
                                fout << symbol;
                            } else {
                                stream_is_active = false;
                            }
                        }
                        tree.insert(symbol);
                        adapter.rewind();
                    }
                }
            }
        }
    }

    ~Decoder() = default;

};

#endif //HUFFMAN_V2_0_CODER_H