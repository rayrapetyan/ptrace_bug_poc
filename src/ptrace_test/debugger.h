#ifndef PTRACE_TEST_DEBUGGER_H
#define PTRACE_TEST_DEBUGGER_H

#include <string>
#include <unistd.h>
#include <vector>

#include "./traced_proc.h"

class Debugger {
public:
    Debugger() {}

    ~Debugger() {}

    void Launch(const std::string &path);
    void Attach(pid_t pid);

    void ReadMemory(void *addr, void *buf, size_t len);
    void WriteMemory(void *addr, void *buf, size_t len);

    void AddBreakpoint(void *addr);
    void RemoveBreakpoint(void *addr);

    void Continue();
    void Next();

    TracedProc &traced_proc() const {
        return *traced_proc_;
    }

private:
    void Init();
    TracedProc *traced_proc_;
};

#endif //PTRACE_TEST_DEBUGGER_H
