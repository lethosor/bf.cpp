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

int main (int argc, const char** argv) {
    using namespace TCLAP;
    bool disassemble = false;
    int mem_size;
    string path, eof;
    try {
        CmdLine cmd("Brainfuck interpreter", ' ', "0.1");
        UnlabeledValueArg<string> path_arg("path", "Path to file", true, "", "Path to file", cmd);
        SwitchArg disassemble_arg("", "disassemble", "Dump bytecode", cmd, false);
        vector<string> eof_flags;
        eof_flags.push_back("0");
        eof_flags.push_back("-1");
        eof_flags.push_back("n");
        eof_flags.push_back("nc");
        eof_flags.push_back("no-change");
        ValuesConstraint<string> eof_constraint(eof_flags);
        ValueArg<string> eof_arg("", "eof", "EOF behavior", false, "no-change", &eof_constraint, cmd);
        ValueArg<int> mem_arg("m", "mem", "Memory size", false, 1024, "Memory size", cmd);
        cmd.parse(argc, argv);
        path = path_arg.getValue();
        disassemble = disassemble_arg.getValue();
        eof = eof_arg.getValue();
        mem_size = mem_arg.getValue();
    }
    catch (ArgException &e) {
        cerr << "error (" << e.argId() << "): " << e.error() << endl;
        return 2;
    }
    char* src = bf_read_file_contents(path.c_str());
    if (!src) {
        cerr << "Could not read from file: " << path << endl;
        return 1;
    }
    bf_bytecode* bytecode = bf_compile(src);
    if (!bytecode) {
        cerr << "Compile failed" << endl;
        return 1;
    }
    bf_vm vm = bf_vm(mem_size);
    vm.eof_flag = (eof == "0") ? BF_EOF_0 : ((eof == "-1") ? BF_EOF_NEG1 : BF_EOF_NO_CHANGE);
    if (disassemble) {
        bf_disassemble(bytecode, stdout);
    }
    else {
        bf_run(vm, bytecode);
    }
    delete bytecode;
    return 0;
}
