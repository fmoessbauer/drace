#include <string>
#include <map>
#include <vector>
#include "ftstates.h"


class ThreadState : public FTStates{
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

    ///thread id, clock
    std::map<uint32_t, uint32_t> thread_vc;

    ///contains a stack trace for each clock value
    ///each stack entry consists of a pc-value(uint64) and flag to indicate
    ///if the entry belongs to a read or write bool==true->r or w, bool==false->function addr
    std::map<uint32_t, std::vector<uint64_t>> stack_trace;

    ///contains the program counters for the read/write operation of a clock, if there was one
    std::map<uint32_t, uint64_t> read_write_pc;

    ThreadState::ThreadState(uint32_t own_tid, ThreadState* parent = nullptr) {
        tid = own_tid;
        uint32_t clock = 0;
        thread_vc.insert(thread_vc.end(), std::pair<uint32_t, uint32_t>(tid, clock));
        if (parent != nullptr) {
            //if parent exists copy stack_trace and vector clock
            stack_trace = parent->stack_trace;
            thread_vc = parent->thread_vc;
        }
    };

    void update(uint32_t thread_id, uint32_t vc) {
        
        auto it = thread_vc.find(thread_id);
        if (it != thread_vc.end()) {
            thread_vc[thread_id] = vc;
        }
        else {
            thread_vc.insert(thread_vc.end(), std::pair<int, int>(thread_id, vc));
        }

    };

    void update(ThreadState* other) {
        for (auto it = other->thread_vc.begin(); it != other->thread_vc.end(); it++) {
            if (it->second > get_vc_by_id(it->first)) {
                update(it->first, it->second);
            }
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

    void set_read_write(uint64_t element) {
        uint32_t clock = get_self();
        if (read_write_pc.find(clock) == read_write_pc.end()) {
            read_write_pc.insert(read_write_pc.end(), std::pair<uint32_t, uint64_t>(clock, element));
        }
        else {
            std::cerr << "Error two read/write operations at same clock" << std::endl;
        }
    }

    void inc_vc() {
        thread_vc[tid]++;
    }

    uint32_t get_self() {
        return thread_vc[tid];
    };

    uint32_t get_vc_by_id(uint32_t other_tid) {
        auto it = thread_vc.find(other_tid);
        if (it == thread_vc.end()){
            //if vector clock is not yet known initialize with 0
            update(other_tid, 0);
        }
        return thread_vc[other_tid];
    };

    uint32_t get_length() {
        return thread_vc.size();
    };

    uint32_t get_id_by_pos(uint32_t pos) {
        if (pos < thread_vc.size()) {
            auto it = thread_vc.begin();
            std::advance(it, pos);
            return it->first;
        }
        else {
            return 0;
        }
    };
    
    ///returns a stack trace of a clock for handing it over to drace
    std::vector<uint64_t> return_stack_trace(int32_t clock){
        std::vector<uint64_t> ret = copy_stack_trace(clock);
       
        if (read_write_pc.find(clock) != read_write_pc.end()) {
            uint64_t rw = read_write_pc[clock];
            ret.push_back(rw);
        }
        else {
            std::cerr << "Happend read/write was not tracked correctly" << std::endl;
        }
        return ret;        
    }

};
