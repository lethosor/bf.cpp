#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string.h>
#include <stdint.h>
#include <vector>

typedef std::basic_string<uint32_t> bf_string;

enum bf_instruction : uint32_t {
    INST_INC = 0,
    INST_MOVE,
    INST_JZ,
    INST_JNZ,
    INST_GETCH,
    INST_PUTCH
};

struct bf_vm {
    uint32_t mem_size;
    uint32_t mem_ptr;
    uint8_t* mem;
    bf_vm(uint32_t _mem_size) {
        mem_size = _mem_size;
        mem_ptr = 0;
        mem = new uint8_t[mem_size];
        memset(mem, 0, sizeof(uint8_t) * mem_size);
    }
    ~bf_vm() {
        delete[] mem;
    }
};

extern "C" {
    bf_string bf_compile (std::string src);
    void bf_run (bf_vm& vm, bf_string bytecode);
    void bf_disassemble (bf_string bytecode, std::ostream& out);
}
