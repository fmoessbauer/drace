/**
 * Applies a Lossy Counting Model on a data stream.
 * This example uses a synthetic stream which contains
 * normal distributed ASCII letters
 *
 * As the creation of random numbers is a bottleneck, this implementation
 * pre-calculates them
 */

#include <iostream>
#include <random>
#include <chrono>
#include <util/statistics.hpp>
#include <lossyCountingModel.hpp>

using value_t = long;
std::normal_distribution<> getDistribution(){
  return std::normal_distribution<>(0, 10);
}

int main(int argc, char ** argv){
  // define input stream
  long   stream_size = 1e9;
  double frequency   = 0.01;
  double error       = 0.001 * frequency;
  std::vector<value_t> buffer;

  // setup counting model
  LossyCountingModel<value_t> lcm(frequency, error, stream_size);
  const auto lcm_state = lcm.getState();
  // divide the stream in 20 parts
  const int  pre_cal_win = (stream_size / lcm_state.w) / 20;
  buffer.resize(lcm_state.w * pre_cal_win);
  printState(lcm_state);

  std::chrono::high_resolution_clock::duration time_acc{0};
  const int num_windows = stream_size / lcm_state.w;
  const int num_calls   = num_windows / pre_cal_win;

#pragma omp parallel
  {
  // use one random device per thread
  std::random_device rd;
  std::mt19937 gen(rd());
  auto dist = getDistribution();

  for(int call = 0; call<num_calls; ++call){
    // calculate numbers
#pragma omp for schedule(static, 2048)
    for(unsigned int i=0; i<lcm_state.w * pre_cal_win; ++i){
      buffer[i] = std::round(dist(gen));
    }

#pragma omp single
    {
    // process stream
    auto begin = std::chrono::high_resolution_clock::now();
    for(int win=0; win<pre_cal_win; ++win){
      const auto & begptr = buffer.cbegin() + (win * lcm_state.w);
      lcm.processWindow(begptr);
    }
    auto end  = std::chrono::high_resolution_clock::now();
    time_acc += end-begin;

    // print progress
    double prog = static_cast<double>(call) / static_cast<double>(num_calls);
    std::cout << "Progress: " << prog * 100 << "%" << std::endl;
    } // omp single 
    }
  } // omp parallel
  // compute results
  auto results   = lcm.computeOutput();

  printPerformance(lcm.getState(), sizeof(value_t), time_acc);

  // output results
  std::cout << "Histogram:" << std::endl;
  for(const auto & kv : results){
    std::cout << "{" << kv.first << ", " << kv.second << "}" << std::endl;
  }
  return 0;
}

