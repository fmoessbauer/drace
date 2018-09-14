#pragma once

#include <thread>

#include "SharedMemory.h";
#include "SMData.h"

/* Handles communication between Drace and MSR */
template<bool SENDER, bool NOTHROW = true>
class MsrDriver {
private:
	SharedMemory<SMData, NOTHROW> _shm;
	SMData * _comm;

protected:
	void yield() const {
		std::this_thread::yield();
	}

public:

	// Attach to shared memory if any
	explicit MsrDriver(const char* shmkey = DRACE_SMR_NAME, bool create = false):
		_shm(shmkey, create),
		_comm(_shm.get())
	{ }

	~MsrDriver() {
		if (nullptr != _comm) {
			_comm->id = SMDataID::EXIT;
			commit();
		}
	}

	/* Returns a pointer to the raw buffer */
	template<typename T = byte>
	T * data() {
		return reinterpret_cast<T*>(_comm->buffer);
	}

	SMDataID id() const {
		return _comm->id;
	}

	void commit() {
		_shm.notify();
	}

	template<typename duration = std::chrono::milliseconds>
	bool wait_receive(const duration & d = std::chrono::milliseconds(100)) {
		return _shm.wait(d);
	}

	/* Sets the message id / state */
	inline void state(SMDataID mid) {
		_comm->id = mid;
	}

	/* Puts value of type T at begin of buffer */
	template<typename T>
	inline void put(SMDataID mid, const T & val) {
		_comm->id = mid;
		reinterpret_cast<T*>(_comm->buffer)[0] = val;
	}


	/* Gets value of type T from begin of buffer */
	template<typename T>
	inline T & get() const {
		return reinterpret_cast<T*>(_comm->buffer)[0];
	}

	/*
	* returns true if the msrdriver is in a valid state and can be used.
	* This is for the nothrow case, where the constructor cannot 
	* throw a exception if an error occured during shm attaching
	*/
	inline bool valid() const {
		return (nullptr != _comm);
	}
};