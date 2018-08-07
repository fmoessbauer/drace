#ifndef STATISTICS_H
#define STATISTICS_H

#include <chrono>
#include <iostream>

/**
 * print Lossy Counting Model State
 */
template<typename State>
void printState(const State & lcm_state){
  std::cout << "LCM Model State: "
            << "f:" << lcm_state.f << ", "
            << "e:" << lcm_state.e << ", "
            << "w:" << lcm_state.w << ", "
            << "N:" << lcm_state.N << std::endl;
}

/**
 * print performance data
 */
template<
  typename State,
  typename Duration>
void printPerformance(
    const State & lcm_state,
    const int   & elem_size,
    const Duration & dur){
  using ms = std::chrono::milliseconds;
  constexpr int MEGA = 1024 * 1024;

  auto duration = std::chrono::duration_cast<ms>(dur);
  long long processed_bytes = (lcm_state.N * elem_size);
  long long bytes_per_ms    = processed_bytes / duration.count();
  long long elems_per_ms    = lcm_state.N / duration.count();
  double mb_per_s     = static_cast<double>(bytes_per_ms) / 1024.0;
  double elems_per_s  = static_cast<double>(elems_per_ms) / 1024.0;

  std::cout << "--- Performance ---" << std::endl
            << "Processed MB: " << (processed_bytes) / (MEGA) << std::endl
            << "Time in ms:   " << duration.count()           << std::endl
            << "MB/s:         " << mb_per_s                   << std::endl
            << "MElem/s:      " << elems_per_s                << std::endl
            << "--- ----------- ---" << std::endl;
}

#endif
