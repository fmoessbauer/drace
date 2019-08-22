#include <stdint.h>
static constexpr uint32_t NOT_DEALLOC = UINT32_MAX;

class AllocationState {

    uint32_t owner_tid;
    uint32_t dealloc_clock;
    uint64_t address;
    uint32_t size;
    uint64_t alloc_pc;

    bool is_overlapping(AllocationState* other) {
        uint64_t other_addr = other->address;
        uint32_t other_size = other->size;

        if (other_addr == address) {
            return true;
        }
        if (other_addr > address) {
            if (other_addr >= (address + size)) {
                return false;
            }
            else {
                return true;
            }
        }
        else {//other_addr < address
            if ((other_addr + other_size) <= address) {
                return false;
            }
            else {
                return true;
            }
        }
    };

public:
    AllocationState::AllocationState(uint64_t addr, uint32_t mem_size, uint32_t tid, uint64_t pc) {
        size            = mem_size;
        address         = addr;
        alloc_pc        = pc;
        owner_tid       = tid;
        dealloc_clock   = NOT_DEALLOC;
    }

    

    void alloc() {
        dealloc_clock = NOT_DEALLOC;
    };

    void dealloc(uint32_t clock) {
        dealloc_clock = clock;
    }

    bool is_race(AllocationState* other, uint32_t current_clock) {
        //not deallocated memory is uint_max and therefore leads always to race
        if (other->owner_tid != owner_tid &&
            current_clock <= dealloc_clock &&
            is_overlapping(other))
        {
            return true;
        }
        return false;
    }

    uint32_t get_owner_tid() {
        return owner_tid;
    };
    uint64_t get_addr() {
        return address;
    }
    uint32_t get_dealloc_clock() {
        return dealloc_clock;
    }
};
