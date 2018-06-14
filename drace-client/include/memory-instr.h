#pragma once

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <atomic>

namespace memory_inst {

	static std::atomic<int> writes;
	static std::atomic<int> reads;

	bool register_events();

	/* allocate thread local storage
	   currently not necessary, as we call tsan directly */
	void allocate_tls();

	void finalize();

	void read_access_mem(opnd_t opcode, opnd_t address);
	void write_access_mem(opnd_t opcode, opnd_t address);

	// Events
	void event_thread_init(void *drcontext);

	void event_thread_exit(void *drcontext);


	/* We transform string loops into regular loops so we can more easily
	* monitor every memory reference they make.
	*/
	dr_emit_flags_t event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);
	dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
		instr_t *instr, bool for_trace,
		bool translating, void *user_data);
}
