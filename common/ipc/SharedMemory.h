#pragma once

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#include <stdexcept>

template<typename T = byte, bool nothrow = false>
class SharedMemory {
	HANDLE hMapFile;
	T*     buffer{ nullptr };
public:
	SharedMemory(const char * name, bool create = false) {
		if (create) {
			hMapFile = CreateFileMapping(
				INVALID_HANDLE_VALUE,    // use paging file
				NULL,                    // default security
				PAGE_READWRITE,          // read/write access
				0,                       // maximum object size (high-order DWORD)
				sizeof(T),         // maximum object size (low-order DWORD)
				name);                   // name of mapping object
		}
		else {
			hMapFile = OpenFileMapping(
				FILE_MAP_ALL_ACCESS,   // read/write access
				FALSE,                 // do not inherit the name
				name);
		}

		if (nullptr == hMapFile) {
			if (nothrow) return;
			throw std::runtime_error("error creating/attaching file mapping");
		}

		buffer = reinterpret_cast<T*>(MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0));
		if (nullptr == buffer) {
			if (nothrow) return;
			throw std::runtime_error("error creating file view");
		}
	}

	~SharedMemory() {
		if (nullptr != buffer) {
			UnmapViewOfFile(buffer);
		}
		if (nullptr != hMapFile) {
			CloseHandle(hMapFile);
		}
	}

	T* get() {
		return buffer;
	}
};