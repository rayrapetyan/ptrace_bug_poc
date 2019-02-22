#include "./traced_proc.h"

#include <errno.h>
#include <pthread_np.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <libprocstat.h>

#include "./helpers.h"

using namespace std;


int TracedProc::GetNumLwps() const {
    int ret;
    errno = 0;
    ret = ptrace(PT_GETNUMLWPS, pid_, 0, 0);
    if (errno != 0)
        throw "error TracedProc::GetNumLwps";
    return ret;
}

vector<lwpid_t> TracedProc::GetLwpList() {
    int num_lwps = GetNumLwps();
    lwpid_t tids[num_lwps];
    int num = 0;
    errno = 0;
    if ((num = ptrace(PT_GETLWPLIST, pid_, (caddr_t) &tids, num_lwps)) == -1 || errno != 0) {
        throw "error TracedProc::GetLwpList";
    }
    int i = 0;
    vector<lwpid_t> res;
    while (i < num) {
        res.push_back(tids[i]);
        ++i;
    }
    return res;
}

void *TracedProc::EntryPoint() {
    if (ep_)
        return ep_;

    bool found = false;

    struct procstat *ps = procstat_open_sysctl();
    if (ps == NULL)
        throw "procstat_open_sysctl";

    uint cnt = 0;
    struct kinfo_proc *kipp = procstat_getprocs(ps, KERN_PROC_PID, pid_, &cnt);
    if (cnt == 0)
        throw "procstat_getprocs";

    Elf_Auxinfo *auxv = NULL;
    while (auxv == NULL) {
        sleep(1);
        auxv = procstat_getauxv(ps, kipp, &cnt);
    }
    for (int i = 0; i < cnt; i++) {
        if (auxv[i].a_type == AT_ENTRY) {
            ep_ = auxv[i].a_un.a_ptr;
            found = true;
            break;
        }
    }
    procstat_freeauxv(ps, auxv);
    if (!found)
        throw "EP not found";
    return ep_;
}



void TracedProc::Step() const {
    errno = 0;
    ptrace(PT_STEP, pid_, (caddr_t) 1, 0);
    if (errno != 0)
        throw "error TracedProc::Step";
    /*int status;
    pid_t pid = wait4(pid_, &status, 0, 0);
    printf("TracedProc::Step res: status: %x, pid: %d, errno: %d\n", status, pid, errno);
    if (pid == -1) {
        throw "error wait";
    }*/
}

void TracedProc::AddBreakpoint(const Breakpoint& bp) {
    char buf[1];
    ReadMemory(bp.addr, buf, 1);
    Breakpoint bp_tmp(bp.addr, *buf);

    *buf = 0xCC;
    WriteMemory(bp_tmp.addr, buf, 1);

    breakpoints_[bp_tmp.addr] = bp_tmp;

    for (auto&& [tid, thread] : threads_) {
        thread.AddBreakpoint(bp_tmp);
    }
}

void TracedProc::RemoveBreakpoint(const Breakpoint& bp) {
    auto it = breakpoints_.find(bp.addr);
    Breakpoint bp_tmp = (*it).second;

    char buf[1] = {bp_tmp.orig_byte};
    WriteMemory(bp_tmp.addr, buf, 1);

    breakpoints_.erase(bp_tmp.addr);

    for (auto&& [tid, thread] : threads_) {
        thread.RemoveBreakpoint(bp_tmp);
    }
}

void TracedProc::ReadMemory(void *addr, void *buf, size_t len) const {
    return ::ReadMemory(pid_, addr, buf, len);
}

void TracedProc::WriteMemory(void *addr, void *buf, size_t len) const {
    return ::WriteMemory(pid_, addr, buf, len);
}

void TracedProc::Continue() {
    for (auto&& [tid, thread] : threads()) {
        // thread may exit while looping here
        if (threads_[tid].exited)
            continue;

        void *rip = thread.GetRip();

        auto it = thread.breakpoints().find(rip);
        if (it == thread.breakpoints().end())
            continue;

        auto bp = (*it).second;
        printf("Restoring orig opcode (tid: %d)\n", tid);
        ::WriteMemory(pid_, rip, (void *) &bp.orig_byte, 1);

        // StepInstruction
        thread.Step();
        // must call wait on process after thread step. Otherwise thread is shown as busy.
        // Moreover, wait should return signal from THIS thread, otherwise step won't happen

        while (true) {
            lwpid_t ttt = TrapWait();
            if (ttt == thread.id())
                break;

            errno = 0;
            if (ptrace(PT_CONTINUE, ttt, (caddr_t) 1, 0) == -1 || errno != 0) {
                throw "error PT_CONTINUE";
            }
        }


        printf("---new_rip: %p (tid: %d))\n", thread.GetRip(), tid);

        printf("Restoring int3 opcode (tid: %d)\n", tid);
        char buf[1] = {(char) 0xCC};
        ::WriteMemory(pid_, bp.addr, buf, 1);
    }
    errno = 0;
    if (ptrace(PT_CONTINUE, pid_, (caddr_t) 1, 0) == -1 || errno != 0) {
        throw "error PT_CONTINUE";
    }
}

lwpid_t TracedProc::TrapWait() {
    printf("TrapWait\n");
    lwpid_t tid;

    do {
        int status;
        pid_t pid = wait4(pid_, &status, 0, 0);

        printf("TrapWait res: status: %x, pid: %d, errno: %d\n", status, pid, errno);
        if (pid == -1) {
            throw "error wait";
        }

        if (WIFEXITED(status)) {
            printf("TrapWait: traced process exited, so we do\n");
            for (auto&& [tid, thread] : threads_) {
                if (tid == tid_) {
                    continue; // no bp hits expected for main thread
                }
                for (auto&& [addr, bp] : thread.breakpoints()) {
                    if (bp.hits != 2) {
                        printf("BOOM! Invalid BP hits counter (hits: %d, tid: %d)\n", bp.hits, tid);
                        exit(0);
                    }
                }
            }
            //break;
            printf("SUCCESS\n");
            exit(0);
        }

        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {

            struct ptrace_lwpinfo li;
            errno = 0;
            if (ptrace(PT_LWPINFO, pid, (caddr_t) &li, sizeof(li)) == -1 || errno != 0) {
                printf("errno: %d\n", errno);
                throw "error ptrace";
            }
            printf("PT_LWPINFO ret: pl_lwpid: %d, si_addr: %p, pl_flags: %d, si_code: %d\n", li.pl_lwpid, li.pl_siginfo.si_addr,
                   li.pl_flags, li.pl_siginfo.si_code);

            tid = li.pl_lwpid;
            if (li.pl_flags & PL_FLAG_BORN) {
                printf("TrapWait: new thread born: %d\n", tid);
                ThreadAdded(tid);
            } else if (li.pl_flags & PL_FLAG_EXITED) {
                printf("TrapWait: exiting thread: %d\n", tid);
                ThreadRemoved(tid);
                // we must send PT_CONTINUE below to this finishing thread
            } else if (li.pl_flags & PL_FLAG_SI) {
                if (li.pl_siginfo.si_code == TRAP_BRKPT) {
                    void *trap_addr = (char *) li.pl_siginfo.si_addr - 1;
                    printf("TrapWait: breakpoint hit at: %p (tid: %d)\n", trap_addr, tid);
                    thread(tid).SetRip(trap_addr);

                    threads_[tid].breakpoints()[trap_addr].record_hit();
                    printf("Hit recorded, new hits counter values: %d\n", threads_[tid].breakpoints()[trap_addr].hits);

                } else if (li.pl_siginfo.si_code == TRAP_TRACE) {
                    printf("TrapWait: trace hit at: %p (tid: %d)\n", (char *) li.pl_siginfo.si_addr, tid);
                } else {
                    throw "unkonwn si_code";
                }
                // else is a TRAP_TRACE
                break;
            } else {
                printf("TrapWait unknown pl_flags: 0x%x\n", li.pl_flags);
                break;
            }

            // continue lwp caused trap stop
            printf("!!!!!!!!Looping in TrapWait\n");
            errno = 0;
            if (ptrace(PT_CONTINUE, tid, (caddr_t) 1, 0) == -1 || errno != 0) {
                throw "error PT_CONTINUE";
            }
        }

    } while (true);
    return tid;
}
