#ifndef PTRACE_TEST_TRACED_PROC_H
#define PTRACE_TEST_TRACED_PROC_H

#include <sys/cdefs.h>
#include <machine/reg.h>
#include <map>
#include <string>
#include <unistd.h>
#include <pthread_np.h>

#include "./traced_thread.h"

class TracedProc {
public:
    TracedProc(const std::string &path, const pid_t pid) :
        path_(path), pid_(pid), ep_(nullptr) {}

    pid_t pid() const {
        return pid_;
    }

    std::map<lwpid_t, TracedThread> threads() const {
        return threads_;
    }

    int GetNumLwps() const;
    std::vector<lwpid_t> GetLwpList();

    void ThreadAdded(lwpid_t tid) {
        TracedThread th(pid_, tid);
        for (auto&& [_, bp] : breakpoints_) {
            th.AddBreakpoint(bp);
        }
        threads_[tid] = th;
    }

    void ThreadRemoved(lwpid_t tid) {
        printf("Setting %d.exited to true\n", tid);
        threads_[tid].exited = true;
    }

    TracedThread& thread(const lwpid_t tid) {
        auto it = threads_.find(tid);
        if (it == threads_.end())
            throw "thread";
        return (*it).second;
    }

    void set_tid(lwpid_t tid) {
        tid_ = tid;
    }

    void *EntryPoint();

    lwpid_t TrapWait();

    void AddBreakpoint(const Breakpoint &bp);
    void RemoveBreakpoint(const Breakpoint &bp);

    void ReadMemory(void *addr, void *buf, size_t len) const;
    void WriteMemory(void *addr, void *buf, size_t len) const;

    void Continue();
    void Step() const;

private:
    std::string path_;
    pid_t pid_;
    void *ep_;
    lwpid_t tid_;
    std::map<lwpid_t, TracedThread> threads_;
    std::map<void *, Breakpoint> breakpoints_;
};

#endif //PTRACE_TEST_TRACED_PROC_H
