#include <map>
#include "ftstates.h"

constexpr auto READ_SHARED = -2;
static constexpr int VAR_NOT_INIT = -1;
class VarState : FTStates {
public:
    uint64_t address;
    int w_tid;
    int r_tid;
    int w;
    /// local clock of last read
    int r;
    std::map<int, int> rvc;

    VarState::VarState(uint64_t addr) {
        address = addr;
        w = r = r_tid = w_tid = VAR_NOT_INIT;
    };

   
    void update(bool is_write, uint32_t vc, uint32_t other_tid) {
        if (is_write) {
            r = -1;
            w_tid = other_tid;
            w = vc;
        }
        else {
            r_tid = other_tid;
            if (r == READ_SHARED) {
                if (rvc.find(other_tid) == rvc.end()) {
                    rvc.insert(rvc.end(), std::pair<int, int>(other_tid, vc));
                }
                else {
                    rvc[other_tid] = vc;
                }
                
            }
            //w = 0;
            else {
                r = vc;
            }
        }
    };

    void set_read_shared() {
        r = READ_SHARED;
    }

    uint32_t get_length() {
        return rvc.size();
    }

    uint32_t get_thread_id_by_pos(uint32_t pos) {
        if (pos < rvc.size()) {
            std::map<int, int>::iterator it = rvc.begin();
            for (int i = 0; i < pos; ++i) {
                it++;
            }
            return it->first;
        }
        else {
            return -1;
        }
    }

    uint32_t get_vc_by_id(uint32_t tid) {
        if (rvc.find(tid) != rvc.end()) {
            return rvc[tid];
        }
        else {
            return 0;
        }
    }

};
