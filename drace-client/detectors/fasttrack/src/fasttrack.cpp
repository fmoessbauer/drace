/*
*
* - All create-functions are threadsafe
*
*
*
*
*
*
*/

#include "fasttrack.h"


#define MAKE_OUTPUT false
#define REGARD_ALLOCS true

namespace drace {

    namespace detector {

        //function is not thread safe!
        void Fasttrack::report_race(
            Fasttrack::tid_ft thr1, Fasttrack::tid_ft thr2,
            bool wr1, bool wr2,
            size_t var,
            uint32_t clk1, uint32_t clk2)
        {
            size_t var_size = 0;

            std::shared_ptr<VarState> var_object = vars[var];

            v_lock.read_lock();
            var_size = var_object->size;
            v_lock.read_unlock();

            s_lock.write_lock();
            std::shared_ptr<StackTrace> s1 = traces[thr1];
            std::shared_ptr<StackTrace> s2 = traces[thr2];
            xvector<size_t> stack1, stack2;

            stack1 = s1->return_stack_trace(var);
            stack2 = s2->return_stack_trace(var);
            s_lock.write_unlock();

            if (stack1.size() > 16) {
                auto end_stack = stack1.end();
                auto begin_stack = stack1.end() - 16;

                stack1 = xvector<size_t>(begin_stack, end_stack);
            }
            if (stack2.size() > 16) {
                auto end_stack = stack2.end();
                auto begin_stack = stack2.end() - 16;

                stack2 = xvector<size_t>(begin_stack, end_stack);
            }

            Detector::AccessEntry access1;
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

            Detector::AccessEntry access2;
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


            Detector::Race race;
            race.first = access1;
            race.second = access2;


            ((void(*)(const Detector::Race*))clb)(&race);
        }


        ///function is thread_safe
        void Fasttrack::read(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v) {
            //cannot happen in current implementation as rw increments clock, discuss


            if (t->return_own_id() == v->get_read_id()) {//read same epoch, same thread;
                return;
            }

            t_lock.read_lock();            
            //check!!!! change double look-up!
            if (v->is_read_shared()  && v->get_vc_by_thr(t) == t->return_own_id()) { //read shared same epoch
                t_lock.read_unlock();
                return;
            }

            if (v->is_wr_race(t))
            { // write-read race
                report_race(v->get_w_tid(), t->get_tid(), true, false, v->address, v->get_w_clock(), t->get_clock());
            }

            //update vc
            if (!v->is_read_shared())
            {
                if (v->get_read_id() == VarState::VAR_NOT_INIT ||
                   (v->get_r_tid() == t->get_tid()     
                   /*&& v->get_read_id() < t->return_own_id()*/ )) //read exclusive->read of same thread but newer epoch
                {
                    v_lock.write_lock();
                    v->update(false, t);
                }
                else { // read gets shared
                    v_lock.write_lock();
                    v->set_read_shared(t);
                }
            }
            else {//read shared
                v_lock.write_lock();
                v->update(false, t);
            }
            v_lock.write_unlock();
            t_lock.read_unlock();
        }

        ///function is thread safe
        void Fasttrack::write(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v) {

            if (t->return_own_id() == v->get_write_id()){//write same epoch
                return;
            }

            t_lock.read_lock();
            if (v->get_write_id() == VarState::VAR_NOT_INIT) { //initial write, update var
                v_lock.write_lock();
                v->update(true, t);
                v_lock.write_unlock();
                t_lock.read_unlock();
                return;
            }

            //tids are different && and write epoch greater or equal than known epoch of other thread
            if (v->is_ww_race(t)) // write-write race
            {
                report_race(v->get_w_tid(), t->get_tid(), true, true, v->address, v->get_w_clock(), t->get_clock());
            }

            if (! v->is_read_shared()) {
                if (v->is_rw_ex_race(t))// read-write race
                {
                    report_race(v->get_r_tid(), t->get_tid(), false, true, v->address, v->get_r_clock(), t->get_clock());
                }
            }
            else {//come here in read shared case
                std::shared_ptr<ThreadState> act_thr = v->is_rw_sh_race(t);
                if (act_thr != nullptr) //read shared read-write race       
                {
                    report_race(act_thr->get_tid(), t->get_tid(), false, true, v->address, v->get_clock_by_thr(act_thr), t->get_clock());
                }
            }
            v_lock.write_lock();
            v->update(true, t);
            v_lock.write_unlock();
            t_lock.read_unlock();
        }


        void Fasttrack::create_var(size_t addr, size_t size) {
            auto var = std::make_shared<VarState>(addr, size);
            vars.insert(vars.end(), { addr, var });
        }

        void Fasttrack::create_lock(void* mutex) {
           auto lock = std::make_shared<VectorClock>();
           locks.insert(locks.end(), { mutex, lock });
        }

        void Fasttrack::create_thread(Detector::tid_t tid, std::shared_ptr<ThreadState> parent) {

            std::shared_ptr<ThreadState> new_thread;
            if (parent == nullptr) {
                new_thread = std::make_shared<ThreadState>(this, tid);
            }
            else {
                t_lock.read_lock();
                new_thread = std::make_shared<ThreadState>(this, tid, parent);
                t_lock.read_unlock();
            }
            threads.insert(threads.end(), { tid, new_thread });

            //creaate also a object to save the stack traces
            traces.insert(traces.end(), { tid, std::make_shared<StackTrace>() });

        }

        void Fasttrack::create_happens(void* identifier) {
            auto new_hp = std::make_shared<VectorClock>();
            happens_states.insert(happens_states.end(), { identifier, new_hp });
        }

        void Fasttrack::create_alloc(size_t addr, size_t size) {
            allocs.insert(allocs.end(), { addr, size });
        }


        void Fasttrack::cleanup(size_t tid) {
            //as ThreadState is destroyed delete all the entries from all vectorclocks
            t_lock.write_lock();
            for (auto it = locks.begin(); it != locks.end(); ++it) {
                it->second->delete_vc(tid);
            }
            t_lock.write_unlock();

            t_lock.write_lock();
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                it->second->delete_vc(tid);
            }
            t_lock.write_unlock();

            t_lock.write_lock();
            for (auto it = happens_states.begin(); it != happens_states.end(); ++it) {
                it->second->delete_vc(tid);
            }
            t_lock.write_unlock();
        }

        bool Fasttrack::init(int argc, const char **argv, Callback rc_clb) {
            clb = rc_clb; //init callback
            return true;
        }

        void Fasttrack::finalize() {
            vars.clear();
            locks.clear();
            happens_states.clear();
            allocs.clear();
            traces.clear();
            while (threads.size() != 0) {
                threads.erase(threads.begin());
            }
        }


        void Fasttrack::read(tls_t tls, void* pc, void* addr, size_t size) {
            size_t var_address = reinterpret_cast<size_t>(addr);

            if (vars.find(var_address) == vars.end()) { //create new variable if new
#if MAKE_OUTPUT
                std::cout << "variable is read before written" << std::endl;//warning
#endif
                create_var(var_address, size);
            }
            //s_lock.write_lock();
            auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            stack->set_read_write(var_address, reinterpret_cast<size_t>(pc));
            //s_lock.write_unlock();

            auto thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            auto var = vars[var_address];
    
            read(thr, var);
            
        }

        void Fasttrack::write(tls_t tls, void* pc, void* addr, size_t size) {
            size_t var_address = ((size_t)addr);

            if (vars.find(var_address) == vars.end()) {
                create_var(var_address, size);
            }
            //s_lock.write_lock();
            auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            stack->set_read_write(var_address, reinterpret_cast<size_t>(pc));
            //s_lock.write_unlock();

            auto thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            auto var = vars[var_address];


            write(thr, var); //func is thread_safe     
        }


        void Fasttrack::func_enter(tls_t tls, void* pc) {

            auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            size_t stack_element = reinterpret_cast<size_t>(pc);

            s_lock.write_lock();
            stack->push_stack_element(stack_element);
            s_lock.write_unlock();
        }

        void Fasttrack::func_exit(tls_t tls) {
            auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            s_lock.write_lock();
            stack->pop_stack_element(); //pops last stack element of current clock
            s_lock.write_unlock();
        }


        void Fasttrack::fork(tid_t parent, tid_t child, tls_t * tls) {
            Fasttrack::tid_ft child_size_t = child;
            *tls = reinterpret_cast<void*>(child_size_t);

            if (threads.find(parent) != threads.end()) {
                t_lock.write_lock();
                threads[parent]->inc_vc(); //inc vector clock for creation of new thread
                t_lock.write_unlock();
                create_thread(child, threads[parent]);
            }
            else {
                create_thread(child);
            }

        }

        void Fasttrack::join(tid_t parent, tid_t child) {
            std::shared_ptr<ThreadState> del_thread = threads[child];

            t_lock.write_lock();
            del_thread->inc_vc();

            //pass incremented clock of deleted thread to parent
            auto ptr = del_thread;
            threads[parent]->update(ptr);
            t_lock.write_unlock();
            traces.erase(child);
            threads.erase(child);
        }


        void Fasttrack::acquire(tls_t tls, void* mutex, int recursive, bool write) {

            if (locks.find(mutex) == locks.end()) {
                create_lock(mutex);
            }
            auto id = reinterpret_cast<Fasttrack::tid_ft>(tls);
            auto thr = threads.find(id);
            auto lock = locks.find(mutex);

            t_lock.write_lock();
            
            (thr->second)->update(lock->second);
            
            t_lock.write_unlock();
        }

        void Fasttrack::release(tls_t tls, void* mutex, bool write) {

            if (locks.find(mutex) == locks.end()) {
#if MAKE_OUTPUT
                std::cerr << "lock is released but was never acquired by any thread" << std::endl;
#endif
                create_lock(mutex);
                return;//as lock is empty (was never acquired), we can return here
            }

            auto id = reinterpret_cast<Fasttrack::tid_ft>(tls);
            auto thr = threads.find(id);
            auto lock = locks.find(mutex);

            t_lock.write_lock();
            (thr->second)->inc_vc();
            
             //increase vector clock and propagate to lock    
            (lock->second)->update((thr->second));
            t_lock.write_unlock();

        }


        void Fasttrack::happens_before(tls_t tls, void* identifier) {
            if (happens_states.find(identifier) == happens_states.end()) {
                create_happens(identifier);
            }

            std::shared_ptr<VectorClock> hp = happens_states[identifier];
            std::shared_ptr<ThreadState> thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];

            t_lock.write_lock();
            (thr)->inc_vc();//increment clock of thread and update happens state

            hp->update(thr->get_tid(), thr->return_own_id());
            t_lock.write_unlock();

            
        }

        /** Draw a happens-after edge between thread and identifier (optional) */
        void Fasttrack::happens_after(tls_t tls, void* identifier) {

            if (happens_states.find(identifier) != happens_states.end()) {
                std::shared_ptr<VectorClock> hp = happens_states[identifier];
                std::shared_ptr<ThreadState> thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];

                t_lock.write_lock();
                //update vector clock of thread with happened before clocks
                thr->update(hp);
                t_lock.write_unlock();
            }
            else {//create -> no happens_before can be synced
                create_happens(identifier);
            }


        }


        void Fasttrack::allocate(
            /// ptr to thread-local storage of calling thread
            tls_t  tls,
            /// current instruction pointer
            void*  pc,
            /// begin of allocated memory block
            void*  addr,
            /// size of memory block
            size_t size
        ) {
#if REGARD_ALLOCS
            size_t address = reinterpret_cast<size_t>(addr);
            if (allocs.find(address) == allocs.end()) {
                create_alloc(address, size);
            }
#endif
        }

        /** Log a memory deallocation*/
        void Fasttrack::deallocate(
            /// ptr to thread-local storage of calling thread
            tls_t tls,
            /// begin of memory block
            void* addr
        ) {
#if REGARD_ALLOCS
            size_t address = reinterpret_cast<size_t>(addr);
            size_t size = allocs[address];
            size_t end_addr = address + size;

            //variable is deallocated so varstate objects can be destroyed
            while (address < end_addr) {
                if (vars.find(address) != vars.end()) {
                    std::shared_ptr<VarState> var = vars.find(address)->second;

                    t_lock.read_lock();
                    size_t var_size = var->size;
                    t_lock.read_unlock();
                    address += var_size;

                    vars.erase(address);
                }
                else {
                    address++;
                }
            }
            allocs.erase(address);
#endif  
        }


        void Fasttrack::detach(tls_t tls, tid_t thread_id) {
            /// \todo This is currently not supported
            return;
        }
        /** Log a thread exit event of a detached thread) */
        void Fasttrack::finish(tls_t tls, tid_t thread_id) {
            ///just delete thread from list, no backward sync needed
            threads.erase(thread_id);
        }

        void Fasttrack::map_shadow(void* startaddr, size_t size_in_bytes) {
            return;
        }


        const char * Fasttrack::name() {
            return "FASTTRACK";
        }

        const char * Fasttrack::version() {
            return "0.0.1";
        }
    }

}
extern "C" __declspec(dllexport) Detector * CreateDetector() {
    return new drace::detector::Fasttrack();
}
