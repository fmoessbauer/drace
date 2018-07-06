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
-s <sampling-rate>: from the considered instructions, 1/n are actually instrumented
--freq-only       : only instrument high-traffic code fragments
--excl-master     : exclude the runtime thread, useful if loader races
--delayed-syms    : Do not lookup symbols on each race
```

*Detector Parameters:*

```
--heap-only       : only detect races on heap-memory (exclude static memory)
```

### Debug Builds

- GoogleTest
- GoogleBenchmark

### Known Issues

- CSharp applications do not run on Windows 10 [#3046](https://github.com/DynamoRIO/dynamorio/issues/3046)
