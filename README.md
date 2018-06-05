# drace-client

The drace-client is a race-detector for windows build on top of DynamoRIO.
It does not require any perparations like instrumentation of the binary to check.

## Dependencies

- CMake > 3.8 (TODO: could be lessend)
- DynamoRIO 7.0.x
- C++11 / C99 Compiler

### Debug Builds

- GoogleTest
- GoogleBenchmark
