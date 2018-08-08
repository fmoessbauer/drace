#pragma once

#include "globals.h"

#include <dr_api.h>

/* Encapsulates and enriches a dynamorio module_data_t struct */
class ModuleData {
public:
	app_pc base;
	app_pc end;
	mutable bool   loaded; // This is necessary to modify value in-place in set
	mutable INSTR_FLAGS instrument;
	module_data_t *info{ nullptr };
	mutable bool   debug_info;

	ModuleData(app_pc mbase, app_pc mend, bool mloaded = true, bool debug_info = false) :
		base(mbase),
		end(mend),
		loaded(mloaded),
		instrument(INSTR_FLAGS::NONE),
		debug_info(debug_info){ }

	~ModuleData() {
		if (info != nullptr) {
			dr_free_module_data(info);
			info = nullptr;
		}
	}

	/* Copy constructor, duplicates dr module data */
	ModuleData(const ModuleData & other) :
		base(other.base),
		end(other.end),
		loaded(other.loaded),
		instrument(other.instrument),
		debug_info(other.debug_info)
	{
		info = dr_copy_module_data(other.info);
	}

	/* Move constructor, moves dr module data */
	ModuleData(ModuleData && other) :
		base(other.base),
		end(other.end),
		loaded(other.loaded),
		instrument(other.instrument),
		info(other.info),
		debug_info(other.debug_info)
	{
		other.info = nullptr;
	}

	void set_info(const module_data_t * mod) {
		info = dr_copy_module_data(mod);
	}

	inline bool operator==(const ModuleData & other) const {
		return (base == other.base);
	}
	inline bool operator!=(const ModuleData & other) const {
		return !(base == other.base);
	}
	inline bool operator<(const ModuleData & other) const {
		return (base < other.base);
	}
	inline bool operator>(const ModuleData & other) const {
		return (base > other.base);
	}
};

/* Single element cache to speedup module lookup */
class ModuleCache {
private:
	/* keep last module here for faster lookup */
	uint64_t     mc_beg = 0;
	uint64_t     mc_end = 0;
	INSTR_FLAGS mc_instr = INSTR_FLAGS::NONE;

public:
	/* Lookup last module in cache, returns (found, instrument)*/
	inline std::pair<bool, INSTR_FLAGS> lookup(app_pc pc) {
		if ((uint64_t)pc >= mc_beg && (uint64_t)pc < mc_end) {
			return std::make_pair(true, mc_instr);
		}
		return std::make_pair(false, INSTR_FLAGS::NONE);
	}
	inline void update(app_pc beg, app_pc end, INSTR_FLAGS instrument) {
		mc_beg = (uint64_t)beg;
		mc_end = (uint64_t)end;
		mc_instr = instrument;
	}
};

class ModuleTracker {
	// RW mutex for access of modules container
	void *mod_lock;
	std::shared_ptr<Symbols> _syms;

public:
	using ModuleData_Set = std::set<ModuleData, std::greater<ModuleData>>;
	ModuleData_Set modules;

	std::vector<std::string> excluded_mods;
	std::vector<std::string> excluded_path_prefix;

public:
	explicit ModuleTracker(const std::shared_ptr<Symbols> & syms);
	~ModuleTracker();

	/* Returns an iterator to the module which contains the given program counter.
	 * The first entry of the pair denots if the pc is in a known module
	 */
	std::pair<bool, ModuleData_Set::iterator> get_module_containing(app_pc pc) const
	{
		ModuleData current(pc, nullptr);

		auto m_it = modules.lower_bound(current);
		if (m_it != modules.end() && pc < m_it->end) {
			return std::make_pair(true, m_it);
		}
		else {
			return std::make_pair(false, modules.end());
		}
	}

	/* Request a read-lock for the module dataset*/
	inline void lock_read() const {
		dr_rwlock_read_lock(mod_lock);
	}

	inline void unlock_read() const {
		dr_rwlock_read_unlock(mod_lock);
	}

	/* Request a write-lock for the module dataset*/
	inline void lock_write() const {
		dr_rwlock_write_lock(mod_lock);
	}

	inline void unlock_write() const {
		dr_rwlock_write_unlock(mod_lock);
	}
};

static void event_module_load(void *drcontext, const module_data_t *mod, bool loaded);
static void event_module_unload(void *drcontext, const module_data_t *mod);
