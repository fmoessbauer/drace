/**
 * Applies a Lossy Counting Model on a data stream.
 * This example uses a synthetic stream which contains
 * normal distributed ASCII letters
 */

#include <iostream>
#include <random>
#include <chrono>
#include <util/statistics.hpp>
#include <lossyCountingModel.hpp>

std::normal_distribution<> getDistribution(){
  char lower = '0';
  char upper = 'z';
  char mean  = (upper + lower) / 2;
  return std::normal_distribution<>(mean, 4);
}


int main(int argc, char ** argv){
  // define input stream
  long   stream_size = 1e7;
  double frequency   = 0.05;
  double error       = 0.005 * frequency;

  // use a random normal distribution of [0-z]
  std::random_device rd;
  std::mt19937 gen(rd());
  auto dist = getDistribution();

  // setup counting model
  LossyCountingModel<char> lcm(frequency, error); 

  // process stream
  auto begin = std::chrono::high_resolution_clock::now();
  for(int i=0; i<stream_size; ++i){
    lcm.processItem(std::round(dist(gen)));
  }

  // compute results
  auto results   = lcm.computeOutput();
  auto end   = std::chrono::high_resolution_clock::now();

  printState(lcm.getState());
  printPerformance(lcm.getState(), sizeof(char), end-begin);

  // output results
  std::cout << "Histogram:" << std::endl;
  for(const auto & kv : results){
    std::cout << "{" << kv.first << ", " << kv.second << "}" << std::endl;
  }
  return 0;
}

