#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <iterator>

#include <lossyCountingModel.hpp>
#include "TestSuite.h"

int runAllTests(){
  using result_t = std::pair<std::string, int>;
  std::vector<result_t> results;

  results.push_back(std::make_pair("batch", testBatch()));
  results.push_back(std::make_pair("stream", testStream()));

  int failedTests = 0;
  std::cout << "== Summary ==" << std::endl;
  for(const auto & result : results){
    std::cout << result.first << ":"
              << (result.second == 0 ? "Success" : "Failure")
              << std::endl;
    failedTests+=result.second;
  }
  return failedTests;
}

template<typename C>
void append_n(C & c, int n, typename C::value_type x){
  std::generate_n(std::back_inserter(c), n, [&x](){return x;});
}

template<typename Map>
bool assertAndCount(
    const Map & c,
    const typename Map::key_type key,
    const int count)
{
  bool passed = true;
  int  gotcount = 0;
  if(c.count(key) != 0){
    gotcount = c.at(key);
    passed &= gotcount == count;
  } else {
    passed &= count == 0;
  }
  if(!passed){
    std::cout << "Expected for " << key
              << ": " << count
              << ", got: " << gotcount
              << std::endl;
  }
  return passed;
}

int testBatch(){
  double frequency = 0.2;
  double error     = 0.1 * frequency;
  std::vector<int> buffer;
  bool batch_passed = true;

  LossyCountingModel<int> lcm(frequency, error);
  const auto lcm_state = lcm.getState();
  buffer.resize(lcm_state.w); // should be 50

  std::fill_n(buffer.begin(),    19, 0);
  std::fill_n(buffer.begin()+19, 11, 1);
  std::fill_n(buffer.begin()+30, 10, 2);
  std::fill_n(buffer.begin()+40, 10, 3);

  lcm.processWindow(buffer.begin());
  auto result = lcm.computeOutput();
  // 0 freq = 18/50 = 0.36
  // 1 freq = 10/50 = 0.2,
  // 2 freq = 9/50 = 0.18
  // 3 freq = 9/50 = 0.18

  batch_passed &= (result.size() == 4);
  batch_passed &= assertAndCount(result, 0, 18);
  batch_passed &= assertAndCount(result, 1, 10);
  batch_passed &= assertAndCount(result, 2,  9);
  batch_passed &= assertAndCount(result, 3,  9);

  // window 1
  std::fill_n(buffer.begin(),    30, 0);
  std::fill_n(buffer.begin()+30, 10, 1);
  std::fill_n(buffer.begin()+40, 10, 2);

  lcm.processWindow(buffer.begin());
  result = lcm.computeOutput();
  // 0 freq = 47/100
  // 1 freq = 19/100
  // 2 freq = 18/100
  // 3 freq = 8/100

  batch_passed &= (result.size() == 3);
  batch_passed &= assertAndCount(result, 0, 47);
  batch_passed &= assertAndCount(result, 1, 19);
  batch_passed &= assertAndCount(result, 2, 18);
 
  // window 2
  std::fill_n(buffer.begin(),    30, 0);
  std::fill_n(buffer.begin()+30, 10, 1);
  std::fill_n(buffer.begin()+40,  5, 3);
  std::fill_n(buffer.begin()+45,  5, 4);

  lcm.processWindow(buffer.begin());
  result = lcm.computeOutput();
  // 0 freq = 76/150
  // 1 freq = 28/150
  // 2 freq = 17/150
  // 3 freq = 12/150

  batch_passed &= (result.size() == 2);
  batch_passed &= assertAndCount(result, 0, 76);
  batch_passed &= assertAndCount(result, 1, 28);
 
  // window 3
  std::fill_n(buffer.begin(),    40, 0);
  std::fill_n(buffer.begin()+40, 10, 1);

  lcm.processWindow(buffer.begin());
  result = lcm.computeOutput();
  // 0 freq = 115/200
  // 1 freq =  37/200
  // 2 freq =  16/200
  // 3 freq =  11/200

  batch_passed &= (result.size() == 2);
  batch_passed &= assertAndCount(result, 0, 115);
  batch_passed &= assertAndCount(result, 1,  37);

  // window 4
  // same data
  lcm.processWindow(buffer.begin());
  result = lcm.computeOutput();
  // 0 freq = 154/250
  // 1 freq =  46/250
  // 2 freq =  15/250
  // 3 freq =  10/250

  batch_passed &= (result.size() == 2);
  batch_passed &= assertAndCount(result, 0, 154);
  batch_passed &= assertAndCount(result, 1,  46);

  return (batch_passed ? 0 : 1);
}

int testStream(){
  double frequency = 0.2;
  double error     = 0.1 * frequency;
  std::vector<int> buffer;
  bool batch_passed = true;

  LossyCountingModel<int> lcm(frequency, error);
  const auto lcm_state = lcm.getState();
  buffer.reserve(lcm_state.w * 5); // should be 50 * 5 

  append_n(buffer, 19, 0);
  append_n(buffer, 11, 1);
  append_n(buffer, 10, 2);
  append_n(buffer, 10, 3);

  append_n(buffer, 30, 0);
  append_n(buffer, 10, 1);
  append_n(buffer, 10, 2);

  append_n(buffer, 30, 0);
  append_n(buffer, 10, 1);
  append_n(buffer,  5, 2);
  append_n(buffer,  5, 3);

  append_n(buffer, 40, 0);
  append_n(buffer, 10, 1);

  append_n(buffer, 40, 0);
  append_n(buffer, 10, 1);

  int hit = 0;
  for(const int & e : buffer){
    hit += lcm.processItem(e);
  }
  auto result = lcm.computeOutput();
//  std::cout << hit << std::endl; 
  batch_passed &= (hit == 5);
  batch_passed &= (result.size() == 2);
  batch_passed &= assertAndCount(result, 0, 154);
  batch_passed &= assertAndCount(result, 1,  46);

  return (batch_passed ? 0 : 1);
}
