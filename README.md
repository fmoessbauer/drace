# DRace

[![reuse compliant](https://reuse.software/badge/reuse-compliant.svg)](https://reuse.software/)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/2553/badge)](https://bestpractices.coreinfrastructure.org/projects/2553)

DRace is a data-race detector for windows applications which uses DynamoRIO
to dynamically instrument a binary at runtime.
It does not require any preparations like instrumentation of the binary to check.
While the detector should work with all binaries that use the POSIX synchronization API,
we focus on applications written in C and C++.
Experimental support for hybrid applications containing native and Dotnet CoreCLR parts
is implemented as well.

For best results, we recommend to provide debug symbols of the
application under test along with the binary.
Without this information, callstacks cannot be fully symbolized (exported functions only)
and user-level synchronization cannot be detected.

## Dependencies

### Runtime

When using the pre-build releases, only DynamoRIO is required:

- [DynamoRIO](https://github.com/DynamoRIO/dynamorio) 7.1.x (use weekly releases)

### Development

- CMake > 3.8
- C++11 / C99 Compiler
- [DynamoRIO](https://github.com/DynamoRIO/dynamorio) > 7.0.x

For detailed information on all dependencies, see `DEPENDENCIES.md`.

### External Libraries

#### DRace

**Mandatory:**

- [jtilly/inih](https://github.com/jtilly/inih)
- [HowardHinnant/date](https://github.com/HowardHinnant/date)
- [muellan/clipp](https://github.com/muellan/clipp)
- LLVM-Tsan (a customized version is included in binary format)

**Optional:**

- [leethomason/tinyxml2](https://github.com/leethomason/tinyxml2)
- [google/googletest](https://github.com/google/googletest)
- [google/benchmark](https://github.com/google/benchmark)

#### Managed Symbol Resolver (MSR)

**Mandatory:**

- [gabime/spdlog](https://github.com/gabime/spdlog)
- [greq7mdp/sparsepp](https://github.com/greg7mdp/sparsepp)

## Using the DRace Race Detector

**Run the detector as follows**

```bash
drrun.exe -no_follow_children -c drace-client.dll <detector parameter> -- application.exe <app parameter>
# see limitations for -no_follow_children option
```

**Command Line Options**

```
SYNOPSIS
        drace-client.dll [-c <config>] [-s <sample-rate>] [-i <instr-rate>] [--lossy
                         [--lossy-flush]] [--excl-traces] [--excl-stack] [--excl-master] [--stacksz
                         <stacksz>] [--delay-syms] [--sync-mode] [--fast-mode] [--suplevel
                         <sample-rate>] [--xml-file <filename>] [--out-file <filename>] [--logfile
                         <filename>] [--extctrl] [--brkonrace] [--version] [-h] [--heap-only]

OPTIONS
        DRace Options
            -c, --config <config>
                    config file (default: drace.ini)

            sampling options
                -s, --sample-rate <sample-rate>
                    sample each nth instruction (default: no sampling)

                -i, --instr-rate <instr-rate>
                    instrument each nth instruction (default: no sampling)

            analysis scope
                --lossy
                    dynamically exclude fragments using lossy counting

                --lossy-flush
                    de-instrument flushed segments (only with --lossy)

                --excl-traces
                    exclude dynamorio traces

                --excl-stack
                    exclude stack accesses

                --excl-master
                    exclude first thread

            --stacksz <stacksz>
                    size of callstack used for race-detection (must be in [1,16], default: 10)

            --delay-syms
                    perform symbol lookup after application shutdown

            --sync-mode
                    flush all buffers on a sync event (instead of participating only)

            --fast-mode
                    DEPRECATED: inverse of sync-mode

            --suplevel <level>
                    suppress similar races (0=detector-default, 1=unique top-of-callstack entry,
                    default: 1)

            data race reporting
                --xml-file, -x <filename>
                    log races in valkyries xml format in this file

                --out-file, -o <filename>
                    log races in human readable format in this file

            --logfile, -l <filename>
                    write all logs to this file (can be null, stdout, stderr, or filename)

            --extctrl
                    use second process for symbol lookup and state-controlling (required for Dotnet)

            --brkonrace
                    abort execution after first race is found (for testing purpose only)

            --version
                    display version information

            -h, --usage
                    display help

        Detector (TSAN) Options
            --heap-only
                    only analyze heap memory
```

### Externally Controlling DRace

DRace can be externally controlled from a controller (`msr.exe`) running in a second process.
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

If symbols for system libraries are necessary (e.g. for Dotnet), they have to be downloaded from a symbol server.
Thereto it is useful to set the variable as follows:

```
set _NT_SYMBOL_PATH="c:\\symbolcache\\;SRV*c:\\symbolcache\\*https://msdl.microsoft.com/download/symbols"
```

### Dotnet

For .Net managed code, a second process (MSR) is needed for symbol resolution.
The MSR is started as follows:

```
ManagedResolver\msr.exe [-v for verbose]
```

After it is started, DRace connects to MSR using shared memory.
The MSR then tries to locate the correct [DAC](https://github.com/dotnet/coreclr/blob/master/Documentation/botr/dac-notes.md)
DLL to resolve managed program counters and symbols.

The output (logs) of the MSR are just for debugging reasons.
The resolved symbols are passed back to drace and merged with the non-managed ones.

### Custom Annotations

Custom synchonisation logic is supported by annotating the corresponding code sections.
Thereto we provide a header with macros in `drace-client/include/annotations/drace_annotation.h`.
To enable these macros, define `DRACE_ANNOTATION` prior to including the header.

A example on how to use the annotations is provided in `test/mini-apps/annotations/`.

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

## Limitations

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

To clone all submodules of this repository, issue the following command inside the drace directory:

```
git submodule update --recursive
```

### Documentation

A doxygen documentation can be generated by building the `doc` target.

### Available Detectors

DRace is shipped with the following detector backends:

- tsan (internal ThreadSanitizer)
- extsan (external ThreadSanitizer, WIP)
- dummy (no detection at all)

To select which detector is build, set the `-DDRACE_DETECTOR=<value>` CMake flag.

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

We use the [REUSE](https://reuse.software/practices/) format for license and copyright information.

```
/*
 * DRace, a dynamic data race detector
 *
 * Copyright <YEAR> <COPYRIGHT HOLDER>
 *
 * SPDX-License-Identifier: MIT 
 */
```

## Citing DRace

A publicly available fulltext of the master's thesis can be found here: [High Performance Dynamic Threading Analysis for Hybrid Applications](https://epub.ub.uni-muenchen.de/60621/)

To cite DRace, please reference:

```
F. Mößbauer. "High Performance Dynamic Threading Analysis for Hybrid Applications", Master Thesis, Faculty of Mathematics, Computer Science and Statistics, Ludwig-Maximilians-Universität München (2019).
```

**BibTex**
```
@misc{moes19,
           title = {High Performance Dynamic Threading Analysis for Hybrid Applications},
         keyword = {Concurrency Bugs; Race Condition; Program Analysis; Binary Instrumentation; Sampling; Managed Applications},
        abstract = {Verifying the correctness of multithreaded programs is a challenging task due to errors that occur sporadically. Testing, the most important verification method for decades, has proven to be ineffective in this context. On the other hand, data race detectors are very successful in finding concurrency bugs that occur due to missing synchronization. However, those tools introduce a huge runtime overhead and therefore are not applicable to the analysis of real-time applications. Additionally, hybrid binaries consisting of Dotnet and native components are beyond the scope of many data race detectors.
In this thesis, we present a novel approach for a dynamic low-overhead data race detector. We contribute a set of fine-grained tuning techniques based on sampling and scoping. These are evaluated on real-world applications, demonstrating that the runtime overhead is reduced while still maintaining a good detection accuracy. Further, we present a proof of concept for hybrid applications and show that data races in managed Dotnet code are detectable by analyzing the 
application on the binary layer. The approaches presented in this thesis are implemented in the open-source tool DRace.},
            year = {2019},
          author = {Felix M\"o\ssbauer},
             url = {http://nbn-resolving.de/urn/resolver.pl?urn=nbn:de:bvb:19-epub-60621-8}
}
```

