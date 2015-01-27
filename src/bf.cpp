#include "bf.h"

bf_string bf_compile (std::string src) {
    bf_string out;
    out.resize(2 * src.size());
    uint8_t delta;
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
            case '.':
                count = 1;
                while (src[++src_idx] == '.')
                    count++;
                out[out_idx++] = INST_PUTCH;
                out[out_idx++] = count;
                break;
            default:
                src_idx++;
        }
    }
    out.resize(out_idx);
    return out;
}

void bf_run (bf_vm& vm, bf_string bytecode) {
    for (size_t i = 0; i < bytecode.size(); i += 2) {
        uint32_t instruction = bytecode[i];
        uint32_t arg = bytecode[i + 1];
        switch (instruction) {
            case INST_INC:
                vm.mem[vm.mem_ptr] += (uint8_t)arg;
                break;
            case INST_PUTCH:
                for (uint32_t count = 0; count < arg; count++) {
                    printf("%c", vm.mem[vm.mem_ptr]);
                }
                fflush(stdout);
                break;
        }
    }
}
