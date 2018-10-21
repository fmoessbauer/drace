#include "../detector_if.h"
#include <string>
#include <memory>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iostream>

#include "ipc/ExtsanData.h"
#include "ipc/SharedMemory.h"
#include "ipc/ringbuffer.hpp"
#include "ipc/spinlock.h"

#undef min
#undef max

using shm_t = ipc::SharedMemory<ipc::QueueMetadata, true>;

std::unique_ptr<shm_t> shm;

bool detector::init(int argc, const char **argv, Callback rc_clb) {
	shm = std::make_unique<ipc::SharedMemory<ipc::QueueMetadata, true>>("drace-events", false);
	return true;
}

void detector::finalize() {
	shm.reset();
}

void detector::acquire(
	tid_t thread_id,
	void* mutex,
	int recursive,
	bool write,
	bool try_lock,
	tls_t tls)
{
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::ACQUIRE;
	auto * buf = (ipc::event::Mutex*)(entry.buffer);
	buf->thread_id = thread_id;
	buf->acquire = true;
	buf->addr = (uint64_t)mutex;
	buf->recursive = recursive;
	buf->write = write;

	std::lock_guard<spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

/* Release a mutex */
void detector::release(
	tid_t thread_id,
	void* mutex,
	int recursive,
	bool write,
	tls_t tls)
{
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::RELEASE;
	auto * buf = (ipc::event::Mutex*)(entry.buffer);
	buf->thread_id = thread_id;
	buf->acquire = false;
	buf->addr = (uint64_t)mutex;
	buf->recursive = recursive;
	buf->write = write;

	std::lock_guard<spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

void detector::happens_before(tid_t thread_id, void* identifier) { }

void detector::happens_after(tid_t thread_id, void* identifier) { }

void detector::read(tid_t thread_id, void* callstack, unsigned stacksize, void* addr, size_t size, tls_t tls)
{
	auto * queue = (ipc::queue_t*)(tls);

	std::lock_guard<spinlock> lg(queue->mxspin);
	
	ipc::event::BufferEntry * entry = queue->get_next_write_slot();
	if (nullptr == entry) {
		std::this_thread::yield();
		return; // Queue is full
	}

	entry->type = ipc::event::Type::MEMREAD;
	auto * buf = (ipc::event::MemAccess*)(entry->buffer);
	buf->thread_id = thread_id;
	memcpy(buf->callstack.data(), callstack, stacksize * sizeof(void*));
	buf->stacksize = stacksize;
	buf->addr = (uint64_t)addr;
	queue->commit_write();
}

void detector::write(tid_t thread_id, void* callstack, unsigned stacksize, void* addr, size_t size, tls_t tls)
{
	auto * queue = (ipc::queue_t*)(tls);

	std::lock_guard<spinlock> lg(queue->mxspin);

	ipc::event::BufferEntry * entry = queue->get_next_write_slot();
	if (nullptr == entry) {
		std::this_thread::yield();
		return; // Queue is full
	}

	entry->type = ipc::event::Type::MEMWRITE;
	auto * buf = (ipc::event::MemAccess*)(entry->buffer);
	buf->thread_id = thread_id;
	memcpy(buf->callstack.data(), callstack, stacksize * sizeof(void*));
	buf->stacksize = stacksize;
	buf->addr = (uint64_t)addr;
	queue->commit_write();
}

void detector::allocate(tid_t thread_id, void* pc, void* addr, size_t size, tls_t tls)
{
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::ALLOCATION;
	auto * buf = (ipc::event::Allocation*)(entry.buffer);
	buf->pc = (uint64_t)pc;
	buf->addr = (uint64_t)addr;
	buf->size = size;
	buf->thread_id = thread_id;

	std::lock_guard<spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

void detector::deallocate(tid_t thread_id, void* addr, tls_t tls) {
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::FREE;
	auto * buf = (ipc::event::Allocation*)(entry.buffer);
	buf->addr = (uint64_t)addr;
	buf->thread_id = thread_id;

	std::lock_guard<spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
	ipc::QueueMetadata * qmeta = shm->get();
	auto & queue = qmeta->queues[++qmeta->next_queue % ipc::QueueMetadata::num_queues];

	*tls = (tls_t)&queue;

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::FORK;
	auto * buf = (ipc::event::ForkJoin*)(entry.buffer);
	buf->child = child;
	buf->parent = parent;

	std::lock_guard<spinlock> lg(queue.mxspin);
	queue.insert(entry);
}

void detector::join(tid_t parent, tid_t child, tls_t tls) {
	auto * queue = (ipc::queue_t*)(tls);

	ipc::event::BufferEntry entry;
	entry.type = ipc::event::Type::JOIN;
	auto * buf = (ipc::event::ForkJoin*)(entry.buffer);
	buf->child = child;
	buf->parent = parent;

	std::lock_guard<spinlock> lg(queue->mxspin);
	queue->insert(&entry);
}

std::string detector::name() {
	return std::string("Dummy");
}

std::string detector::version() {
	return std::string("1.0.0");
}
