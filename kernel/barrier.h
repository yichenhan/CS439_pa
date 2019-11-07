#ifndef _barrier_h_
#define _barrier_h_

#include "atomic.h"
#include "semaphore.h"

class Barrier {
    Atomic<int32_t> count;
    Semaphore sem;
public:
    Barrier(uint32_t count) : count(count),sem(0) {}
    
    void sync() {
        int x = count.add_fetch(-1);
        if (x < 0) Debug::panic("count went negative in barrier");
        if (x == 0) {
            sem.up();
        } else {
            sem.down();
            sem.up();
        }
    }
};
        
        
        

#endif

