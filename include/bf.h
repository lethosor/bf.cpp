#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

enum bf_instruction : uint16_t {
    INST_INC = 0,
    INST_MOVE,
    INST_JZ,
    INST_JNZ,
    INST_GETCH,
    INST_PUTCH,
    INST_SET
};

enum bf_eof_flag : uint8_t {
    BF_EOF_0 = 0,
    BF_EOF_NEG1,
    BF_EOF_NO_CHANGE
};

struct bf_vm {
    uint32_t mem_size;
    uint32_t mem_ptr;
    uint8_t* mem;
    bf_eof_flag eof_flag;
    bf_vm(uint32_t _mem_size) {
        eof_flag = BF_EOF_NO_CHANGE;
        mem_size = _mem_size;
        mem_ptr = 0;
        mem = new uint8_t[mem_size];
        memset(mem, 0, sizeof(uint8_t) * mem_size);
    }
    ~bf_vm() {
        delete[] mem;
    }
};

struct bf_bytecode {
    size_t length;
    uint8_t* contents;
    bf_bytecode (size_t _length) {
        length = _length;
        contents = new uint8_t[length];
        memset(contents, 0, sizeof(uint8_t) * length);
    }
    ~bf_bytecode() {
        delete[] contents;
    }
};

#define BC_INC(ptr, delta) ptr = (uint8_t*)ptr + delta
#define BC_READ(ptr, type, dest) dest = *((type*)ptr)
#define BC_READ_INC(ptr, type, dest) BC_READ(ptr, type, dest); BC_INC(ptr, sizeof(type))
#define BC_WRITE(ptr, type, value) *((type*)ptr) = value
#define BC_WRITE_INC(ptr, type, value) BC_WRITE(ptr, type, value); BC_INC(ptr, sizeof(type))

extern "C" {
    char* bf_read_file_contents (const char* path);
    bf_bytecode* bf_compile (std::string src);
    void bf_run (bf_vm& vm, bf_bytecode* bytecode);
    void bf_disassemble (bf_bytecode* bytecode, FILE* out);
}
