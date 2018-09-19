#pragma once

#include "globals.h"
#include <dr_api.h>
#include <map>
#include <memory>

/* Encapsulates and enriches a dynamorio module_data_t struct */
class ModuleData {
public:
	/* Flags describing characteristics of a module */
	enum class MOD_TYPE_FLAGS : uint8_t {
		UNKNOWN = 0x0,
		NATIVE  = 0x1,
		MANAGED = 0x2,
	};

public:
	app_pc base;
	app_pc end;
	bool   loaded; // This is necessary to modify value in-place in set
	INSTR_FLAGS instrument;
	MOD_TYPE_FLAGS modtype{ MOD_TYPE_FLAGS::UNKNOWN };
	module_data_t *info{ nullptr };
	bool   debug_info;

private:
	/*
	* Determines if the module is a managed-code module
	* using the PE header
	*/
	void tag_module();

public:

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
		modtype(other.modtype),
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
		modtype(other.modtype),
		debug_info(other.debug_info)
	{
		other.info = nullptr;
	}

	void set_info(const module_data_t * mod) {
		info = dr_copy_module_data(mod);
		tag_module();
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
	// as we use lower_bound search, we have to reverse the sorting
	using map_t = std::map<app_pc, std::shared_ptr<ModuleData>, std::greater<app_pc>>;

	// RW mutex for access of modules container
	void *mod_lock;
	std::shared_ptr<Symbols> _syms;
	map_t _modules_idx;

public:
	using PModuleData = std::shared_ptr<ModuleData>;

	std::vector<std::string> excluded_mods;
	std::vector<std::string> excluded_path_prefix;

public:
	explicit ModuleTracker(const std::shared_ptr<Symbols> & syms);
	~ModuleTracker();

	/* Returns a shared_ptr to the module which contains the given program counter.
	 * If the pc is not in a known module, returns a nullptr
	 */
	PModuleData get_module_containing(const app_pc pc) const;

	/* Registers a new module by moving it */
	inline PModuleData add(ModuleData && mod) {
		PModuleData ptr = std::make_shared<ModuleData>(mod);
		return ptr;
	}

	/* Registers a new module by copying it */
	inline PModuleData add(const ModuleData & mod) {
		PModuleData ptr = std::make_shared<ModuleData>(mod);
		return ptr;
	}

	/* Creates new module in place */
	template<class... Args>
	inline PModuleData add_emplace(Args&&... args) {
		PModuleData ptr = std::make_shared<ModuleData>(std::forward<Args>(args)...);
		_modules_idx.emplace(ptr->base, ptr);
		return ptr;
	}

	/* Registers a module and sets flags accordingly */
	PModuleData register_module(const module_data_t * mod, bool loaded);

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
