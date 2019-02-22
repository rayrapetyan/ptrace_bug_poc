#include "./debugger.h"

#include <errno.h>
#include <pthread_np.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libprocstat.h>

using namespace std;

void Debugger::Next() {
    printf("Debugger::Next()\n");
    traced_proc().Step();
    traced_proc().TrapWait();
}

void Debugger::Continue() {
    printf("\n\n\n\n\nSTAGE 1: proc Continue (single steps all BP-ed threads) and call PT_CONTINUE\n");
    traced_proc().Continue();

    printf("\nSTAGE 2: TrapWait\n");
    traced_proc().TrapWait();
}

void Debugger::WriteMemory(void *addr, void *buf, size_t len) {
    traced_proc().WriteMemory(addr, buf, len);
}

void Debugger::ReadMemory(void *addr, void *buf, size_t len) {
    traced_proc().ReadMemory(addr, buf, len);
}

void Debugger::AddBreakpoint(void *addr) {
    printf("Debugger::AddBreakpoint addr: %p\n", addr);
    traced_proc().AddBreakpoint(Breakpoint(addr));

}

void Debugger::RemoveBreakpoint(void *addr) {
    printf("Debugger::RemoveBreakpoint addr: %p\n", addr);
    traced_proc().RemoveBreakpoint(Breakpoint(addr));
}

void Debugger::Init() {
    printf("Debugger::Init\n");

    int status;
    pid_t pid = wait4(traced_proc().pid(), &status, 0, 0);
    printf("Debugger::wait4 res: status: %x, pid: %d, errno: %d\n", status, pid, errno);
    if (pid == -1) {
        throw "error wait";
    }

    vector<lwpid_t> traced_threads = traced_proc().GetLwpList();
    printf("traced proc main thread: %d\n", traced_threads[0]);

    traced_proc().ThreadAdded(traced_threads[0]);
    traced_proc().set_tid(traced_threads[0]);

    // intercept LWP events in traced proc
    errno = 0;
    if (ptrace(PT_LWP_EVENTS, pid, 0, 1) == -1 || errno != 0) {
        throw "error ptrace";
    }
}

void Debugger::Launch(const std::string &path) {
    printf("Debugger::Launch %s\n", path.c_str());
    printf("debugger pid: %d, tid: %d\n", getpid(), pthread_getthreadid_np());
    pid_t pid = fork();
    if (pid == -1) {
        throw "error fork";
    } else if (pid == 0) {
        pid_t child_pid = getpid();
        int child_tid = pthread_getthreadid_np();
        printf("tracee pid: %d, tid: %d\n", child_pid, child_tid);

        ptrace(PT_TRACE_ME, 0, 0, 0);

        char *exec_argv[] = {NULL};
        char *exec_environ[] = {NULL};
        // execve replaces traced process with a new executable
        if (execve(path.c_str(), exec_argv, exec_environ) == -1) {
            throw "error execve";
        }
        printf("you'll never see this\n");
    } else {
        traced_proc_ = new TracedProc(path, pid);
    }
    Init();
}

void Debugger::Attach(pid_t pid) {
    ptrace(PT_ATTACH, pid, 0, 0);
    traced_proc_ = new TracedProc("", pid);
    Init();
}
