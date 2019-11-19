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

#include <string>
#include <fstream>

#include <dr_api.h>
#include <ipc/ExtsanData.h>
#include <detector/Detector.h>

using namespace ipc::event;

namespace drace
{
    namespace detector
    {
        /// Fake detector that traces all calls to the \ref Detector interface
        class TraceBinary : public Detector
        {
        private:
            void * iolock;
            std::fstream file;

        public:
            virtual bool init(int argc, const char **argv, Callback rc_clb) {
                iolock = dr_mutex_create();
                file = std::fstream("trace.bin", std::ios::out | std::ios::binary);
                return true;
            }

            virtual void finalize() {
                dr_mutex_destroy(iolock);
                file.close();
            }

            virtual void map_shadow(void* startaddr, size_t size_in_bytes) { };

            virtual void func_enter(tls_t tls, void* pc) {
                write_log_sync({Type::FUNCENTER, {(uint32_t)tls, (uint64_t)pc}});
            }

            virtual void func_exit(tls_t tls) {
                write_log_sync({Type::FUNCEXIT, {(uint32_t)tls}});
            }

            virtual void acquire(
                tls_t tls,
                void* mutex,
                int recursive,
                bool write)
            {
                ipc::event::BufferEntry buf{Type::ACQUIRE};
                buf.payload.mutex = {(uint32_t)tls, (uint64_t)mutex, (int)recursive, write, true};
                write_log_sync(buf);
            }

            virtual void release(
                tls_t tls,
                void* mutex,
                bool write) {
                    ipc::event::BufferEntry buf{Type::RELEASE};
                    buf.payload.mutex = {(uint32_t)tls, (uint64_t)mutex, (int)0, write, false};
                    write_log_sync(buf);
                }

            virtual void happens_before(tls_t tls, void* identifier) {
                write_log_sync({Type::HAPPENSBEFORE, {(uint32_t)tls, (uint64_t)identifier}});
            }

            virtual void happens_after(tls_t tls, void* identifier) {
                write_log_sync({Type::HAPPENSAFTER, {(uint32_t)tls, (uint64_t)identifier}});
            }

            virtual void read(tls_t tls, void* pc, void* addr, size_t size) {
                ipc::event::BufferEntry buf{Type::MEMREAD};
                buf.payload.memaccess = {(uint32_t)tls, (uint64_t)pc, (uint64_t)addr, (uint64_t)size};
                write_log_sync(buf);
            }

            virtual void write(tls_t tls, void* pc, void* addr, size_t size) {
                ipc::event::BufferEntry buf{Type::MEMWRITE};
                buf.payload.memaccess = {(uint32_t)tls, (uint64_t)pc, (uint64_t)addr, (uint64_t)size};
                write_log_sync(buf);
            }

            virtual void allocate(tls_t tls, void* pc, void* addr, size_t size) {
                ipc::event::BufferEntry buf{Type::ALLOCATION};
                buf.payload.allocation = {(uint32_t)tls, (uint64_t)pc, (uint64_t)addr, (uint64_t)size};
                write_log_sync(buf);
            }

            virtual void deallocate(tls_t tls, void* addr) {
                ipc::event::BufferEntry buf{Type::FREE};
                buf.payload.allocation = {(uint32_t)tls, (uint64_t)0x0, (uint64_t)addr, (uint64_t)0x0};
                write_log_sync(buf);
            }

            virtual void fork(tid_t parent, tid_t child, tls_t * tls) {
                *tls = (void*)((uint64_t)child);
                write_log_sync({Type::FORK, {(uint32_t)parent, (uint32_t)child}});
            }

            virtual void join(tid_t parent, tid_t child) {
                write_log_sync({Type::JOIN, {(uint32_t)parent, (uint32_t)child}});
            }

            virtual void detach(tls_t tls, tid_t thread_id) {
                write_log_sync({Type::DETACH, {(uint32_t)tls}});
            };

            virtual void finish(tls_t tls, tid_t thread_id) {
                write_log_sync({Type::FINISH, {(uint32_t)tls}});
            };

            virtual const char * name() {
                return "Dummy";
            }

            virtual const char * version() {
                return "1.0.0";
            }

        private:
            void write_log_sync(const ipc::event::BufferEntry & event){
                dr_mutex_lock(iolock);
                file.write((char*)&event, sizeof(ipc::event::BufferEntry));
                dr_mutex_unlock(iolock);
            }
        };
    }
}

extern "C" __declspec(dllexport) Detector * CreateDetector() {
    return new drace::detector::TraceBinary();
}
