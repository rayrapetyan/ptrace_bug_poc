#ifndef PTRACE_TEST_TRACED_THREAD_H
#define PTRACE_TEST_TRACED_THREAD_H

#include <map>
#include <sys/types.h>
#include <x86/reg.h>

#include "./breakpoint.h"

class TracedThread {
public:
    TracedThread() {}

    explicit TracedThread(const pid_t pid, const lwpid_t id) : exited(false), pid_(pid), id_(id) {}

    void AddBreakpoint(const Breakpoint& bp);
    void RemoveBreakpoint(const Breakpoint& bp);

    void Step() const;

    reg GetRegs() const;
    void SetRegs(const reg& regs) const;

    void* GetRip() const;
    void SetRip(void *addr) const;

    lwpid_t id() const {
        return id_;
    }

    std::map<void *, Breakpoint>& breakpoints() {
        return breakpoints_;
    }

    bool exited;
private:
    pid_t pid_;
    lwpid_t id_;
    std::map<void *, Breakpoint> breakpoints_;
};

#endif //PTRACE_TEST_TRACED_THREAD_H
