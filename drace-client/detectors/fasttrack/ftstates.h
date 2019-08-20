#ifndef FTSTATES_H
#define FTSTATES_H

class FTStates {

public:
    //virtual void update(uint32_t tid, uint32_t vc) = 0;
    virtual uint32_t get_vc_by_id(uint32_t tid) = 0;
    virtual uint32_t get_id_by_pos(uint32_t pos) = 0;
    virtual uint32_t get_length() = 0;
};

#endif
