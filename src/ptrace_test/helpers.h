#ifndef PTRACE_TEST_HELPERS_H
#define PTRACE_TEST_HELPERS_H

#include <sys/types.h>

void ReadMemory(pid_t pid, void *addr, void *buf, size_t len);
void WriteMemory(pid_t pid, void *addr, void *buf, size_t len);

#endif //PTRACE_TEST_HELPERS_H
