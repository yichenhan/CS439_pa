#ifndef _bb_h_
#define _bb_h_

#include "semaphore.h"

template <int N, typename T>
class BoundedBuffer {
    int head;
    int n;
    Semaphore nEmpty;
    Semaphore nFull;
    Semaphore lock;
    T data[N];
public:
    BoundedBuffer() : head(0), n(0), nEmpty(N), nFull(0), lock(1) {}

    void put(T t) {
        nEmpty.down();
        lock.down();
        data[(head + n) % N] = t;
        n++;
        lock.up();
        nFull.up();
    }

    T get() {
        T out;
        nFull.down();
        lock.down();
        out = data[head];
        head = (head + 1) % N;
        n--;
        lock.up();
        nEmpty.up();
        return out;
    }
        
};

#endif
