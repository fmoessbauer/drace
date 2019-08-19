#pragma once 
#include <map>
#include "ftstates.h"



class LockState : public FTStates {
public:
    std::map<int, int> lock_vc; //thread id vector clocks
    LockState::LockState() {};

    LockState::LockState(int id, int vc) {
        lock_vc.insert(lock_vc.end(), std::pair<int, int>(id, vc));
    };

    void update(uint32_t id, uint32_t vc) {
        if (lock_vc.find(id) == lock_vc.end()) {
            lock_vc.insert(lock_vc.end(), std::pair<int, int>(id, vc));
        }
        else {
            lock_vc[id] = vc;
        }
    };

    uint32_t get_vc_by_id(uint32_t tid) {
        if (lock_vc.find(tid) != lock_vc.end()) {
            return lock_vc[tid];
        }
        else {
            return 0;
        }
    }    

    uint32_t get_thread_id_by_pos(uint32_t pos) {
        if (pos < lock_vc.size()) {
            std::map<int, int>::iterator it = lock_vc.begin();
            for (int i = 0; i < pos; ++i) {
                it++;
            }
            return it->first;
        }
        else {
            return -1;
        }
    }

    uint32_t get_length() {
        return lock_vc.size();
    }


};

