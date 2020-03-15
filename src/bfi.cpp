#include "bf.h"

#include "tclap/CmdLine.h"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::string;
using std::stringstream;
using std::vector;

enum bfi_error : int {
    BFI_ARG_ERROR = 1,
    BFI_IO_ERROR,
    BFI_BYTECODE_ERROR,
    BFI_COMPILE_ERROR
};

int main (int argc, const char** argv) {
    using namespace TCLAP;
    bool disassemble = false,
        dump_c = false,
        stat = false,
        unknown_fatal = false;
    int mem_size;
    string path,
        eof,
        out_path;
    try {
        CmdLine cmd("Brainfuck interpreter", ' ', "0.1");
        UnlabeledValueArg<string> path_arg("path", "Path to file", true, "", "Path to file", cmd);
        SwitchArg disassemble_arg("d", "disassemble", "Dump bytecode", cmd, false);
        SwitchArg dump_c_arg("c", "dump-c", "Dump C translation", cmd, false);
        SwitchArg stat_arg("", "stat", "Display statistics", cmd, false);
        SwitchArg unknown_fatal_arg("", "unknown-fatal", "Abort on unknown instructions", cmd, false);
        vector<string> eof_flags;
        eof_flags.push_back("0");
        eof_flags.push_back("-1");
        eof_flags.push_back("n");
        eof_flags.push_back("nc");
        eof_flags.push_back("no-change");
        ValuesConstraint<string> eof_constraint(eof_flags);
        ValueArg<string> eof_arg("", "eof", "EOF behavior", false, "no-change", &eof_constraint, cmd);
        ValueArg<int> mem_arg("m", "mem", "Memory size", false, 1024, "Memory size", cmd);
        ValueArg<string> out_path_arg("o", "out", "Bytecode output path", false, "", "path", cmd);
        cmd.parse(argc, argv);
        path = path_arg.getValue();
        disassemble = disassemble_arg.getValue();
        dump_c = dump_c_arg.getValue();
        stat = stat_arg.getValue();
        unknown_fatal = unknown_fatal_arg.getValue();
        eof = eof_arg.getValue();
        out_path = out_path_arg.getValue();
        mem_size = mem_arg.getValue();
    }
    catch (ArgException &e) {
        cerr << "error (" << e.argId() << "): " << e.error() << endl;
        return BFI_ARG_ERROR;
    }
    bf_file_contents src = bf_read_file(path.c_str());
    if (!src.contents) {
        cerr << "Could not read from file: " << path << endl;
        return BFI_IO_ERROR;
    }
    bf_bytecode* bytecode;
    if (src.length >= 8 && strncmp((char*)src.contents, BF_BYTECODE_HEADER, 4) == 0) {
        uint32_t src_version;
        memcpy(&src_version, src.contents + 4, sizeof(src_version));
        if (src_version != bf_version) {
            cerr << "Incompatible bytecode version (expected " << bf_version << ", "
                << src_version << " found)" << endl;
            return BFI_BYTECODE_ERROR;
        }
        bytecode = new bf_bytecode(src.length - 8);
        memcpy(bytecode->contents, src.contents + 8, src.length - 8);
    }
    else {
        vector<bf_optimize_fn> funcs;
        funcs.push_back(bf_optimize_remove_duplicates);
        string src_contents((char*)src.contents, src.length);
        bytecode = bf_compile(src_contents, &funcs);
        if (!bytecode) {
            cerr << "Compile failed" << endl;
            return BFI_COMPILE_ERROR;
        }
    }
    if (stat) {
        cout << "File (including unrecognized characters): " << src.length << " bytes" << endl;
        cout << "Bytecode: " << bytecode->length << " bytes" << endl;
    }
    if (out_path.size()) {
        bf_file_contents out;
        out.contents = new uint8_t[bytecode->length + 8];
        memcpy(out.contents, BF_BYTECODE_HEADER, 4);
        memcpy(out.contents + 4, &bf_version, 4);
        memcpy(out.contents + 8, bytecode->contents, bytecode->length);
        out.length = bytecode->length + 8;
        bf_write_file(out_path.c_str(), out);
        return 0;
    }
    bf_vm vm = bf_vm(mem_size);
    vm.opts.eof_flag = (eof == "0") ? BF_EOF_0 : ((eof == "-1") ? BF_EOF_NEG1 : BF_EOF_NO_CHANGE);
    vm.opts.unknown_fatal = unknown_fatal;
    if (disassemble) {
        bf_disassemble(bytecode, stdout);
    }
    else if (dump_c)
        bf_dump_c(bytecode, stdout);
    else {
        bf_run(vm, bytecode);
    }
    delete bytecode;
    return 0;
}
