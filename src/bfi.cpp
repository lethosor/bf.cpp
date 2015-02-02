#include "bf.h"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::string;
using std::stringstream;

int main (int argc, const char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " path_to_program" << endl;
        return 1;
    }
    std::string src;
    if (!bf_read_file_contents(argv[1], src)) {
        cerr << "Could not read from file: " << argv[1] << endl;
        return 2;
    }
    bf_string bytecode = bf_compile(src);
    bf_vm vm = bf_vm(1024);
    vm.eof_flag = BF_EOF_NO_CHANGE;
    for (int i = 2; i < argc; i++) {
        string arg(argv[i]);
        if (arg.find("--eof=") != string::npos) {
            arg = arg.substr(arg.find("=") + 1);
            if (arg == "0")
                vm.eof_flag = BF_EOF_0;
            else if (arg == "-1")
                vm.eof_flag = BF_EOF_NEG1;
            else if (arg == "n" || arg == "nc" || arg == "no-change")
                vm.eof_flag = BF_EOF_NO_CHANGE;
            else {
                cerr << "--eof must be one of 0, -1, n|nc|no-change" << endl;
                return 1;
            }
        }
        else if (arg.find("--disassemble") != string::npos) {
            bf_disassemble(bytecode, cout);
            return 0;
        }
    }
    bf_run(vm, bytecode);
    return 0;
}
