#ifndef _DR_LOCK_H
#define _DR_LOCK_H


#include<ipc/spinlock.h>




#define _DR_API_ true
#define _STD_LOCK_ false

#if _DR_API_
#define _STD_LOCK_ false
#endif

#undef min
#undef max

#if _DR_API_
#include "dr_api.h"
#endif

//implement  shared_timed_mutex if
class DrLock {
#if _DR_API_
private:
    void * lock;


public:

    DrLock::DrLock():lock(dr_rwlock_create()) {}

    DrLock::~DrLock() {
        dr_rwlock_destroy(lock);
    }

    void read_lock() {
        dr_rwlock_read_lock(lock);
    }

    void read_unlock(){
        dr_rwlock_read_unlock(lock);
    }

    void write_lock(){
        dr_rwlock_write_lock(lock);
    }

    void write_unlock(){
        dr_rwlock_write_unlock(lock);
    }

    void write_trylock(){
        dr_rwlock_write_trylock(lock);
    }
#endif // _DR_API_

#if _STD_LOCK_
private:
    ipc::spinlock lock;

public:
    DrLock::DrLock(){}

    DrLock::~DrLock() {    }

    void read_lock() {
        lock.lock();
    }

    void read_unlock() {
        lock.unlock();
    }

    void write_lock() {
        lock.lock();
    }

    void write_unlock() {
        lock.unlock();
    }

    void write_trylock() {
       
    }
#endif // _STD_LOCK_
};


#endif // !_DR_LOCK_H
