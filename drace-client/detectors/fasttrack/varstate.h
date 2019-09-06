#include "vectorclock.h"
#include "xmap.h"


class VarState  {

    ///pointer to thread which last wrote the var
    std::shared_ptr<ThreadState> w_tid;

    ///pointer to thread which last read the var, if not read_shared
    std::shared_ptr<ThreadState> r_tid;

    ///contains read_shared case all involved threads and clocks


    xmap<std::shared_ptr<ThreadState>, uint32_t>* vc = nullptr;
   

public:
    static constexpr int READ_SHARED = -2;
    static constexpr int VAR_NOT_INIT = -1;

    ///var address
    uint64_t address;

    /// var size
    uint32_t size;

    /// local clock of last write
    int32_t w_clock;

    /// local clock of last read
    int32_t r_clock;

    VarState::VarState(uint64_t addr, uint32_t var_size) {
        address = addr;
        size = var_size;
        w_clock = r_clock = VAR_NOT_INIT;
        r_tid = w_tid = nullptr;
    };

    VarState::~VarState() {
        delete vc;
    }

    ///evaluates for write/write races through this and and access through t
    bool is_ww_race(std::shared_ptr<ThreadState> t) {
        if (    t->tid  != get_w_tid() &&
                w_clock >= t->get_vc_by_id(get_w_tid()) ) {
            return true;
        }
        return false;
    };

    ///evaluates for write/read races through this and and access through t
    bool is_wr_race(std::shared_ptr<ThreadState> t) {
        if (    w_clock     != VAR_NOT_INIT &&
                get_w_tid() != t->tid       &&
                w_clock     >= t->get_vc_by_id(get_w_tid()) )
        {
            return true;
        }
        return false;
    };

    ///evaluates for read-exclusive/write races through this and and access through t
    bool is_rw_ex_race(std::shared_ptr<ThreadState> t) {
      
        if (    r_clock != VAR_NOT_INIT  &&
                t->tid  != get_r_tid()   &&
                r_clock >= t->get_vc_by_id(get_r_tid()) )// read-write race
        {
            return true;
        }
        return false;
    };

    ///evaluates for read-shared/write races through this and and access through t
    std::shared_ptr <ThreadState> is_rw_sh_race(std::shared_ptr<ThreadState> t) {
        for (int i = 0; i < vc->size(); ++i) {
            std::shared_ptr<ThreadState> act_thr = get_thr(i);

            if (t->tid != act_thr->tid &&
               get_vc_by_id(act_thr) >= t->get_vc_by_id(act_thr->tid))
            {
                return act_thr;
            }
        }
        return nullptr;
    }

    ///return tid of thread which last wrote this var
    int32_t get_w_tid() {
        if (w_tid == nullptr) {
            return VAR_NOT_INIT;
        }
        return w_tid->tid;
    }

    ///return tid of thread which last read this var, if not read shared
    int32_t get_r_tid() {
        if (r_tid == nullptr) {
            return VAR_NOT_INIT;
        }
        return r_tid->tid;
    }

    ///updates the var state because of an new read or write access through an thread
    void update(bool is_write, std::shared_ptr<ThreadState> thread) {
        if (is_write) {
            r_clock = VAR_NOT_INIT;
            r_tid = nullptr;
            delete vc;
            vc = nullptr;
            w_tid = thread;
            w_clock = thread->get_self();
        }
        else {
            if (r_clock == READ_SHARED) {
                if (vc->find(thread) == vc->end()) {
                    vc->insert(vc->end(), { thread, thread->get_self() });
                }
                else {
                    (*vc)[thread] = thread->get_self();
                }
            }
            else {
                r_tid = thread;
                r_clock = thread->get_self();
            }
        }
    };

    ///sets read state to shared
    void set_read_shared(std::shared_ptr<ThreadState> thread) {
        vc = new xmap<std::shared_ptr<ThreadState>, uint32_t>;

        
        vc->insert(vc->end(), { r_tid, r_clock });
        vc->insert(vc->end(), { thread, thread->get_self() });

        r_clock = READ_SHARED;
        r_tid = nullptr;
    }

    ///if in read_shared state, then returns thread id of position pos in vector clock
    std::shared_ptr<ThreadState> get_thr(uint32_t pos) {
        if (pos < vc->size()) {
            auto it = vc->begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return nullptr;
        }
    }

    ///return stored clock value, which belongs to ThreadState t, 0 if not available
    uint32_t get_vc_by_id(std::shared_ptr<ThreadState> t) {
        if (vc->find(t) != vc->end()) {
            return (*vc)[t];
        }
        return 0;
    }

};
