#ifndef THREADSTATE_H
#define THREADSTATE_H

#include <vector>
#include <memory>
#include "vectorclock.h"
#include "xmap.h"
#include "xvector.h"
#include <atomic>
#include "tl_alloc.h"

namespace drace {
    namespace detector {
        class Fasttrack;
    }
}

///implements a threadstate, holds the thread's vectorclock and the thread's id (tid and act clock), as well as pointer to the fasttrack class
class ThreadState : public VectorClock<>{//tl_alloc<std::pair<const size_t, size_t>>> {
private:

    ///holds the tid and the actual clock value -> lower 32 bits are clock, upper 32 are the tid
    std::atomic<uint64_t> id;

    ///fasttrack* is needed to call the ft-cleanup function with the own tid, when destructing
    drace::detector::Fasttrack* ft = nullptr;

public:
       
    ///constructor of ThreadState object, initializes tid and clock, copies the vector of parent thread, if a parent thread exists
    ThreadState::ThreadState(   drace::detector::Fasttrack* ft_inst,
                                uint32_t own_tid, std::shared_ptr<ThreadState> parent = nullptr); 

    ThreadState::~ThreadState(); 


    ///increases own clock value
    void inc_vc();

    ///returns own clock value (upper bits) & thread (lower bits)
    size_t return_own_id() const;

    ///returns thread id
    uint32_t get_tid() const;

    ///returns current clock
    uint32_t get_clock() const;

    ///may be called after exitting of thread
    void delete_vector() {
        vc.clear();
    }


};

#endif // !THREADSTATE_H
