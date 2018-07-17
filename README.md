# drace-client

The drace-client is a race-detector for windows build on top of DynamoRIO.
It does not require any perparations like instrumentation of the binary to check.

## Dependencies

- CMake > 3.8 (TODO: could be lessend)
- DynamoRIO 7.0.x
- C++11 / C99 Compiler

## Using the Drace Race Detector

**Run the detector as follows**

```bash
drrun.exe -c drace-client.dll <detector parameter> -- application.exe <app parameter>
```

Currently the following parameters are implemented

*Instrumentation Parameters:*

```
-c <filename>     : path to config file. If not set `drace.ini` is used
-s <sampling-rate>: from the considered instructions, 1/n are actually instrumented
--freq-only       : only instrument high-traffic code fragments
--yield-on-evt    : yield active thread after buffer is processed due to an event (e.g. mutex lock / unlock)
                    this might be necessary if more threads than cores are active
--excl-master     : exclude the runtime thread, useful if loader races
--delayed-syms    : Do not lookup symbols on each race
```

*Detector Parameters:*

```
--heap-only       : only detect races on heap-memory (exclude static memory)
```

## Testing with GoogleTest

Both the detector and a fully integrated DR-Client can be tested using the following command:

```
# Detector Tests
./test/drace-client-Tests.exe --dr <path-to-drrun.exe> --gtest_filter="Detector*"
# Integration Tests
./test/drace-client-Tests.exe --dr <path-to-drrun.exe> --gtest_filter="Dr*"
```

## Benchmarking with GoogleBenchmark

### Known Issues

- CSharp applications do not run on Windows 10 [#3046](https://github.com/DynamoRIO/dynamorio/issues/3046)
- TSAN can only be started once, as the cleanup is not fully working

