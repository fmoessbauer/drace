#ifndef LOSSY_COUNTING_MODEL_H
#define LOSSY_COUNTING_MODEL_H

#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>

/**
 * Lossy Counting Model implemenation for processing a
 * data stream according to the following constraints:
 * - Single Pass
 * - Limited memory
 * - Volume of data in real-time
 */
template<typename T>
class LossyCountingModel {
public:
  /**
   * holds a LCM Model state
   */
  struct State {
    double    f; /// frequency
    double    e; /// error
    unsigned long      w; /// window size
    unsigned long long N; /// stream length
  };

private:
  using histogram_type = typename std::unordered_map<T,size_t>;

  double              _frequency;
  double              _error;
  unsigned long       _window_size;
  unsigned long long  _total_processed_elements = 0;
  histogram_type      _histogram;

public:
  /**
   * Setup a Lossy Counting Model with sampling
   * frequency and error.
   */
  LossyCountingModel(double frequency, double error, long estimated_length = 1e5) noexcept :
    _frequency(frequency),
    _error(error),
    _window_size(static_cast<unsigned long>(std::round(1.0 / error))) 
  {
    // tuning of hash table
    //int estimated_size = static_cast<int>((1.0 / _error) * std::log2(
    //                        _error * static_cast<double>(estimated_length)));
    //_histogram.reserve(estimated_size);
	  _histogram.reserve(1024);
  }

  /**
   * process a single item of the datastream
   * \return true if the sampling window was processed after this item,
   *         false otherwise
   */
  bool processItem(const T & e) noexcept {
    ++(_histogram[e]);
    ++_total_processed_elements;
    if((_total_processed_elements % _window_size) == 0)
    {
      _decreaseAllFrequencies();
      return true;
    }
    return false;
  }

  /**
   * Process N elements from begin iterator without copying it to a
   * temporary buffer. The caller is responsible for providing enough
   * elements
   */
  template<typename Iter>
  void processWindow(Iter it) noexcept {
    for(unsigned int cnt = 0; cnt < _window_size; ++cnt){
      ++(_histogram[*it]);
      ++it;
    }
    _total_processed_elements += _window_size;
    _decreaseAllFrequencies();
  }

  /**
   * returns the final histogram containing only elements
   * with count exceeding fN - eN. The type of the container
   * the final histogram is stored in can be set using the
   * template parameter.
   */
  template<typename ContainerT = std::map<T,int>>
  ContainerT computeOutput() const noexcept {
    ContainerT result;

    const int threshold = (_frequency * _total_processed_elements) -
                    (_error * _total_processed_elements);

    std::copy_if(_histogram.begin(), _histogram.end(),
        std::inserter(result, result.end()),
        [&](const std::pair<T,int> & el){
          return el.second >= threshold;
          });
    return result;
  }

  State getState() const noexcept {
    return State{_frequency, _error, _window_size, _total_processed_elements};
  }

private:
  void _decreaseAllFrequencies() noexcept {
    auto it = _histogram.begin();
    while(it != _histogram.end()){
      if(it->second == 1){
        it = _histogram.erase(it);
      } else {
        --(it->second);
        ++it;
      }
    } 
  }
};
#endif

