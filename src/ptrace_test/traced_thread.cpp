#include "./traced_thread.h"

#include <errno.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "./helpers.h"

void TracedThread::AddBreakpoint(const Breakpoint& bp) {
    breakpoints_[bp.addr] = bp;
}

void TracedThread::RemoveBreakpoint(const Breakpoint& bp) {
    breakpoints_.erase(bp.addr);
}

void TracedThread::Step() const {
    printf("TracedThread::Step(cur_rip: %p) (tid: %d)\n", GetRip(), id_);
    errno = 0;
    ptrace(PT_STEP, id_, (caddr_t) 1, 0);
    if (errno != 0)
        throw "error PT_STEP";
/*
    int status;
    pid_t pid = wait4(pid_, &status, 0, 0);
    printf("TrapWait res: status: %x, pid: %d, errno: %d\n", status, pid, errno);
    if (pid == -1) {
        throw "error wait";
    }

    struct ptrace_lwpinfo li;
    errno = 0;
    if (ptrace(PT_LWPINFO, pid, (caddr_t) &li, sizeof(li)) == -1 || errno != 0)
        throw "error ptrace";
    printf("PT_LWPINFO ret: pl_lwpid: %d, si_addr: %p, pl_flags: %d, si_code: %d\n", li.pl_lwpid, li.pl_siginfo.si_addr,
            li.pl_flags, li.pl_siginfo.si_code);
*/
}

reg TracedThread::GetRegs() const {
    reg regs;
    errno = 0;
    ptrace(PT_GETREGS, id_, (caddr_t) &regs, 0);
    if (errno != 0)
        throw "error PT_GETREGS";
    return regs;
}

void TracedThread::SetRegs(const reg& regs) const {
    errno = 0;
    ptrace(PT_SETREGS, id_, (caddr_t) &regs, 0);
    if (errno != 0)
        throw "error PT_SETREGS";
}

void * TracedThread::GetRip() const {
    reg regs = GetRegs();
    return (void *)regs.r_rip;
}

void TracedThread::SetRip(void *addr) const {
    reg regs = GetRegs();
    regs.r_rip = (int64_t)addr;
    SetRegs(regs);
}
