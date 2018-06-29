#include "stack-demangler.h"
#include "drace-client.h"
#include "module-tracker.h"

#include <dr_api.h>

void stack_demangler::demangle(void *stack) {
	// get top element of stack
	app_pc top = *(app_pc *)stack;
	dr_printf("Demangle Stack at %p\n", top);

	// Create dummy shadow module
	module_tracker::module_info_t pc_mod(top, nullptr);
	
	auto pc_mod_it = modules.lower_bound(pc_mod);
	if ((pc_mod_it != modules.end()) && (top < pc_mod_it->end)) {
		// bb in known module
		const char * mod_name = dr_module_preferred_name(pc_mod_it->info);
		dr_printf("PC %p is in Module %s, from %p to %p\n",
			top, mod_name, pc_mod_it->base, pc_mod_it->end);
	}
	else {
		dr_printf("PC %p is in Unknown Module\n", top);
	}
}
