#include <string>
#include <map>
#include <vector>
#include "vectorclock.h"


class ThreadState : public VectorClock{
private:
    ///if a stack trace for a clock value is not existing it will be created
    ///otherwise it just returns
    void make_stack_trace(uint32_t clock) {
        /// return pointer to stack trace element of clock 
        //int i = clock;
        for (int i = clock; i >= 0; --i) {
            if (stack_trace.find(i) != stack_trace.end()) {
                if (i == clock) {
                    //list is already existing
                    return;
                }
                else {
                    //list is copied from the nearest older one
                    auto new_list = std::pair<int, std::vector<uint64_t>>(clock, stack_trace[i]);
                    stack_trace.insert(stack_trace.end(), new_list);
                    return;
                }
            }
            if (i == 0) {
                //no list exists, create the first empty list
                std::vector<uint64_t> vec;
                auto empty_list = std::pair<uint32_t, std::vector<uint64_t>>(clock, vec);
                stack_trace.insert(stack_trace.end(), empty_list);
                return;
            }
        }
    };

    ///returns a copy of the vector at time of clock
    /// if clock = -1 -> latest value
    std::vector<uint64_t> copy_stack_trace(int32_t clock) {
        std::vector<uint64_t> ret_stack_trace;
        //int i = clock;
        if (clock != -1) {
            for (int i = clock; i >= 0; --i) {
                if (stack_trace.find(i) != stack_trace.end()) {
                    ret_stack_trace = stack_trace[i];
                    break;
                }
            }
        }
        else {
            auto it = stack_trace.rbegin();
            ret_stack_trace = it->second;
        }

        return ret_stack_trace;
    }

public:
    ///own thread id
    uint32_t tid;
        
    ///contains a stack trace for each clock value
    ///each stack entry consists of a pc-value(uint64) and flag to indicate
    ///if the entry belongs to a read or write bool==true->r or w, bool==false->function addr
    std::map<uint32_t, std::vector<uint64_t>> stack_trace;

    ///contains the program counters for the read/write operation of a clock, if there was one
    std::map<uint64_t, uint64_t> read_write_pc;
    std::map<uint64_t, uint64_t> allocation_pc;

    ThreadState::ThreadState(uint32_t own_tid, ThreadState* parent = nullptr) {
        tid = own_tid;
        uint32_t clock = 0;
        vc.insert(vc.end(), std::pair<uint32_t, uint32_t>(tid, clock));
        if (parent != nullptr) {
            //if parent exists vector clock
            vc = parent->vc;
        }
    };

       

    void pop_stack_element() {
        uint32_t clock = get_self();
        make_stack_trace(clock);
        stack_trace[clock].pop_back();
    }

    void push_stack_element(uint64_t element) {
        int clock = get_self();
        make_stack_trace(clock);
        stack_trace[clock].push_back(element);
    }

    void set_allocation(uint64_t addr, uint64_t pc) {
        
        if (allocation_pc.find(addr) == allocation_pc.end()) {
            allocation_pc.insert(allocation_pc.end(), { addr, pc });
        }
        else {
            allocation_pc[addr] = pc;
        }
    }

    void set_read_write(uint64_t addr, uint64_t pc) {
        if (read_write_pc.find(addr) == read_write_pc.end()) {
            read_write_pc.insert(read_write_pc.end(), { addr, pc });
        }
        else {
            read_write_pc[addr] = pc;
        }
    }

    void inc_vc() {
        vc[tid]++;
    }


    uint32_t get_self() {
        return vc[tid];
    };


    
    ///returns a stack trace of a clock for handing it over to drace
    std::vector<uint64_t> return_stack_trace(uint32_t clock, uint64_t address, bool is_alloc) {
        std::vector<uint64_t> ret = copy_stack_trace(clock);

        if (!is_alloc){
            if (read_write_pc.find(address) != read_write_pc.end()) {
                uint64_t rw = read_write_pc[address];
                ret.push_back(rw);
            }
            else {
                std::cerr << "A read/write operation was not tracked correctly" << std::endl;
            }
        }
        else {
            if (allocation_pc.find(address) != allocation_pc.end()) {
                uint64_t al = allocation_pc[address];
                ret.push_back(al);
            }
            else {
                std::cerr << "An allocation operation was not tracked correctly" << std::endl;
            }
        }
        return ret;        
    }

};
