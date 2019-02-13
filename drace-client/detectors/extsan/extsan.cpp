/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */

#include <string>
#include <memory>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iostream>

#include <detector/detector_if.h>

#include "ipc/ExtsanData.h"
#include "ipc/SharedMemory.h"
#include "ipc/ringbuffer.hpp"
#include "ipc/spinlock.h"

#undef min
#undef max

namespace detector {
	/// external detector which performes analysis in MSR
	namespace extsan {

		using shm_t = ipc::SharedMemory<ipc::QueueMetadata, true>;

		struct tls_data {
			detector::tid_t thread_id;
			ipc::queue_t* queue;
		};
	} // namespace extsan
} // namespace detector

std::unique_ptr<detector::extsan::shm_t> shm;

bool detector::init(int argc, const char **argv, Callback rc_clb) {
	shm = std::make_unique<ipc::SharedMemory<ipc::QueueMetadata, true>>("drace-events", false);
	return true;
}

void detector::finalize() {
	shm.reset();
}

void detector::acquire(
	tls_t tls,
	void* mutex,
	int recursive,
	bool write)
{
	using namespace extsan;
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::ACQUIRE;
	auto * buf = (ipc::event::Mutex*)(entry.buffer);
	buf->thread_id = ((tls_data*)&tls)->thread_id;
	buf->acquire = true;
	buf->addr = (uint64_t)mutex;
	buf->recursive = recursive;
	buf->write = write;

	std::lock_guard<ipc::spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

/* Release a mutex */
void detector::release(
	tls_t tls,
	void* mutex,
	bool write)
{
	using namespace extsan;
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::RELEASE;
	auto * buf = (ipc::event::Mutex*)(entry.buffer);
	buf->thread_id = ((tls_data*)&tls)->thread_id;
	buf->acquire = false;
	buf->addr = (uint64_t)mutex;
	buf->write = write;

	std::lock_guard<ipc::spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

void detector::happens_before(tid_t thread_id, void* identifier) { }

void detector::happens_after(tid_t thread_id, void* identifier) { }

void detector::read(tls_t tls, void* callstack, unsigned stacksize, void* addr, size_t size)
{
	using namespace extsan;
	auto * queue = (ipc::queue_t*)(tls);

	std::lock_guard<ipc::spinlock> lg(queue->mxspin);

	ipc::event::BufferEntry * entry = queue->get_next_write_slot();
	if (nullptr == entry) {
		std::this_thread::yield();
		return; // Queue is full
	}

	entry->type = ipc::event::Type::MEMREAD;
	auto * buf = (ipc::event::MemAccess*)(entry->buffer);
	buf->thread_id = ((tls_data*)&tls)->thread_id;
	memcpy(buf->callstack.data(), callstack, stacksize * sizeof(void*));
	buf->stacksize = stacksize;
	buf->addr = (uint64_t)addr;
	queue->commit_write();
}

void detector::write(tls_t tls, void* callstack, unsigned stacksize, void* addr, size_t size)
{
	using namespace extsan;
	auto * queue = (ipc::queue_t*)(tls);

	std::lock_guard<ipc::spinlock> lg(queue->mxspin);

	ipc::event::BufferEntry * entry = queue->get_next_write_slot();
	if (nullptr == entry) {
		std::this_thread::yield();
		return; // Queue is full
	}

	entry->type = ipc::event::Type::MEMWRITE;
	auto * buf = (ipc::event::MemAccess*)(entry->buffer);
	buf->thread_id = ((tls_data*)&tls)->thread_id;
	memcpy(buf->callstack.data(), callstack, stacksize * sizeof(void*));
	buf->stacksize = stacksize;
	buf->addr = (uint64_t)addr;
	queue->commit_write();
}

void detector::allocate(tls_t tls, void* pc, void* addr, size_t size)
{
	using namespace extsan;
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::ALLOCATION;
	auto * buf = (ipc::event::Allocation*)(entry.buffer);
	buf->pc = (uint64_t)pc;
	buf->addr = (uint64_t)addr;
	buf->size = size;
	buf->thread_id = ((tls_data*)&tls)->thread_id;

	std::lock_guard<ipc::spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

void detector::deallocate(tls_t tls, void* addr) {
	using namespace extsan;
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::FREE;
	auto * buf = (ipc::event::Allocation*)(entry.buffer);
	buf->addr = (uint64_t)addr;
	buf->thread_id = ((tls_data*)&tls)->thread_id;

	std::lock_guard<ipc::spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
	using namespace extsan;
	ipc::QueueMetadata * qmeta = shm->get();
	auto & queue = qmeta->queues[++qmeta->next_queue % ipc::QueueMetadata::num_queues];
	auto tls_ = new tls_data;

	tls_->thread_id = child;
	tls_->queue = &queue;

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::FORK;
	auto * buf = (ipc::event::ForkJoin*)(entry.buffer);
	buf->child = child;
	buf->parent = parent;

	std::lock_guard<ipc::spinlock> lg(queue.mxspin);
	queue.insert(entry);
	*tls = (tls_t)tls_;
}

void detector::join(tid_t parent, tid_t child, tls_t tls) {
	using namespace extsan;
	auto * queue = ((tls_data*)&tls)->queue;

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::JOIN;
	auto * buf = (ipc::event::ForkJoin*)(entry.buffer);
	buf->child = child;
	buf->parent = parent;

	std::lock_guard<ipc::spinlock> lg(queue->mxspin);
	queue->insert(&entry);
	delete ((tls_data*)&tls);
}

std::string detector::name() {
	return std::string("Dummy");
}

std::string detector::version() {
	return std::string("1.0.0");
}