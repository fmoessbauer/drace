#include "globals.h"
#include "memory-tracker.h"
#include "module-tracker.h"
#include "symbols.h"
#include "race-collector.h"

/*
* Thread local storage metadata has to be globally accessable
*/
reg_id_t tls_seg;
uint     tls_offs;
int      tls_idx;

void *th_mutex;
void *tls_rw_mutex;

// Global Config Object
drace::Config config;

std::atomic<int> num_threads_active{ 0 };
std::atomic<uint> runtime_tid{ 0 };
std::unordered_map<thread_id_t, per_thread_t*> TLS_buckets;
std::atomic<thread_id_t> last_th_start{ 0 };
std::atomic<bool> th_start_pending{ false };

std::string config_file("drace.ini");

std::unique_ptr<MemoryTracker> memory_tracker;
std::unique_ptr<ModuleTracker> module_tracker;
std::unique_ptr<Symbols> symbol_table;
std::unique_ptr<RaceCollector> race_collector;

/* Runtime parameters */
params_t params;
