#pragma once
/*
* Provides routines to demangle and reconstruct the stack at given program counter
*/

namespace stack_demangler {
	void demangle(void* pc);
}
