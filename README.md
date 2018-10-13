# DRace

The drace-client is a race detector for windows build on top of DynamoRIO.
It does not require any perparations like instrumentation of the binary to check.
However if debug information is not available, header-only C++11 constructs
cannot be detected correctly.

## Dependencies

- CMake > 3.8
- DynamoRIO 7.0.x
- C++11 / C99 Compiler

### External Libraries

#### DRace

**Mandatory:**

- [jtilly/inih](https://github.com/jtilly/inih)
- [leethomason/tinyxml2](https://github.com/leethomason/tinyxml2)
- [HowardHinnant/date](https://github.com/HowardHinnant/date)

**Optional:**

- [google/googletest](https://github.com/google/googletest)
- [google/benchmark](https://github.com/google/benchmark)
- [greq7mdp/sparsepp](https://github.com/greq7mdp/sparsepp)

#### MSR

**Mandatory:**

- [gabime/spdlog](https://github.com/gabime/spdlog)

## Using the DRace Race Detector

**Run the detector as follows**

```bash
drrun.exe -c drace-client.dll <detector parameter> -- application.exe <app parameter>
```

Currently the following parameters are implemented

*Instrumentation Parameters:*

```
-c <filename>     : path to config file. If not set `drace.ini` is used
-s <sampling-rate>: from all observed memory-references, analyze 1/n
-i <instr-rate>   : from the considered instructions, 1/n are actually instrumented
--lossy           : do not gather mem-refs from high-traffic application parts after some time
--lossy-flush     : remove instrumentation from high-traffic application parts after some time
                  : (only in combination with --lossy)
--yield-on-evt    : yield active thread after buffer is processed due to an event (e.g. mutex lock / unlock)
                    this might be necessary if more threads than cores are active
--excl-master     : exclude the runtime thread, useful if loader races
--delayed-syms    : do not lookup symbols on each race
--fast-mode       : only flush local buffers on sync-event (all buffers otherwise)
--stacksz         : size of callstack used for race-detection (must be in [1,16])
--xml-file <file> : log races in valkyries xml format in this file
--out-file <file> : log races in human readable format in this file
--extctrl         : Use second process for symbol lookup and state-controlling (required for Dotnet)
```

*Detector Parameters:*

```
--heap-only       : only detect races on heap-memory (exclude static memory)
```

### External Controlling DRace

DRace can be externally controlled using the external controller `msr.exe`.
To set the detector state during runtime, the following keys (committed using `enter`) are available:

```
e\t enable detector on all threads
d\t disable detector on all threads
s\t <rate> set sampling rate to 1/x (similar to `-s` in DRace)
```

### Symbol Resolving

DRace requires symbol information for wrapping functions and to resolve stack traces.
For the main functionality of C and C++ only applications, export information is sufficient.
However for additional and more precise race-detection (e.g. C++11, QT), debug information is necessary.

The application searches for this information in the path of the module and in `_NT_SYMBOL_PATH`.
However, only local symbols are searched (non `SRV` parts).

If symbols for system libs are necessary (e.g. for .Net), download them first from a symbol server.
Thereto it is useful to set the variable as follows:

```
set _NT_SYMBOL_PATH="c:\\symbolcache\\;SRV*c:\\symbolcache\\*https://msdl.microsoft.com/download/symbols"
```

### Dotnet

For .Net managed code, a second process (called MSR) is needed for symbol resolution.
This can be started as follows:

```
ManagedResolver\msr.exe [-v for verbose]
```

After it is started, DRace connects to MSR using shared memory.
MSR then tries to locate the correct [DAC](https://github.com/dotnet/coreclr/blob/master/Documentation/botr/dac-notes.md)
dll to resolve managed program counters and symbols.

The output (logs) of MSR are just for debugging reasons. The resolved symbols are passed back to drace
and merged with the non-managed ones.

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
- If using the SparsePP hashmap, the application might crash if a reallocation occurs which is not detected by DR correctly.

