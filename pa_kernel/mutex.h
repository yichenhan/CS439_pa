#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "semaphore.h"

class Mutex : Semaphore {
public:
    Mutex() : Semaphore(1) {}
    void lock(void) { down(); }
    void unlock(void) { up(); }
};

class Mutexer {
	Mutex& m;
public:
	inline Mutexer(Mutex& m) : m(m) { m.lock(); }
	inline ~Mutexer() { m.unlock(); }
};

#endif
