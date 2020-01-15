# Standalone

This directory contains the standalone components of the DRace project.
The DRACE_ENABLE_RUNTIME CMake flag can be set to `OFF`, if only this components shall be build. 

```bash
# from DRace root directory
git submodule init
git submodule update --recursive
mkdir build && cd build
cmake -DBOOST_ROOT=/opt/boost/ -DDRACE_ENABLE_RUNTIME=OFF ..
cmake --build .
```

## Standalone Components

- Fasttrack (Standalone Version)
- Binary Decoder

### Fasttrack

A standalone Data Race detector which can be used via the defined detector interface of [DRace](../common/detector/Detector.h) or with the [Binary Decoder](./binarydecoder/BinaryDecoder.cpp).
For an example how to use this interface, have a look into the [unit tests](../test/src/DetectorTest.cpp) of DRace-compatible detectors.

### Binary Decoder

Decodes a binary trace file which was created with the [TraceBinary](../drace-client/detectors/traceBinary/TraceBinary.cpp) detector of DRace and feeds the commands to a detector.

## Supported Environments

|Architecture|Windows        |Linux          |
|------------|---------------|---------------|
|x86 (32bit) | FT            | DRace-RT, FT  |
|x86 (64bit) | DRace         | DRace-RT, FT  |
|ARM         | -             | FT (untested) |

**Legend**:

- FT: Fasttrack library
- DRace-RT: Just DRace-Runtime
- DRace: Runtime + all components (e.g. FT, TSAN)
