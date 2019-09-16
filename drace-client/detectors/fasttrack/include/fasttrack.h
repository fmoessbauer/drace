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


#define MAKE_OUTPUT false
#define REGARD_ALLOCS true

namespace drace {

    namespace detector {

        class Fasttrack : public Detector {
        public:
            typedef size_t tid_ft;

        private:    
            /// globals ///
            static constexpr int max_stack_size = 16;

            xmap<size_t, size_t> allocs;
            xmap<size_t, std::shared_ptr<VarState>> vars;
            xmap<void*, std::shared_ptr<VectorClock>> locks;
            xmap<tid_ft, std::shared_ptr<ThreadState>> threads;
            xmap<void*, std::shared_ptr<VectorClock>> happens_states;
            xmap<size_t, std::shared_ptr<StackTrace>> traces;
            void * clb;

            //declaring order is also the acquiring order of the locks

            DrLock t_lock;
            DrLock v_lock;

            //DrLock h_lock;
            //DrLock a_lock;


            void report_race(
                tid_ft thr1, tid_ft thr2,
                bool wr1, bool wr2,
                size_t var,
                uint32_t clk1, uint32_t clk2);



            void read(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v);

            void write(std::shared_ptr<ThreadState> t, std::shared_ptr<VarState> v);

            void create_var(size_t addr, size_t size);

            void create_lock(void* mutex);

            void create_thread(Detector::tid_t tid, std::shared_ptr<ThreadState> parent = nullptr);

            void create_happens(void* identifier);

            void create_alloc(size_t addr, size_t size);

        public:
            Fasttrack() {};

            void cleanup(size_t tid);

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

            void allocate(
                /// ptr to thread-local storage of calling thread
                tls_t  tls,
                /// current instruction pointer
                void*  pc,
                /// begin of allocated memory block
                void*  addr,
                /// size of memory block
                size_t size
            ) override;

            /** Log a memory deallocation*/
            void deallocate(
                /// ptr to thread-local storage of calling thread
                tls_t tls,
                /// begin of memory block
                void* addr
            ) override;

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
