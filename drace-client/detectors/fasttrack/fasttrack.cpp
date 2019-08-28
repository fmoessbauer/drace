
#include <mutex> // for lock_guard
#include <iostream>
#include <detector/detector_if.h>
#include <ipc/spinlock.h>
#include "threadstate.h"
#include "varstate.h"
#include "xmap.h"
#include "xvector.h"



#define MAKE_OUTPUT false
#define REGARD_ALLOCS true


namespace fasttrack {
/// globals ///
    static constexpr int max_stack_size = 16;

    xmap<size_t, size_t> allocs;
    xmap<size_t, VarState*> vars;
    xmap<void*, VectorClock*> locks;
    xmap<detector::tid_t, std::shared_ptr<ThreadState>> threads;
    xmap<void*, VectorClock*> happens_states;
    void * clb;

    //declaring order is also the acquiring order of the locks
    static ipc::spinlock t_lock;
    static ipc::spinlock v_lock;
    static ipc::spinlock l_lock;
    static ipc::spinlock h_lock;
    static ipc::spinlock a_lock;

    void report_race(   unsigned thr1,  unsigned thr2,
                        bool wr1,       bool wr2,
                        size_t var,
                        int clk1,       int clk2    ) {


        uint32_t var_size = 0;

        VarState* var_object = vars[var];
        var_size = var_object->size;
     
        std::shared_ptr<ThreadState> thread1 = threads[thr1];
        std::shared_ptr<ThreadState> thread2 = threads[thr2];
        xvector<size_t> stack1, stack2;

        stack1 = thread1->return_stack_trace(var);
        stack2 = thread2->return_stack_trace(var);

        if (stack1.size() > 16) {
            auto end_stack  = stack1.end();
            auto begin_stack = stack1.end() - 16;

            stack1 = xvector<size_t>(begin_stack, end_stack);
        }
        if (stack2.size() > 16) {
            auto end_stack = stack2.end();
            auto begin_stack = stack2.end() - 16;

            stack2 = xvector<size_t>(begin_stack, end_stack);
        }

        detector::AccessEntry access1;
        access1.thread_id = thr1;
        access1.write = wr1;
        access1.accessed_memory = var;
        access1.access_size = var_size;
        access1.access_type = 0;
        access1.heap_block_begin = 0;
        access1.heap_block_size = 0;
        access1.onheap = false;
        access1.stack_size = stack1.size();
        std::copy(stack1.begin(), stack1.end(), access1.stack_trace);

        detector::AccessEntry access2;
        access2.thread_id = thr2;
        access2.write = wr2;
        access2.accessed_memory = var;
        access2.access_size = var_size;
        access2.access_type = 0;
        access2.heap_block_begin = 0;
        access2.heap_block_size = 0;
        access2.onheap = false;
        access2.stack_size = stack2.size();
        std::copy(stack2.begin(), stack2.end(), access2.stack_trace);


        detector::Race race;
        race.first = access1;
        race.second = access2;


        ((void(*)(const detector::Race*))clb)(&race);

    }


    void read(std::shared_ptr<ThreadState> t, VarState* v) {
       //cannot happen in current implementation as rw increments clock, discuss
        if (t->get_self() == v->r_clock && t->tid == v->get_r_tid()) {//read same epoch, sane thread
            return;
        }

        if (v->r_clock == VarState::READ_SHARED && v->get_vc_by_id(t) == t->get_self() && v->get_vc_by_id(t) != 0) { //read shared same epoch
            return;
        }

        if (v->is_wr_race(t))
        { // write-read race
            report_race(v->get_w_tid(), t->tid, true, false, v->address, v->w_clock, t->get_self());
        }

        //update vc
        if (v->r_clock != VarState::READ_SHARED)
        {
            if (v->r_clock == VarState::VAR_NOT_INIT ||
                (v->get_r_tid() == t->tid     &&
                    v->r_clock < t->get_vc_by_id(v->get_r_tid())))//read exclusive
            {
                v->update(false, t);
            }
            else { // read gets shared
                v->set_read_shared(t);
            }
        }
        else {//read shared
            v->update(false, t);
        }
        
    }

    void write(std::shared_ptr<ThreadState> t, VarState* v) {

        if (v->w_clock == VarState::VAR_NOT_INIT) { //initial write, update var
            v->update(true, t);
            return;
        }

        // not possib with current impl
        if (t->get_self() == v->w_clock &&
            t->tid == v->get_w_tid())
        {//write same epoch
            return;
        }

        //tids are different && and write epoch greater or equal than known epoch of other thread
        if (v->is_ww_race(t)) // write-write race
        {
            report_race(v->get_w_tid(), t->tid, true, true, v->address, v->w_clock, t->get_self());
        }

        if (v->r_clock != VarState::READ_SHARED) {
            if (v->is_rw_ex_race(t))// read-write race
            {
                report_race(v->get_r_tid(), t->tid, false, true, v->address, v->r_clock, t->get_self());
            }
        }
        else {//come here in read shared case
            std::shared_ptr<ThreadState> act_thr = v->is_rw_sh_race(t);
            if (act_thr != nullptr) //read shared read-write race       
            {
                report_race(act_thr->tid, t->tid, false, true, v->address, v->get_vc_by_id(act_thr), t->get_self());
            }
        }
        v->update(true, t);
    }

    void create_var(size_t addr, size_t size) {
        VarState* var = new VarState(addr, size);
        v_lock.lock();
        vars.insert(vars.end(), std::pair<size_t, VarState*>(addr, var));
        v_lock.unlock();
    }

    void create_lock(void* mutex) {
        VectorClock* lock = new VectorClock();
        std::lock_guard<ipc::spinlock>lg_v(v_lock);
        locks.insert(locks.end(), std::pair<void*, VectorClock*>(mutex, lock));
    }

    ///the deleter for a ThreadStates Object
    ///if it is deleted, also the tid is removed of all other vector clocks which hold the tid
    void thread_state_deleter(ThreadState* ptr) {
        uint32_t tid = ptr->tid;

        //as ThreadState is destroyed delete all the entries from all vectorclocks
        l_lock.lock();
        for (auto it = locks.begin(); it != locks.end(); ++it) {
            it->second->delete_vc(tid);
        }
        l_lock.unlock();
        t_lock.lock();
        for (auto it = threads.begin(); it != threads.end(); ++it) {
            it->second->delete_vc(tid);
        }
        t_lock.unlock();
        h_lock.lock();
        for (auto it = happens_states.begin(); it != happens_states.end(); ++it) {
            it->second->delete_vc(tid);
        }
        h_lock.unlock();
        delete ptr;
    }
    
    void create_thread(detector::tid_t tid, std::shared_ptr<ThreadState> parent = nullptr) {
        std::shared_ptr<ThreadState> new_thread;

        if (parent == nullptr) {
            new_thread = std::shared_ptr<ThreadState>(new ThreadState(tid), thread_state_deleter);
        }
        else {
            std::lock_guard<ipc::spinlock>lg_t(t_lock);
            new_thread = std::shared_ptr<ThreadState>(new ThreadState(tid, parent), thread_state_deleter);
        }
        std::lock_guard<ipc::spinlock>lg_t(t_lock);
        threads.insert(threads.end(), { tid, new_thread });
    }


    void create_happens(void* identifier) {
        VectorClock * new_hp = new VectorClock();
        std::lock_guard<ipc::spinlock>lg_h(h_lock);
        happens_states.insert(happens_states.end(), std::pair<void*, VectorClock*>(identifier, new_hp));
    }

    void create_alloc(size_t addr, size_t size) {
        std::lock_guard<ipc::spinlock>lg_a(a_lock);
        allocs.insert(allocs.end(), { addr, size});
    }

}

bool detector::init(int argc, const char **argv, Callback rc_clb) {
    fasttrack::clb = rc_clb; //init callback
    return true;
}

void detector::finalize() {
    
    while (fasttrack::vars.size() > 0) {
        auto it = fasttrack::vars.begin();
        delete it->second;
        fasttrack::vars.erase(it);
    }
    while (fasttrack::locks.size() > 0) {
        auto it = fasttrack::locks.begin();
        delete it->second;
        fasttrack::locks.erase(it);
    }
    while (fasttrack::threads.size() > 0) {
        fasttrack::threads.erase(fasttrack::threads.begin());
    }
    while (fasttrack::happens_states.size() > 0) {
        auto it = fasttrack::happens_states.begin();
        delete it->second;
        fasttrack::happens_states.erase(it);
    }
}


void detector::read(tls_t tls, void* pc, void* addr, size_t size) {
    size_t var_address = reinterpret_cast<size_t>(addr);

    if (fasttrack::vars.find(var_address) == fasttrack::vars.end()) { //create new variable if new
    #if MAKE_OUTPUT
        std::cout << "variable is read before written" << std::endl;//warning
    #endif
        fasttrack::create_var(var_address, size);
    }

    auto thr = fasttrack::threads[reinterpret_cast<detector::tid_t>(tls)];

    std::lock_guard<ipc::spinlock>lg_t (fasttrack::t_lock);
    {
        std::lock_guard<ipc::spinlock>lg_v (fasttrack::v_lock);

        thr->set_read_write(var_address, reinterpret_cast<size_t>(pc));
        fasttrack::read(thr, fasttrack::vars[var_address]);
    }
}

void detector::write(tls_t tls, void* pc, void* addr, size_t size) {
    size_t var_address = ((size_t)addr);

    if (fasttrack::vars.find(var_address) == fasttrack::vars.end()) {
        fasttrack::create_var(var_address, size);
    }
    auto thr = fasttrack::threads[reinterpret_cast<detector::tid_t>(tls)];

    std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
    {
        std::lock_guard<ipc::spinlock>lg_v(fasttrack::v_lock);

        thr->set_read_write(var_address, reinterpret_cast<size_t>(pc));
        fasttrack::write(thr, fasttrack::vars[var_address]);
    }
}


void detector::func_enter(tls_t tls, void* pc) {
    std::shared_ptr<ThreadState> thr = fasttrack::threads[reinterpret_cast<detector::tid_t>(tls)];
    size_t stack_element = reinterpret_cast<size_t>(pc);

    std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
    thr->push_stack_element(stack_element);

}

void detector::func_exit(tls_t tls) {
    std::shared_ptr<ThreadState> thr = fasttrack::threads[reinterpret_cast<detector::tid_t>(tls)];

    std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
    thr->pop_stack_element(); //pops last stack element of current clock
}


void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
    *tls = reinterpret_cast<void*>(child);
        
    if (fasttrack::threads.find(parent) != fasttrack::threads.end()) {
        fasttrack::t_lock.lock();
        fasttrack::threads[parent]->inc_vc(); //inc vector clock for creation of new thread
        fasttrack::t_lock.unlock();
        fasttrack::create_thread(child, fasttrack::threads[parent]);
    }
    else {
        fasttrack::create_thread(child);
    }
    
}

void detector::join(tid_t parent, tid_t child) {
    std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
    std::shared_ptr<ThreadState> del_thread = fasttrack::threads[child];
    del_thread->inc_vc();

    //pass incremented clock of deleted thread to parent
    auto ptr = del_thread.get();
    fasttrack::threads[parent]->update(ptr);

    fasttrack::threads.erase(child);
}


void detector::acquire(tls_t tls, void* mutex, int recursive, bool write) {
        
    if (fasttrack::locks.find(mutex) == fasttrack::locks.end()) {
        fasttrack::create_lock(mutex);
    }

    auto id = reinterpret_cast<detector::tid_t>(tls);

    std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
    {
        std::lock_guard<ipc::spinlock>lg_l(fasttrack::l_lock);
        //propagate sync data from lock to thread
        (fasttrack::threads[id])->update(fasttrack::locks[mutex]);
    }

}

void detector::release(tls_t tls, void* mutex, bool write) {
 
    if (fasttrack::locks.find(mutex) == fasttrack::locks.end()) {
        #if MAKE_OUTPUT
        std::cerr << "lock is released but was never acquired by any thread" << std::endl;
        #endif
        fasttrack::create_lock(mutex);
    }

    auto id = reinterpret_cast<detector::tid_t>(tls);

    std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
    {
        std::lock_guard<ipc::spinlock>lg_l(fasttrack::l_lock);
        //increase vector clock and propagate to lock
        fasttrack::threads[id]->inc_vc();
        auto ptr = (fasttrack::threads[id]).get();
        (fasttrack::locks[mutex])->update(ptr);
    }
}


void detector::happens_before(tls_t tls, void* identifier) {
    if (fasttrack::happens_states.find(identifier) == fasttrack::happens_states.end()) {
        fasttrack::create_happens(identifier);
    }

    VectorClock* hp = fasttrack::happens_states[identifier];
    std::shared_ptr<ThreadState> thr = fasttrack::threads[reinterpret_cast<detector::tid_t>(tls)];

    std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
    {
        std::lock_guard<ipc::spinlock>lg_h(fasttrack::h_lock);
        //increment clock of thread and update happens state
        thr->inc_vc();
        hp->update(thr->tid, thr->get_self());
    }
}

/** Draw a happens-after edge between thread and identifier (optional) */
void detector::happens_after(tls_t tls, void* identifier) {
    
    if (fasttrack::happens_states.find(identifier) != fasttrack::happens_states.end()) {
        VectorClock* hp = fasttrack::happens_states[identifier];
        std::shared_ptr<ThreadState> thr = fasttrack::threads[reinterpret_cast<detector::tid_t>(tls)];

        std::lock_guard<ipc::spinlock>lg_t(fasttrack::t_lock);
        {
            std::lock_guard<ipc::spinlock>lg_h(fasttrack::h_lock);
            //update vector clock of thread with happened before clocks
            thr->update(hp);
        }

    }
    else {//create -> no happens_before can be synced
        fasttrack::create_happens(identifier);
    }

    
}


void detector::allocate(
    /// ptr to thread-local storage of calling thread
    tls_t  tls,
    /// current instruction pointer
    void*  pc,
    /// begin of allocated memory block
    void*  addr,
    /// size of memory block
    size_t size
){
#if REGARD_ALLOCS
    size_t address = reinterpret_cast<size_t>(addr);
    if (fasttrack::allocs.find(address) == fasttrack::allocs.end()) {
        fasttrack::create_alloc(address, size);
    }
#endif
}

/** Log a memory deallocation*/
void detector::deallocate(
    /// ptr to thread-local storage of calling thread
    tls_t tls,
    /// begin of memory block
    void* addr
){
#if REGARD_ALLOCS
    size_t address = reinterpret_cast<size_t>(addr);
    size_t size = fasttrack::allocs[address];
    size_t end_addr = address + size;

    //variable is deallocated so varstate objects can be destroyed
    while (address < end_addr) {
        if (fasttrack::vars.find(address) != fasttrack::vars.end()) {
            std::lock_guard<ipc::spinlock> lg_v(fasttrack::v_lock);
            {
                VarState* var = fasttrack::vars.find(address)->second;
                size_t var_size = var->size;
                address += var_size;
                delete var;
                fasttrack::vars.erase(address);
            }
        }
        else {
            address++;
        }
    }
    std::lock_guard<ipc::spinlock> lg_a(fasttrack::a_lock);
    fasttrack::allocs.erase(address);

#endif  
}



//TODO
void detector::detach(tls_t tls, tid_t thread_id){}
/** Log a thread exit event (detached thread) */
void detector::finish(tls_t tls, tid_t thread_id){}



const char * detector::name() {
    return "FASTTRACK";
}

const char * detector::version() {
    return "0.0.1";
}


