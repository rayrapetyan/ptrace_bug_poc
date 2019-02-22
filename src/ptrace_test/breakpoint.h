#ifndef PTRACE_TEST_BREAKPOINT_H
#define PTRACE_TEST_BREAKPOINT_H

class Breakpoint {
public:
    Breakpoint() : addr(nullptr), orig_byte(0), hits(0) {}
    explicit Breakpoint(void *addr) : addr(addr), orig_byte(0), hits(0) {}
    Breakpoint(void *addr, char orig_byte) : addr(addr), orig_byte(orig_byte), hits(0) {}
    void record_hit() {
        ++hits;
    }
    void *addr;
    char orig_byte;
    int hits;
};

#endif //PTRACE_TEST_BREAKPOINT_H
