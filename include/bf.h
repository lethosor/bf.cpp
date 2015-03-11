#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

typedef uint32_t bf_op;

enum bf_instruction : uint32_t {
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
    bf_op* contents;
    bf_bytecode (size_t _length) {
        length = _length;
        contents = new bf_op[length];
        memset(contents, 0, sizeof(bf_op) * length);
    }
    ~bf_bytecode() {
        delete[] contents;
    }
};

extern "C" {
    char* bf_read_file_contents (const char* path);
    bf_bytecode* bf_compile (std::string src);
    void bf_run (bf_vm& vm, bf_bytecode* bytecode);
    void bf_disassemble (bf_bytecode* bytecode, std::ostream& out);
}

char* bf_read_file_contents (const char* path) {
    char* out = NULL;
    long length = 0;
    FILE* f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        out = (char*)malloc(length * sizeof(char));
        if (out) {
            fread(out, 1, length, f);
        }
        fclose(f);
    }
    return out;
}

bf_bytecode* bf_compile (std::string src) {
    bf_op* out = new bf_op[2 * src.size()];
    uint8_t delta;
    uint32_t mem_delta;
    uint32_t count;
    size_t src_idx, out_idx;
    // Strip unrecognized characters
    src_idx = 0; out_idx = 0;
    std::string tmp = src;
    for ( ; src_idx < src.size(); src_idx++) {
        switch (src[src_idx]) {
            case '+':
            case '-':
            case '>':
            case '<':
            case '[':
            case ']':
            case '.':
            case ',':
                tmp[out_idx++] = src[src_idx];
                break;
            default:
                break;
        }
    }
    src = tmp;
    src.resize(out_idx);
    src_idx = 0; out_idx = 0;
    while (src_idx < src.size()) {
        char ch = src[src_idx];
        switch (ch) {
            case '+':
            case '-':
                delta = (ch == '+') ? 1 : -1;
                while (ch == '+' || ch == '-') {
                    ch = src[++src_idx];
                    if (ch == '+') delta++;
                    else if (ch == '-') delta--;
                }
                if (delta) {
                    out[out_idx++] = INST_INC;
                    out[out_idx++] = (uint32_t)delta;
                }
                break;
            case '<':
            case '>':
                mem_delta = (ch == '>') ? 1 : -1;
                while (ch == '>' || ch == '<') {
                    ch = src[++src_idx];
                    if (ch == '>') mem_delta++;
                    else if (ch == '<') mem_delta--;
                }
                if (mem_delta) {
                    out[out_idx++] = INST_MOVE;
                    out[out_idx++] = (uint32_t)mem_delta;
                }
                break;
            case '[':
                {
                    // Optimize [-]+++ patterns
                    delta = 0;  // Keep track of delta to avoid interpreting [+-] as [-]
                    bool optimize_set = false;
                    size_t peek_idx = src_idx + 1;
                    while (peek_idx < src.size()) {
                        switch (src[peek_idx]) {
                            case '+':
                                delta++;
                                break;
                            case '-':
                                delta--;
                                break;
                            case ']':
                                optimize_set = true;
                                goto peek_done;
                            default:
                                goto peek_done;
                        }
                        peek_idx++;
                    }
                    peek_done:
                    if (optimize_set && delta) {
                        int8_t value = 0;
                        peek_idx++;  // Skip trailing ]
                        for ( ; peek_idx < src.size(); peek_idx++) {
                            if (src[peek_idx] == '+')
                                value++;
                            else if (src[peek_idx] == '-')
                                value--;
                            else
                                break;
                        }
                        out[out_idx++] = INST_SET;
                        out[out_idx++] = (uint32_t)value;
                        src_idx = peek_idx;
                        break;
                    }
                }
                out[out_idx] = INST_JZ;
                out[out_idx + 1] = out_idx;
                out_idx += 2;
                src_idx++;
                break;
            case ']':
                out[out_idx] = INST_JNZ;
                out[out_idx + 1] = out_idx;
                out_idx += 2;
                src_idx++;
                break;
            case '.':
            case ',':
                count = 1;
                while (src[++src_idx] == ch)
                    count++;
                out[out_idx++] = (ch == '.') ? INST_PUTCH : INST_GETCH;
                out[out_idx++] = count;
                break;
            default:
                src_idx++;
                break;
        }
    }
    size_t out_size = out_idx;
    std::vector<uint32_t> loop_stack;
    for (size_t i = 0; i < out_size; i += 2) {
        if (out[i] == INST_JZ)
            loop_stack.push_back((uint32_t)i);
        else if (out[i] == INST_JNZ) {
            uint32_t size = (uint32_t)loop_stack.size();
            if (size) {
                out[i + 1] = (uint32_t)loop_stack[size - 1];
                out[loop_stack[size - 1] + 1] = (uint32_t)i;
                loop_stack.resize(size - 1);
            }
        }
    }
    bf_bytecode* bytecode = new bf_bytecode(out_size);
    memcpy(bytecode->contents, out, out_size * sizeof(bf_op));
    return bytecode;
}

void bf_run (bf_vm& vm, bf_bytecode* bytecode) {
    for (size_t i = 0; i < bytecode->length; i += 2) {
        uint32_t instruction = bytecode->contents[i];
        uint32_t arg = bytecode->contents[i + 1];
        switch (instruction) {
            case INST_INC:
                vm.mem[vm.mem_ptr] += (uint8_t)arg;
                break;
            case INST_MOVE:
                vm.mem_ptr = (vm.mem_ptr + arg) % vm.mem_size;
                break;
            case INST_JZ:
                if (!vm.mem[vm.mem_ptr])
                    i = arg;
                break;
            case INST_JNZ:
                if (vm.mem[vm.mem_ptr])
                    i = arg;
                break;
            case INST_PUTCH:
                for (uint32_t count = 0; count < arg; count++) {
                    std::cout << (unsigned char)vm.mem[vm.mem_ptr];
                }
                std::cout.flush();
                break;
            case INST_GETCH:
                for (uint32_t count = 0; count < arg; count++) {
                    int ch = std::cin.get();
                    if (ch != -1)
                        vm.mem[vm.mem_ptr] = ch;
                    else {
                        if (vm.eof_flag == BF_EOF_0)
                            vm.mem[vm.mem_ptr] = 0;
                        else if (vm.eof_flag == BF_EOF_NEG1)
                            vm.mem[vm.mem_ptr] = -1;
                    }
                }
                break;
            case INST_SET:
                vm.mem[vm.mem_ptr] = arg;
                break;
            default:
                std::cerr << "warn: unrecognized instruction: " << instruction << std::endl;
                break;
        }
    }
}

void bf_disassemble (bf_bytecode* bytecode, std::ostream& out) {
    static std::map<uint32_t, std::string> imap;
    if (!imap.size()) {
        imap[INST_INC]   = "INC  ";
        imap[INST_MOVE]  = "MOVE ";
        imap[INST_JZ]    = "JZ   ";
        imap[INST_JNZ]   = "JNZ  ";
        imap[INST_GETCH] = "GETCH";
        imap[INST_PUTCH] = "PUTCH";
        imap[INST_SET]   = "SET  ";
    }
    for (size_t i = 0; i < bytecode->length; i += 2) {
        out << "instruction " << (int)i << ": ";
        uint32_t instruction = bytecode->contents[i],
            arg = bytecode->contents[i + 1];
        out << imap[bytecode->contents[i]] << " (" << (int32_t)bytecode->contents[i + 1] << "):\t";
        switch (instruction) {
            case INST_SET:
                out << "[-]";
                // fallthru
            case INST_INC:
                if (arg <= 128) {
                    for (uint8_t i = 0; i < arg; i++)
                        out << '+';
                }
                else {
                    for (uint8_t i = 255; i >= arg; i--)
                        out << '-';
                }
                break;
            case INST_MOVE:
                if ((int32_t)arg >= 0) {
                    for (uint32_t i = 0; i < arg; i++)
                        out << '>';
                }
                else {
                    for (uint32_t i = UINT32_MAX; i >= arg; i--)
                        out << '<';
                }
                break;
            case INST_JZ:
                out << '[';
                break;
            case INST_JNZ:
                out << ']';
                break;
            case INST_GETCH:
                out << ',';
                break;
            case INST_PUTCH:
                out << '.';
                break;
        }
        out << std::endl;
    }
}
