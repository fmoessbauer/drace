#include <map>
#include "vectorclock.h"
#include "xmap.h"

class VarState  {

public:
    static constexpr int READ_SHARED = -2;
    static constexpr int VAR_NOT_INIT = -1;

    uint64_t address;
    uint32_t size;
    std::shared_ptr<ThreadState> w_tid;
    std::shared_ptr<ThreadState> r_tid;
    int32_t w_clock;
    /// local clock of last read
    int32_t r_clock;
    xmap<std::shared_ptr<ThreadState>, uint32_t> vc;

    VarState::VarState(uint64_t addr, uint32_t var_size) {
        address = addr;
        size = var_size;
        w_clock = r_clock = VAR_NOT_INIT;
        r_tid = w_tid = nullptr;
    };

    bool is_ww_race(std::shared_ptr<ThreadState> t) {
        if (t->tid != get_w_tid() &&
            w_clock >= t->get_vc_by_id(get_w_tid())) {
            return true;
        }
        return false;
    };

    bool is_wr_race(std::shared_ptr<ThreadState> t) {
        if (w_clock != VAR_NOT_INIT &&
            get_w_tid() != t->tid      &&
            w_clock >= t->get_vc_by_id(get_w_tid())) {
            return true;
        }
        return false;
    };

    bool is_rw_ex_race(std::shared_ptr<ThreadState> t) {
      
        if (r_clock != VAR_NOT_INIT &&
            t->tid != get_r_tid()      &&
            r_clock >= t->get_vc_by_id(get_r_tid()))// read-write race
        {
            return true;
        }
        return false;
    };

    std::shared_ptr <ThreadState> is_rw_sh_race(std::shared_ptr<ThreadState> t) {
        for (int i = 0; i < vc.size(); ++i) {
            auto act_thr = get_thr(i);

            if (t->tid != act_thr->tid &&
               vc[act_thr] >= t->get_vc_by_id(act_thr->tid))
            {
                return act_thr;
            }
        }
        return nullptr;
    }
    
    int32_t get_w_tid() {
        if (w_tid == nullptr) {
            return VAR_NOT_INIT;
        }
        return w_tid->tid;
    }

    int32_t get_r_tid() {
        if (r_tid == nullptr) {
            return VAR_NOT_INIT;
        }
        return r_tid->tid;
    }



    void update(bool is_write, std::shared_ptr<ThreadState> thread) {
        if (is_write) {
            r_clock = VAR_NOT_INIT;
            r_tid = nullptr;
            vc.clear();
            w_tid = thread;
            w_clock = thread->get_self();
        }
        else {
            if (r_clock == READ_SHARED) {
                if (vc.find(thread) == vc.end()) {
                    vc.insert(vc.end(), { thread, thread->get_self() });
                }
                else {
                    vc[thread] = thread->get_self();
                }
            }
            else {
                r_tid = thread;
                r_clock = thread->get_self();
            }
        }
    };

    void set_read_shared(std::shared_ptr<ThreadState> thread) {
        vc.insert(vc.end(), { r_tid, r_clock });
        vc.insert(vc.end(), { thread, thread->get_self() });

        r_clock = READ_SHARED;
        r_tid = nullptr;
    }


    std::shared_ptr<ThreadState> get_thr(uint32_t pos) {
        if (pos < vc.size()) {
            auto it = vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return nullptr;
        }
    }



};
