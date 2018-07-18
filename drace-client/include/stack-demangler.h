#pragma once
/*
* Provides routines to demangle and reconstruct the stack at given program counter
*/

#include <string>

namespace stack_demangler {
	std::string demangle(void* pc);
}
