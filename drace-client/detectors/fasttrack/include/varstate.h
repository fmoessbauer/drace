#ifndef VARSTATE_H
#define VARSTATE_H

#include "vectorclock.h"
#include "xmap.h"
#include "threadstate.h"
#include <atomic>
//#include <memory>

class VarState  {

    ///pointer to thread which last wrote the var
    std::shared_ptr<ThreadState> w_tid;

    ///pointer to thread which last read the var, if not read_shared
    std::shared_ptr<ThreadState> r_tid;

    ///contains read_shared case all involved threads and clocks
    std::unique_ptr <xmap< std::shared_ptr<ThreadState>, size_t >> vc;

    bool read_shared = false;

public:
    //static constexpr int READ_SHARED = -2;
    static constexpr int VAR_NOT_INIT = 0;
    //static constexpr size_t bit_mask_64 = 0xFFFFFFFF00000000;//32 ones & 32 zeros
    //static constexpr size_t bit_mask_32 = 0xFFFF0000; //16 ones & 16 zeros

    ///var address
    const size_t address;

    /// var size
    const size_t size;

    /// the upper half of the bits are the thread id the lower half is the clock of the thread
    std::atomic<size_t> w_id;

    /// local clock of last read
    std::atomic<size_t> r_id;

    VarState::VarState(size_t addr, size_t var_size);


    ///evaluates for write/write races through this and and access through t
    bool is_ww_race(std::shared_ptr<ThreadState> t);

    ///evaluates for write/read races through this and and access through t
    bool is_wr_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-exclusive/write races through this and and access through t
    bool is_rw_ex_race(std::shared_ptr<ThreadState> t);

    ///evaluates for read-shared/write races through this and and access through t
    std::shared_ptr <ThreadState> is_rw_sh_race(std::shared_ptr<ThreadState> t);

    size_t get_write_id() const;

    size_t get_read_id() const;

    ///return tid of thread which last wrote this var
    size_t get_w_tid() const;

    ///return tid of thread which last read this var, if not read shared
    size_t get_r_tid() const;

    size_t get_w_clock() const;

    size_t get_r_clock() const;

    bool is_read_shared() const;

    ///updates the var state because of an new read or write access through an thread
    void update(bool is_write, std::shared_ptr<ThreadState> thread);

    ///sets read state to shared
    void set_read_shared(std::shared_ptr<ThreadState> thread);

    ///if in read_shared state, then returns thread id of position pos in vector clock
    std::shared_ptr<ThreadState> get_thr(uint32_t pos) const;

    ///return stored clock value, which belongs to ThreadState t, 0 if not available
    size_t get_vc_by_thr(std::shared_ptr<ThreadState> t) const;

    size_t get_clock_by_thr(std::shared_ptr<ThreadState> t) const;


};
#endif // !VARSTATE_H
