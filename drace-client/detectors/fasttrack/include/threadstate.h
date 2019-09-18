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


class ThreadState : public VectorClock<>{//tl_alloc<std::pair<const size_t, size_t>>> {
private:
    std::atomic<uint64_t> id;
    drace::detector::Fasttrack* ft = nullptr;

public:
    ///own thread id
       
    ///constructor of ThreadState object, initializes tid and clock, copies the vector of parent thread, if a parent thread exists
    ThreadState::ThreadState(   drace::detector::Fasttrack* ft_inst,
                                uint32_t own_tid, std::shared_ptr<ThreadState> parent = nullptr); 

    ThreadState::~ThreadState(); 


    ///increases own clock value
    void inc_vc();

    ///returns own clock value (upper bits) & thread (lower bits)
    size_t return_own_id() const;

    uint32_t get_tid() const;

    uint32_t get_clock() const;

    ///may be called after exitting of thread
    void delete_vector() {
        vc.clear();
    }


};

#endif // !THREADSTATE_H
