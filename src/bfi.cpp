#include "bf.h"

using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::string;
using std::stringstream;

int main() {
    stringstream ss;
    string line;
    while (getline(cin, line))
        ss << line << endl;
    bf_string bytecode = bf_compile(ss.str());
    bf_vm vm = bf_vm(1024);
    bf_run(vm, bytecode);
    return 0;
}
