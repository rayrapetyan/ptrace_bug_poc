#include <iostream>
#include <unistd.h>

#include "./debugger.h"


int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: ptrace_test app_path bp_addr\n");
        exit(0);
    }

    Debugger d;

    //d.Launch("/tmp/bpcountstest.ff7d33ce");
    //d.AddBreakpoint(d.traced_proc().EntryPoint());
    //d.AddBreakpoint((void *)0x4878b7);

    d.Launch(argv[1]);
    d.AddBreakpoint((void *)std::stol(argv[2], nullptr, 16));

    for (int i = 0; i < 1000; ++i) {
        d.Continue();
    }
    return 0;
}