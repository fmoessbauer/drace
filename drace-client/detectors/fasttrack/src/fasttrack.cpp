/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "fasttrack.h"
#include "fasttrack_export.h"

namespace drace {

    namespace detector {

        //function is not thread safe!
        void Fasttrack::report_race(
            uint32_t thr1, uint32_t thr2,
            bool wr1, bool wr2,
            size_t var,
            uint32_t clk1, uint32_t clk2)
        {
            std::list<size_t> stack1, stack2;
            {
                std::shared_ptr<StackTrace> s1;
                std::shared_ptr<StackTrace> s2;
                std::lock_guard<rwlock> ex_l(s_lock);
                auto it = traces.find(thr1);
                if (it != traces.end()) {
                    s1 = it->second;
                }
                else { //if thread is, because of finishing not in stack traces anymore, return
                    return;
                }
                it = traces.find(thr2);
                if (it != traces.end()) {
                    s2 = it->second;
                }
                else {//if thread is, because of finishing not in stack traces anymore, return
                    return;
                }

                stack1 = s1->return_stack_trace(var);
                stack2 = s2->return_stack_trace(var);
            }
            
            size_t var_size = 0;
            var_size = vars[var]->size; //size is const member -> thread safe

            while(stack1.size() > Detector::max_stack_size) {
                stack1.pop_front();
            }
            while (stack2.size() > Detector::max_stack_size) {
                stack2.pop_front();
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
                        
            if (t->return_own_id() == v->get_read_id()) {//read same epoch, same thread;
                return;
            }

            std::lock_guard<xlock> ex_l(t_lock);
            size_t id = t->return_own_id();
            uint32_t tid = t->get_tid();
            
            if (v->is_read_shared()  && v->get_vc_by_thr(tid) == id) { //read shared same epoch
                return;
            }

            if (v->is_wr_race(t))
            { // write-read race
                report_race(v->get_w_tid(), tid, true, false, v->address, v->get_w_clock(), t->get_clock());
            }

            //update vc
            if (!v->is_read_shared())
            {
                if (v->get_read_id() == VarState::VAR_NOT_INIT ||
                   (v->get_r_tid() == tid )) //read exclusive->read of same thread but newer epoch
                {
                    v->update(false, id);
                }
                else { // read gets shared
                    v->set_read_shared(id);
                }
            }
            else {//read share
                v->update(false, id);
            }
        }

        ///function is thread safe
        void Fasttrack::write(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v) {

            if (t->return_own_id() == v->get_write_id()){//write same epoch
                return;
            }


            std::lock_guard<xlock> ex_l(t_lock);

            if (v->get_write_id() == VarState::VAR_NOT_INIT) { //initial write, update var   
                v->update(true, t->return_own_id());
                return;
            }

            uint32_t tid = t->get_tid();

            //tids are different && and write epoch greater or equal than known epoch of other thread
            if (v->is_ww_race(t)) // write-write race
            {
                report_race(v->get_w_tid(), tid, true, true, v->address, v->get_w_clock(), t->get_clock());
            }

            if (! v->is_read_shared()) {
                if (v->is_rw_ex_race(t))// read-write race
                {
                    report_race(v->get_r_tid(), t->get_tid(), false, true, v->address, v->get_r_clock(), t->get_clock());
                }
            }
            else {//come here in read shared case
                uint32_t act_tid = v->is_rw_sh_race(t);
                if (act_tid != 0) //read shared read-write race       
                {
                    report_race(act_tid, tid, false, true, v->address, v->get_clock_by_thr(act_tid), t->get_clock());
                }
            }
            v->update(true, t->return_own_id());
        }

        ///returns iterator to inserted element
        auto Fasttrack::create_var(size_t addr, size_t size) {
            uint16_t u16_size = static_cast<uint16_t>(size);

            auto var = std::make_shared<VarState>(addr, u16_size);
            return vars.insert({ addr, var }).first;
        }

        auto Fasttrack::create_lock(void* mutex) {
           auto lock = std::make_shared<VectorClock<>>();
           return locks.insert({ mutex, lock }).first;
        }

        void Fasttrack::create_thread( Detector::tid_t tid, std::shared_ptr<ThreadState> parent) {

            std::shared_ptr<ThreadState> new_thread;
            if (parent == nullptr) {
                new_thread = std::make_shared<ThreadState>(this, tid);
                threads.insert( { tid, new_thread });
            }
            else {
                std::lock_guard<xlock> ex_l(t_lock);
                new_thread = std::make_shared<ThreadState>( this, tid, parent);
                threads.insert({ tid, new_thread });
            }

            //create also a object to save the stack traces
            std::lock_guard<xlock> ex_l(s_insert);
            traces.insert({ tid, std::make_shared<StackTrace>() });

        }

        auto Fasttrack::create_happens(void* identifier) {
            auto new_hp = std::make_shared<VectorClock<>>();
            return happens_states.insert({ identifier, new_hp }).first;
        }

        void Fasttrack::create_alloc(size_t addr, size_t size) {
            allocs.insert({ addr, size });
        }


        void Fasttrack::cleanup(uint32_t tid) {
            //as ThreadState is destroyed delete all the entries from all vectorclocks
            {
                std::lock_guard<xlock> ex_l(t_lock);
                for (auto it = locks.begin(); it != locks.end(); ++it) {
                    it->second->delete_vc(tid);
                }

                for (auto it = threads.begin(); it != threads.end(); ++it) {
                    it->second->delete_vc(tid);
                }

                for (auto it = happens_states.begin(); it != happens_states.end(); ++it) {
                    it->second->delete_vc(tid);
                }
            }
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
            std::shared_ptr<VarState> var;
            std::shared_ptr<ThreadState> thr;
            {
                std::shared_lock<rwlock> sh_l(s_lock);
                auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
                stack->set_read_write((size_t)(addr), reinterpret_cast<size_t>(pc));
            }

            {
                std::shared_lock<xlock> ex_l(v_insert);
                auto it = vars.find((size_t)(addr));
                if (it == vars.end()) { //create new variable if new
#if MAKE_OUTPUT
                    std::cout << "variable is read before written" << std::endl;//warning
#endif
                    var = (create_var((size_t)(addr), size))->second;
                }
                else {
                    var = it->second;
                }
            }

            thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            read(thr, var);
        }

        void Fasttrack::write(tls_t tls, void* pc, void* addr, size_t size) {
            std::shared_ptr<VarState> var;
            std::shared_ptr<ThreadState> thr;

            {
                std::shared_lock<rwlock> sh_l(s_lock);
                auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
                stack->set_read_write((size_t)addr, reinterpret_cast<size_t>(pc));
            }
            {
                std::shared_lock<xlock> ex_l(v_insert);
                auto it = vars.find((size_t)addr);//vars[(size_t)addr];
                if (it == vars.end()) {
                    var = (create_var((size_t)(addr), size))->second;
                }
                else {
                    var = it->second;
                }
            }
            thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];
          
            write(thr, var); //func is thread_safe     
        }


        void Fasttrack::func_enter(tls_t tls, void* pc) {

            auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            size_t stack_element = reinterpret_cast<size_t>(pc);
            //shared lock is enough as a single thread cannot do more than one action (r,w,enter,exit...) at a time
            std::shared_lock<rwlock> sh_l(s_lock);
            stack->push_stack_element(stack_element);
        }

        void Fasttrack::func_exit(tls_t tls) {
            auto stack = traces[reinterpret_cast<Fasttrack::tid_ft>(tls)];
            
            //shared lock is enough as a single thread cannot do more than one action (r,w,enter,exit...) at a time
            std::shared_lock<rwlock> sh_l(s_lock);
            stack->pop_stack_element(); //pops last stack element of current clock
        }


        void Fasttrack::fork(tid_t parent, tid_t child, tls_t * tls) {
            Fasttrack::tid_ft child_size_t = child;
            *tls = reinterpret_cast<void*>(child_size_t);

            std::lock_guard<xlock> ex_l(t_insert);
            if (threads.find(parent) != threads.end()) {
                {
                    threads[parent]->inc_vc(); //inc vector clock for creation of new thread
                }
                create_thread(child, threads[parent]);
            }
            else {
                create_thread(child);
            }

        }

        void Fasttrack::join(tid_t parent, tid_t child) {
            std::shared_ptr<ThreadState> del_thread = threads[child];

            {
                std::lock_guard<xlock> ex_l(t_lock);
                del_thread->inc_vc();
                //pass incremented clock of deleted thread to parent
                threads[parent]->update(del_thread);
                del_thread->delete_vector();
            }
            {
                std::lock_guard<xlock> ex_l(t_insert);
                threads.erase(child);
            }
            std::lock_guard<xlock> ex_l(s_insert);
            traces.erase(child);
        }


        void Fasttrack::acquire(tls_t tls, void* mutex, int recursive, bool write) {

            std::shared_ptr<VectorClock<>> lock;

            {
                std::lock_guard<xlock> ex_l(l_insert);
                auto it = locks.find(mutex);
                if (it == locks.end()) {
                    lock = create_lock(mutex)->second;
                }
                else {
                    lock = it->second;
                }
            }
            auto id = reinterpret_cast<Fasttrack::tid_ft>(tls);
            auto thr = threads[id];

            std::lock_guard<xlock> ex_l(t_lock);            
            (thr)->update(lock);

        }

        void Fasttrack::release(tls_t tls, void* mutex, bool write) {
            std::shared_ptr<VectorClock<>> lock;

            {
                std::lock_guard<xlock> ex_l(l_insert);
                auto it = locks.find(mutex);
                if (it == locks.end()) {
    #if MAKE_OUTPUT
                    std::cerr << "lock is released but was never acquired by any thread" << std::endl;
    #endif
                    create_lock(mutex)->second;
                    return;//as lock is empty (was never acquired), we can return here
                }
                else {
                    lock = it->second;
                }
            }
            tid_ft id = reinterpret_cast<tid_ft>(tls);
            auto thr = threads[id];
            
            std::lock_guard<xlock> ex_l(t_lock);
            (thr)->inc_vc();
            
             //increase vector clock and propagate to lock    
            (lock)->update(thr);

        }


        void Fasttrack::happens_before(tls_t tls, void* identifier) {

            std::shared_ptr<VectorClock<>> hp;

            {
                std::lock_guard<xlock> ex_l(h_insert);
                auto it = happens_states.find(identifier);
                if (it == happens_states.end()) {
                    hp = create_happens(identifier)->second;
                }
                else {
                    hp = it->second;
                }
            }
           
            std::shared_ptr<ThreadState> thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];

            std::lock_guard<xlock> ex_l(t_lock);
            (thr)->inc_vc();//increment clock of thread and update happens state
            hp->update(thr->get_tid(), thr->return_own_id());
        }

        /** Draw a happens-after edge between thread and identifier (optional) */
        void Fasttrack::happens_after(tls_t tls, void* identifier) {
            std::shared_ptr<VectorClock<>> hp;
            {
                std::lock_guard<xlock> ex_l(h_insert);
                auto it = happens_states.find(identifier);
                if (it == happens_states.end()) {
                    create_happens(identifier);
                    return;//create -> no happens_before can be synced
                }
                else {
                    hp = it->second;
                }
            }
            std::shared_ptr<ThreadState> thr = threads[reinterpret_cast<Fasttrack::tid_ft>(tls)];

            std::lock_guard<xlock> ex_l(t_lock);
            //update vector clock of thread with happened before clocks
            thr->update(hp);

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
            std::lock_guard<rwlock> ex_l(a_lock);
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
            size_t end_addr;
            {
                std::shared_lock<rwlock> sh_l(a_lock);
                end_addr = address + allocs[address];
            }

            //variable is deallocated so varstate objects can be destroyed
            while (address < end_addr) {
                std::lock_guard<xlock> ex_l(t_lock);
                if (vars.find(address) != vars.end()) {
                    std::shared_ptr<VarState> var = vars.find(address)->second;
                    vars.erase(address);
                    address++;
                }
                else {
                    address++;
                }
            }
            std::lock_guard<rwlock> ex_l(a_lock);
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
            {
                std::lock_guard<xlock> ex_l(t_lock);
                threads[thread_id]->delete_vector();
            }
            {
                std::lock_guard<xlock> ex_l(t_insert);
                threads.erase(thread_id);
            }
            std::lock_guard<xlock> ex_l(s_insert);
            traces.erase(thread_id);
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

extern "C" FASTTRACK_EXPORT Detector * CreateDetector() {
    return new drace::detector::Fasttrack();
}
