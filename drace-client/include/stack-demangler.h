#pragma once
/*
* Provides routines to demangle and reconstruct the stack at given program counter
*/

#include "symbols.h"

#include <string>

namespace stack_demangler {
	symbols::SymbolLocation demangle(void* pc);
}
