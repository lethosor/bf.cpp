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
    //bf_disassemble(bytecode, cout);
    bf_run(vm, bytecode);
    return 0;
}
