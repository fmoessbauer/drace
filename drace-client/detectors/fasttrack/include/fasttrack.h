#ifndef FASTTRACK_H
#define FASTTRACK_H

#include <mutex> // for lock_guard
#include <iostream>
#include <detector/Detector.h>
#include <ipc/spinlock.h>
#include "threadstate.h"
#include "varstate.h"
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
            xmap<size_t, VarState*> vars;
            xmap<void*, VectorClock*> locks;
            xmap<tid_ft, std::shared_ptr<ThreadState>> threads;
            xmap<void*, VectorClock*> happens_states;
            void * clb;

            //declaring order is also the acquiring order of the locks
            ipc::spinlock t_lock;
            ipc::spinlock v_lock;
            ipc::spinlock l_lock;
            ipc::spinlock h_lock;
            ipc::spinlock a_lock;

            void report_race(
                tid_ft thr1, tid_ft thr2,
                bool wr1, bool wr2,
                size_t var,
                uint32_t clk1, uint32_t clk2);



            void read(std::shared_ptr<ThreadState> t, VarState* v);

            void write(std::shared_ptr<ThreadState> t, VarState* v);

            void create_var(size_t addr, size_t size);

            void create_lock(void* mutex);

            void create_thread(Detector::tid_t tid, std::shared_ptr<ThreadState> parent = nullptr);

            void create_happens(void* identifier);

            void create_alloc(size_t addr, size_t size);

        public:

            void cleanup(size_t tid);

            bool init(int argc, const char **argv, Callback rc_clb);

            void finalize();

            void read(tls_t tls, void* pc, void* addr, size_t size);

            void write(tls_t tls, void* pc, void* addr, size_t size);


            void func_enter(tls_t tls, void* pc);

            void func_exit(tls_t tls);


            void fork(tid_t parent, tid_t child, tls_t * tls);

            void join(tid_t parent, tid_t child);


            void acquire(tls_t tls, void* mutex, int recursive, bool write);

            void release(tls_t tls, void* mutex, bool write);


            void happens_before(tls_t tls, void* identifier);

            /** Draw a happens-after edge between thread and identifier (optional) */
            void happens_after(tls_t tls, void* identifier);

            void allocate(
                /// ptr to thread-local storage of calling thread
                tls_t  tls,
                /// current instruction pointer
                void*  pc,
                /// begin of allocated memory block
                void*  addr,
                /// size of memory block
                size_t size
            );

            /** Log a memory deallocation*/
            void deallocate(
                /// ptr to thread-local storage of calling thread
                tls_t tls,
                /// begin of memory block
                void* addr
            );

            void detach(tls_t tls, tid_t thread_id);
            /** Log a thread exit event of a detached thread) */

            void finish(tls_t tls, tid_t thread_id);

            void map_shadow(void* startaddr, size_t size_in_bytes);


            const char * name();

            const char * version();
        };

    }
}


#endif // !FASTTRACK_H
