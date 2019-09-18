#ifndef _X_LOCK_H
#define _X_LOCK_H


#include "ipc/spinlock.h"
#include <shared_mutex>




//implement  shared_timed_mutex if
class xlock {
private:
    ipc::spinlock this_lock;

public:

    inline void lock_shared() {
        this_lock.lock();
    }

    inline bool try_lock_shared() {
        //no dynamrio api function available
        return this_lock.try_lock();
    }

    inline void unlock_shared() {
        this_lock.unlock();
    }

    inline void lock() {
        this_lock.lock();
    }

    inline void unlock() {
        this_lock.unlock();
    }

    inline bool try_lock() {
        return this_lock.try_lock();
    }

};


#endif // !_DR_LOCK_H
