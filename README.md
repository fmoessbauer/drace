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
drrun.exe -no_follow_children -c drace-client.dll <detector parameter> -- application.exe <app parameter>
# see limitations for -no_follow_children option
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
--excl-traces     : exclude frequent pathes by not addind memory instrumentation
                  : to DynamoRIO traces (only instrument non-trace BBs)
--excl-stack      : exclude addresses inside the stack of the calling thread
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
e        enable detector on all threads
d        disable detector on all threads
s <rate> set sampling rate to 1/x (similar to `-s` in DRace)
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

### Custom Annotations

Custom synchonisation logic is supported by annotating the corresponding code sections.
Thereto we provide a header with macros in `drace-client/include/annotations/drace_annotation.h`.
To enable these macros, define `DRACE_ANNOTATION` prior to including the header.

A example how to use the annotations is given in `test/mini-apps/annotations/`.

## Testing with GoogleTest

Both the detector and a fully integrated DR-Client can be tested using the following command:

```
# Detector Tests
./test/drace-client-Tests.exe --dr <path-to-drrun.exe> --gtest_filter="Detector*"
# Integration Tests
./test/drace-client-Tests.exe --dr <path-to-drrun.exe> --gtest_filter="Dr*"
```

**Note:** Before pushing a commit, please run the integration tests.
Later on, bugs are very tricky to find. 

## Benchmarking with GoogleBenchmark

### Limitations

- CSharp applications do not run on Windows 10 [#3046](https://github.com/DynamoRIO/dynamorio/issues/3046)
- TSAN can only be started once, as the cleanup is not fully working
- `no_follow_children`: Due to the TSAN limitation, drace can only analyze a single process. This process is the initially started one.
- If using the SparsePP hashmap, the application might crash if a reallocation occurs which is not detected by DR correctly.

## Build

DRace is build using CMake. The only external dependency is DynamoRIO.
For best compability with Windows 10, use the latest available weekly build.
The path to your DynamoRIO installation has to be set using `-DDynamoRIO_DIR`.

A sample VisualStudio `CMakeSettings.json` is given here:

```
{
  "name": "x64-Release-TSAN",
  "generator": "Ninja",
  "configurationType": "RelWithDebInfo",
  "inheritEnvironments": [ "msvc_x64_x64" ],
  "buildRoot": "${env.USERPROFILE}\\CMakeBuilds\\${workspaceHash}\\build\\${name}",
  "installRoot": "${env.USERPROFILE}\\CMakeBuilds\\${workspaceHash}\\install\\${name}",
  "cmakeCommandArgs": "-DDRACE_XML_EXPORTER=1 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=<PATH-TO-DYNAMORIO>/cmake -DDRACE_DETECTOR=tsan",
  "buildCommandArgs": "-v",
  "ctestCommandArgs": ""
}
```

### Documentation

A doxygen documentation can be generated by building the `doc` target.

### Available Detectors

DRace is shipped with the following detector backends:

- tsan (internal ThreadSanitizer)
- extsan (external ThreadSanitizer, WIP)
- dummy (no detection at all)

To select which detector is build, set `-DDRACE_DETECTOR=<value>` CMake flag.

**tsan**

The detector is run along with the application. No further threads are started.

**extsan**

DRace sends all events (memory-accesses, sync events, ...) to a different process (MSR) using shared memory and fifo queues.
The MSR process then passes the events to the ThreadSanitizer. For communication and analysis, an arbitrary number of queues can be used.
Each queue is then processed and analyzed by it's own thread.

*Note*: This is work-in-progress and is there for evaluation of this concept. On systems with only a few cores, the performance is poor.

**dummy**

This detector does not detect any races. It is there to evaluate the overhead of the other detectors vs the instrumentation overhead.

## Licensing

DRace is primarily licensed under the terms of the MIT license.

Each of its source code files contains a license declaration in its header. Whenever a file is provided under an additional or different license than MIT, this is stated in the file header.
Any file that may lack such a header has to be considered licensed under MIT (default license).

If two licenses are specified in a file header, you are free to pick the one that suits best your particular use case.
You can also continue to use the file under the dual license.
When choosing only one, remove the reference to the other from the file header.

### External Ressources

Most external ressources are located in the `vendor` directory.
For licensing information regarding these components, we refer to the information bundled with the individual ressource.

### License Header Format

```
/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) <COPYRIGHT HOLDER>, <YEAR>
 *
 * Authors:
 *  Your Name <your.email@host.dom>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */
```
