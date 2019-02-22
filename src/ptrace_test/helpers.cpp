#include "./helpers.h"

#include <errno.h>
#include <stdio.h>
#include <sys/ptrace.h>

void ReadMemory(pid_t pid, void *addr, void *buf, size_t len) {
    printf("\tReadMemory addr: %p\n", addr);
    struct ptrace_io_desc ioreq = {PIOD_READ_I, addr, buf, len};
    errno = 0;
    ptrace(PT_IO, pid, (caddr_t)&ioreq, 0);
    if (errno != 0)
        throw "error PT_IO";
}

void WriteMemory(pid_t pid, void *addr, void *buf, size_t len) {
    printf("\tWriteMemory addr: %p\n", addr);
    struct ptrace_io_desc ioreq = {PIOD_WRITE_I, addr, buf, len};
    errno = 0;
    ptrace(PT_IO, pid, (caddr_t)&ioreq, 0);
    if (errno != 0) {
        printf("%d\n", errno);
        throw "error PT_IO";
    }

    char buf_tmp[1];
    ReadMemory(pid, addr, buf_tmp, len);
    if (buf_tmp[0] != *(char*)buf) {
        throw "error WriteMemory";
    } else {
        printf("-Written byte: %x at: %p\n", buf_tmp[0] & 0xff, addr);
    }
}