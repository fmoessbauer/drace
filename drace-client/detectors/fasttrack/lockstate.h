#pragma once 
#include <map>
#include "vectorclock.h"



class LockState : public VectorClock {
public:
    
    LockState::LockState() {};

    /*LockState::LockState(uint32_t id, uint32_t clock) {
        vc.insert(vc.end(), std::pair<uint32_t, uint32_t>(id, clock));
    };*/
           
};

