#include "bf.h"

bf_string bf_compile (std::string src) {
    bf_string out;
    out.resize(2 * src.size());
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
                out[out_idx++] = INST_INC;
                out[out_idx++] = (uint32_t)delta;
                break;
            case '<':
            case '>':
                mem_delta = (ch == '>') ? 1 : -1;
                while (ch == '>' || ch == '<') {
                    ch = src[++src_idx];
                    if (ch == '>') mem_delta++;
                    else if (ch == '<') mem_delta--;
                }
                out[out_idx++] = INST_MOVE;
                out[out_idx++] = (uint32_t)mem_delta;
                break;
            case '[':
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
                count = 1;
                while (src[++src_idx] == '.')
                    count++;
                out[out_idx++] = INST_PUTCH;
                out[out_idx++] = count;
                break;
            default:
                src_idx++;
                break;
        }
    }
    out.resize(out_idx);
    std::vector<uint32_t> loop_stack;
    for (size_t i = 0; i < out.size(); i += 2) {
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
    return out;
}

void bf_run (bf_vm& vm, bf_string bytecode) {
    for (size_t i = 0; i < bytecode.size(); i += 2) {
        uint32_t instruction = bytecode[i];
        uint32_t arg = bytecode[i + 1];
        //printf("inst %i = %i, %i | mp=%i mc=%i\n", (int)i, instruction, arg, vm.mem_ptr, vm.mem[vm.mem_ptr]);
        switch (instruction) {
            case INST_INC:
                vm.mem[vm.mem_ptr] += (uint8_t)arg;
                break;
            case INST_MOVE:
                //printf("mp: %i + %i -> ", vm.mem_ptr, arg);
                vm.mem_ptr = (vm.mem_ptr + arg) % vm.mem_size;
                //printf("%i\n", vm.mem_ptr);
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
                    fprintf(stdout, "%c", vm.mem[vm.mem_ptr]);
                }
                fflush(stdout);
                break;
            default:
                std::cerr << "warn: unrecognized instruction: " << instruction << std::endl;
                break;
        }
    }
}

void bf_disassemble (bf_string bytecode, std::ostream& out) {
    static std::map<uint32_t, std::string> imap;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        imap[INST_INC] = "INC";
        imap[INST_MOVE] = "MOVE";
        imap[INST_JZ] = "JZ";
        imap[INST_JNZ] = "JNZ";
        imap[INST_GETCH] = "GETCH";
        imap[INST_PUTCH] = "PUTCH";
    }
    for (size_t i = 0; i < bytecode.size(); i += 2) {
        out << "instruction " << (int)i << ": ";
        out << imap[bytecode[i]] << "\t[" << bytecode[i + 1] << "]";
        out << std::endl;
    }
}
