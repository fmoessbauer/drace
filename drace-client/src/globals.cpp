#include "globals.h"
#include "memory-tracker.h"
#include "Module.h"
#include "symbols.h"
#include "race-collector.h"
#include "statistics.h"
#include "ipc/MtSyncSHMDriver.h"

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

std::chrono::system_clock::time_point app_start;
std::chrono::system_clock::time_point app_stop;

std::string config_file("drace.ini");

std::unique_ptr<MemoryTracker> memory_tracker;
std::unique_ptr<module::Tracker> module_tracker;
std::shared_ptr<Symbols> symbol_table;
std::unique_ptr<RaceCollector> race_collector;
std::unique_ptr<Statistics> stats;
std::unique_ptr<ipc::MtSyncSHMDriver<true, true>> shmdriver;

/* Runtime parameters */
params_t params;
