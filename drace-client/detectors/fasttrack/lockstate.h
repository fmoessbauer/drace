#pragma once 
#include <map>
#include "ftstates.h"



class LockState : public FTStates {
public:
    std::map<uint32_t, uint32_t> lock_vc; //thread id vector clocks
    LockState::LockState() {};

    LockState::LockState(uint32_t id, uint32_t vc) {
        lock_vc.insert(lock_vc.end(), std::pair<uint32_t, uint32_t>(id, vc));
    };

    void update(uint32_t id, uint32_t vc) {
        if (lock_vc.find(id) == lock_vc.end()) {
            lock_vc.insert(lock_vc.end(), std::pair<uint32_t, uint32_t>(id, vc));
        }
        else {
            lock_vc[id] = vc;
        }
    };

    void delete_vc(uint32_t tid) {
        if(lock_vc.find(tid) != lock_vc.end())
            lock_vc.erase(tid);
    }

    uint32_t get_vc_by_id(uint32_t tid) {
        if (lock_vc.find(tid) != lock_vc.end()) {
            return lock_vc[tid];
        }
        else {
            return 0;
        }
    }    

    uint32_t get_id_by_pos(uint32_t pos) {
        if (pos < lock_vc.size()) {
            auto it = lock_vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return 0;
        }
    }

    uint32_t get_length() {
        return lock_vc.size();
    }

    
};

