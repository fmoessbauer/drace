# Lossy Counting Model

[![CircleCI](https://circleci.com/gh/fmoessbauer/LossyCountingModel.svg?style=shield)](https://circleci.com/gh/fmoessbauer/LossyCountingModel)

Implementation of the Lossy Counting stream processing algorithm in C++.

Parts of the implementation are based on [mvogiatzis/freq-count](https://github.com/mvogiatzis/freq-count). He also made an excellent blog article on how the algorithms works.

The implementation of the algorithm is capable of processing streams with millions of elements by using only very limited resources. This includes the following features:

- Single pass
- Limited memory
- Real-time data processing

## How the model works

The Lossy Counting Model takes a frequency and an error as input. The frequency denotes the threshold for an item to be visible in the output. The error denotes the maximum underestimation of a returned element to be e*N where N is the lenght of the stream.

The model is capable of processing single elements as well as batches of size window size. In both cases no data is copied.

### Setup a model and process

```c++
LossyCountingModel<T> lcm(frequency, error);

// feed in data
while(...){
  lcm.processItem(item);  
}

// compute results (map containing the histogram)
auto results = lcm.computeOutput();
```

## Compiling

The implementation requires C++11 and is header only. Hence it can be easily included into any application. You might also have a look at the provided examples.


## Contributing

Do you want to improve the implementation or report bugs? Just contact me or open an issue / pull request on GitHub.

## Performance

The implementation of the model should be with good performance. However the main limitation in the examples is the generation of the random numbers.
