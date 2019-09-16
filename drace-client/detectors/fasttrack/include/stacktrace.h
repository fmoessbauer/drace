#include "xvector.h"
#include "xmap.h"
#include <ipc/DrLock.h>

class StackTrace {
    xvector<size_t> global_stack;
    xmap<size_t,  std::pair<size_t, size_t>> read_write;


    xvector<size_t> make_trace(std::pair<size_t, size_t> data) {
        xvector<size_t> this_stack;
        this_stack.push_back(data.first);
        return this_stack;

        /* len = data.second;
        xvector<size_t> this_stack(global_stack.begin(), (global_stack.begin() + len));
        while (unsigned int pos = contains_zero(this_stack)) {
            this_stack.erase(this_stack.begin() + pos);
            this_stack.erase(this_stack.begin() + pos - 1);
        }
        this_stack.push_back(data.first);
        return this_stack;*/
    }

    unsigned int contains_zero(xvector<size_t> vec) {

        for (auto it = vec.begin(); it != vec.end(); it++) {
            if (*it == 0) {
                return std::distance(vec.begin(), it);
            }
        }
        return 0;
    }

public:

    void pop_stack_element() {

        global_stack.push_back(0);

    }

    void push_stack_element(size_t element) {
        global_stack.push_back(element);
    }

    ///when a var is written or read, it copies the stack and adds the pc of the
    ///r/w operation to be able to return the stack trace if a race was detected
    void set_read_write(size_t addr, size_t pc) {
        uint32_t len = global_stack.size();

        if (read_write.find(addr) == read_write.end()) {
            read_write.insert(read_write.end(), { addr, {pc, len} });
        }
        else {
            read_write.find(addr)->second = { pc, len };
        }

    }

    ///returns a stack trace of a clock for handing it over to drace
    xvector<size_t> return_stack_trace(size_t address) {

        auto it = read_write.find(address);
        if (it != read_write.end()) {
            auto data = it->second;
            xvector<size_t> t = make_trace(data);
            return t;
        }
        else {
            //A read/write operation was not tracked correctly -> return empty stack trace
            return  xvector<size_t>(0);
        }
        
    }
};
