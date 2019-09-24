#ifndef FASTTRACK_H
#define FASTTRACK_H

#include <mutex> // for lock_guard
#include <iostream>
#include <detector/Detector.h>
#include <ipc/spinlock.h>
#include <ipc/DrLock.h>
#include "threadstate.h"
#include "varstate.h"
#include "stacktrace.h"
#include "xmap.h"
#include "xvector.h"
#include "ipc/xlock.h"
//#include <unordered_map>
#include "parallel_hashmap/phmap.h"

#define MAKE_OUTPUT true
#define REGARD_ALLOCS false

namespace drace {

    namespace detector {

        class Fasttrack : public Detector {
        public:
            typedef size_t tid_ft;
            typedef DrLock rwlock;
            

        private:    
          
            /// globals ///
            static constexpr int max_stack_size = 16;

            ///these maps hold the various state objects together with the identifiers

            phmap::parallel_node_hash_map<size_t, size_t> allocs;
            phmap::parallel_node_hash_map<size_t, std::shared_ptr<VarState>> vars;
            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> locks;

            phmap::parallel_node_hash_map<tid_ft, std::shared_ptr<ThreadState>> threads;

            phmap::parallel_node_hash_map<void*, std::shared_ptr<VectorClock<>>> happens_states;
            phmap::parallel_node_hash_map<size_t, std::shared_ptr<StackTrace>> traces;
        
            ///holds the callback address to report a race to the drace-main 
            void * clb;

            ///those locks secure the .find and .insert actions to the according lists
            xlock t_insert;
            xlock v_insert;
            xlock s_insert;
            xlock l_insert;
            xlock h_insert;

            ///lock for the accesses of the var and thread objects
            xlock t_lock;

            ///locks for the accesses of the allocation objects
            rwlock a_lock;

            ///locks for the accesses of the stacktrace objects
            rwlock s_lock;


            void report_race(
                uint32_t thr1, uint32_t thr2,
                bool wr1, bool wr2,
                size_t var,
                uint32_t clk1, uint32_t clk2);

            void read(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v);

            void write(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v);

            auto create_var(size_t addr, size_t size);

            auto create_lock(void* mutex);

            void create_thread(Detector::tid_t tid, std::shared_ptr<ThreadState> parent = nullptr);

            auto create_happens(void* identifier);

            void create_alloc(size_t addr, size_t size);

        public:
            Fasttrack() {};

            //public functions are explained in the detector base class

            void cleanup(uint32_t tid);

            bool init(int argc, const char **argv, Callback rc_clb) override;

            void finalize() override;

            void read(tls_t tls, void* pc, void* addr, size_t size) override;

            void write(tls_t tls, void* pc, void* addr, size_t size) override;


            void func_enter(tls_t tls, void* pc) override;

            void func_exit(tls_t tls) override;


            void fork(tid_t parent, tid_t child, tls_t * tls) override;

            void join(tid_t parent, tid_t child) override;


            void acquire(tls_t tls, void* mutex, int recursive, bool write) override;

            void release(tls_t tls, void* mutex, bool write) override;


            void happens_before(tls_t tls, void* identifier) override;

            /** Draw a happens-after edge between thread and identifier (optional) */
            void happens_after(tls_t tls, void* identifier) override;

            void allocate(tls_t  tls, void*  pc, void*  addr, size_t size) override;

            /** Log a memory deallocation*/
            void deallocate( tls_t tls, void* addr) override;

            void detach(tls_t tls, tid_t thread_id) override;
            /** Log a thread exit event of a detached thread) */

            void finish(tls_t tls, tid_t thread_id) override;

            void map_shadow(void* startaddr, size_t size_in_bytes) override;


            const char * name() override;

            const char * version() override;
        };

    }
}



#endif // !FASTTRACK_H
