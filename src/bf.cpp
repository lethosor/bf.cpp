#include "bf.h"

bf_file_contents bf_read_file (const char* path) {
    uint8_t* out = NULL;
    size_t length = 0;
    FILE* f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        out = (uint8_t*)malloc(length * sizeof(uint8_t));
        if (out) {
            fread(out, 1, length, f);
        }
        fclose(f);
    }
    bf_file_contents res;
    res.contents = out;
    res.length = length;
    return res;
}

bool bf_write_file (const char* path, bf_file_contents contents) {
    FILE* f = fopen(path, "wb");
    if (!f)
        return false;
    fwrite(contents.contents, sizeof(uint8_t), contents.length, f);
    fclose(f);
    return true;
}

bf_bytecode* bf_compile (std::string src, std::vector<bf_optimize_fn>* optimize_fns) {
    uint8_t* out = new uint8_t[8 * src.size()];
    uint8_t* out_start = out;
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
                    BC_WRITE_INC(out, bf_instruction, INST_INC);
                    BC_WRITE_INC(out, uint8_t, delta);
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
                    BC_WRITE_INC(out, bf_instruction, INST_MOVE);
                    BC_WRITE_INC(out, uint32_t, mem_delta);
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
                        BC_WRITE_INC(out, bf_instruction, INST_SET);
                        BC_WRITE_INC(out, uint8_t, value);
                        src_idx = peek_idx;
                        break;
                    }
                }
                BC_WRITE_INC(out, bf_instruction, INST_JZ);
                BC_WRITE_INC(out, uint32_t, 0);
                src_idx++;
                break;
            case ']':
                BC_WRITE_INC(out, bf_instruction, INST_JNZ);
                BC_WRITE_INC(out, uint32_t, 0);
                src_idx++;
                break;
            case '.':
            case ',':
                count = 1;
                while (src[++src_idx] == ch)
                    count++;
                BC_WRITE_INC(out, bf_instruction, (ch == '.') ? INST_PUTCH : INST_GETCH);
                BC_WRITE_INC(out, uint32_t, count);
                break;
            default:
                std::cerr << "unrecognized source character: " << ch << std::endl;
                src_idx++;
                break;
        }
    }

    size_t out_size = out - out_start;
    bf_bytecode* bc_out = new bf_bytecode(out_size);
    memcpy(bc_out->contents, out_start, out_size * sizeof(uint8_t));

    if (optimize_fns) {
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t i = 0; i < optimize_fns->size(); i++) {
                bf_optimize_fn func = (*optimize_fns)[i];
                bf_bytecode* bc_in = bc_out;
                bc_out = new bf_bytecode(bc_in->length);
                func(bc_in, bc_out);
                if (bc_in->length != bc_out->length)
                    changed = true;
                else if (memcmp(bc_in->contents, bc_out->contents, bc_in->length) != 0)
                    changed = true;
                delete bc_in;
            }
        }
    }

    std::vector<uint32_t> loop_stack;
    out = bc_out->contents;
    out_start = out;
    while (out < bc_out->contents + bc_out->length) {
        uint32_t i = out - bc_out->contents;
        bf_instruction instruction;
        uint32_t size;
        BC_READ_INC(out, bf_instruction, instruction);
        switch (instruction) {
            case INST_JZ:
                loop_stack.push_back(i);
                BC_INC(out, sizeof(uint32_t));
                break;
            case INST_JNZ:
                size = (uint32_t)loop_stack.size();
                if (size) {
                    *(uint32_t*)((uint8_t*)(out_start + loop_stack[size - 1]) + sizeof(bf_instruction)) = i;
                    BC_WRITE_INC(out, uint32_t, loop_stack[size - 1]);
                    loop_stack.resize(size - 1);
                }
                else {
                    std::cerr << "unexpected end loop" << std::endl;
                    return NULL;
                }
                break;
            case INST_INC:
            case INST_SET:
                BC_INC(out, sizeof(uint8_t));
                break;
            default:
                BC_INC(out, sizeof(uint32_t));
                break;
        }
    }
    return bc_out;
}

void bf_optimize_remove_duplicates (bf_bytecode* bc_in, bf_bytecode* bc_out) {
    uint8_t* in = bc_in->contents;
    uint8_t* out = bc_out->contents;
    bf_instruction last = (bf_instruction)-1, cur = (bf_instruction)-2;
    uint32_t* arg = new uint32_t;
    while (in < bc_in->contents + bc_in->length) {
        BC_READ_INC(in, bf_instruction, cur);
        switch (cur) {
            case INST_INC:
            case INST_SET:
                BC_READ_INC(in, uint8_t, *arg);
                if (cur == last) {
                    uint8_t* dest = out - sizeof(uint8_t);
                    *dest = (cur == INST_INC) ? (*dest + *(uint8_t*)arg) : *(uint8_t*)arg;
                }
                else {
                    BC_WRITE_INC(out, bf_instruction, cur);
                    BC_WRITE_INC(out, uint8_t, *arg);
                }
                break;
            case INST_MOVE:
            case INST_PUTCH:
            case INST_GETCH:
                BC_READ_INC(in, uint32_t, *arg);
                if (cur == last) {
                    *(out - sizeof(uint32_t)) += *arg;
                }
                else {
                    BC_WRITE_INC(out, bf_instruction, cur);
                    BC_WRITE_INC(out, uint32_t, *arg);
                }
                break;
            default:
                BC_READ_INC(in, uint32_t, *arg);
                BC_WRITE_INC(out, bf_instruction, cur);
                BC_WRITE_INC(out, uint32_t, *arg);
                break;
        }
        last = cur;
    }
    bc_out->length = out - bc_out->contents;
}

void bf_run (bf_vm& vm, bf_bytecode* bytecode) {
    uint8_t* contents = bytecode->contents;
    uint8_t* contents_start = contents;
    uint8_t* contents_end = contents + bytecode->length;
    bf_instruction instruction;
    uint32_t* arg = new uint32_t;
    while (contents != contents_end) {
        BC_READ_INC(contents, bf_instruction, instruction);
        switch (instruction) {
            case INST_INC:
                BC_READ_INC(contents, uint8_t, *arg);
                vm.mem[vm.mem_ptr] += *(uint8_t*)arg;
                break;
            case INST_MOVE:
                BC_READ_INC(contents, uint32_t, *arg);
                vm.mem_ptr = (vm.mem_ptr + *arg) % vm.mem_size;
                break;
            case INST_JZ:
                BC_READ_INC(contents, uint32_t, *arg);
                if (!vm.mem[vm.mem_ptr])
                    contents = contents_start + *arg;
                break;
            case INST_JNZ:
                BC_READ_INC(contents, uint32_t, *arg);
                if (vm.mem[vm.mem_ptr])
                    contents = contents_start + *arg;
                break;
            case INST_PUTCH:
                BC_READ_INC(contents, uint32_t, *arg);
                for (uint32_t count = 0; count < *arg; count++) {
                    std::cout << (unsigned char)vm.mem[vm.mem_ptr];
                }
                std::cout.flush();
                break;
            case INST_GETCH:
                BC_READ_INC(contents, uint32_t, *arg);
                for (uint32_t count = 0; count < *arg; count++) {
                    int ch = std::cin.get();
                    if (ch != -1)
                        vm.mem[vm.mem_ptr] = ch;
                    else {
                        if (vm.opts.eof_flag == BF_EOF_0)
                            vm.mem[vm.mem_ptr] = 0;
                        else if (vm.opts.eof_flag == BF_EOF_NEG1)
                            vm.mem[vm.mem_ptr] = -1;
                    }
                }
                break;
            case INST_SET:
                BC_READ_INC(contents, uint8_t, *arg);
                vm.mem[vm.mem_ptr] = *(uint8_t*)arg;
                break;
            default:
                std::cerr << (vm.opts.unknown_fatal ? "fatal" : "warn")
                    << ": unrecognized instruction: " << instruction << std::endl;
                if (vm.opts.unknown_fatal)
                    return;
                contents++;
                break;
        }
    }
}

void bf_disassemble (bf_bytecode* bytecode, FILE* out) {
    static std::map<uint32_t, const char*> imap;
    if (!imap.size()) {
        imap[INST_INC]   = "INC  ";
        imap[INST_MOVE]  = "MOVE ";
        imap[INST_JZ]    = "JZ   ";
        imap[INST_JNZ]   = "JNZ  ";
        imap[INST_GETCH] = "GETCH";
        imap[INST_PUTCH] = "PUTCH";
        imap[INST_SET]   = "SET  ";
    }
    uint8_t* contents_start = bytecode->contents;
    uint8_t* contents = contents_start;
    bf_instruction instruction;
    uint32_t* arg = new uint32_t;
    uint32_t maxarg = 0;
    int i = 1;
    while (contents < contents_start + bytecode->length) {
        fprintf(out, "inst #%-3i idx=%-4lli: ", i++, int64_t(contents - contents_start));
        BC_READ_INC(contents, bf_instruction, instruction);
        fprintf(out, "%02i %s ", instruction,
            imap.find(instruction) != imap.end() ? imap[instruction] : "?????");
        switch (instruction) {
            case INST_SET:
                BC_READ_INC(contents, uint8_t, *arg);
                fprintf(out, "%6i ", *(uint8_t*)arg);
                fprintf(out, "[-]");
                // fallthru
            case INST_INC:
                if (instruction != INST_SET) {
                    BC_READ_INC(contents, uint8_t, *arg);
                    fprintf(out, "%6i ", *(uint8_t*)arg);
                }
                if (*(uint8_t*)arg <= 128) {
                    for (uint8_t i = 0; i < *arg; i++)
                        fprintf(out, "+");
                }
                else {
                    for (uint8_t i = 255; i >= *arg; i--)
                        fprintf(out, "-");
                }
                break;
            case INST_MOVE:
                BC_READ_INC(contents, uint32_t, *arg);
                maxarg = *arg;
                fprintf(out, "%6i ", *arg);
                if (*(int32_t*)arg >= 0) {
                    maxarg = (*arg > 255) ? 255 : *arg;
                    for (uint32_t i = 0; i < maxarg; i++)
                        fprintf(out, ">");
                }
                else {
                    maxarg = (*arg < UINT32_MAX - 255) ? UINT32_MAX - 255 : *arg;
                    for (uint32_t i = UINT32_MAX; i >= maxarg; i--)
                        fprintf(out, "<");
                }
                if (maxarg != *arg)
                    printf(" (truncated)");
                break;
            case INST_JZ:
            case INST_JNZ:
                BC_READ_INC(contents, uint32_t, *arg);
                fprintf(out, "%6i ", *arg);
                fprintf(out, "%c", instruction == INST_JZ ? '[' : ']');
                break;
            case INST_GETCH:
            case INST_PUTCH:
                BC_READ_INC(contents, uint32_t, *arg);
                fprintf(out, "%6i ", *arg);
                for (uint32_t i = 0; i < *arg; i++)
                    fprintf(out, "%c", instruction == INST_GETCH ? ',' : '.');
                break;
        }
        fprintf(out, "\n");
    }
}

static const char* c_start =
"#include <unistd.h>\n"
"#include <stdint.h>\n"
"uint8_t r[65536],*e=r;\n"
"int main(){\n";
static const char* c_end =
"  return 0;\n"
"}\n";

void bf_dump_c (bf_bytecode* bytecode, FILE* out) {
    fprintf(out, "%s", c_start);
    uint8_t* contents_start = bytecode->contents;
    uint8_t* contents = contents_start;
    bf_instruction instruction;
    uint32_t* arg = new uint32_t;
    int level = 1;
    #define indent(level) for (int i##__LINE__ = level; i##__LINE__ > 0; i##__LINE__--) { fprintf(out, " "); }
    while (contents < contents_start + bytecode->length) {
        BC_READ_INC(contents, bf_instruction, instruction);
        switch (instruction) {
            case INST_INC:
            case INST_SET:
                BC_READ_INC(contents, uint8_t, *arg);
                indent(level);
                fprintf(out, "*e%s%i;\n", (instruction == INST_SET) ? "=" : "+=", (int)(*(uint8_t*)arg));
                break;
            case INST_MOVE:
                BC_READ_INC(contents, uint32_t, *arg);
                indent(level);
                if (*arg > INT32_MAX) {
                    fprintf(out, "e-=%u;\n", UINT32_MAX - *arg + 1);
                }
                else {
                    fprintf(out, "e+=%u;\n", *arg);
                }
                break;
            case INST_JZ:
                BC_INC(contents, sizeof(uint32_t));
                indent(level++);
                fprintf(out, "while(*e){\n");
                break;
            case INST_JNZ:
                BC_INC(contents, sizeof(uint32_t));
                indent(--level);
                fprintf(out, "}\n");
                break;
            case INST_GETCH:
            case INST_PUTCH:
                BC_READ_INC(contents, uint32_t, *arg);
                for (uint32_t i = 0; i < *arg; i++) {
                    indent(level);
                    fprintf(out, "%s(%i, e, 1);\n",
                        (instruction == INST_GETCH) ? "read" : "write",
                        (instruction == INST_GETCH) ? 0 : 1);
                }
                break;
        }
    }
    fprintf(out, "%s", c_end);
    delete arg;
    #undef indent
}
