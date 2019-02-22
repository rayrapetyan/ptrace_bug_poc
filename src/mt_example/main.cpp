#include <iostream>
#include <thread>

#include <pthread_np.h>

static const int num_threads = 2;

void foo() {
    for (int i = 0; i < 2; ++i) {
        printf("hi: %d (tid: %d)\n", i, pthread_getthreadid_np());
    }
}

int main() {
    std::thread t[num_threads];

    for (int i = 0; i < num_threads; ++i) {
        t[i] = std::thread(foo);
    }

    for (int i = 0; i < num_threads; ++i) {
        t[i].join();
    }

    return 0;
}
