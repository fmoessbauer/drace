# Standalone

This directory contains the standalone components of the DRace project.
The DRACE_ENABLE_RUNTIME CMake flag can be set to ```false```, if one only this components shall be build.

## Standalone Components

- Fasttrack (Standalone Version)
- Binary Decoder

### Fasttrack

A standalone Data Race detector which can be fed via the defined detector interface of [DRace](../common/detector/Detector.h) or with the [Binary Decoder](./binarydecoder/BinaryDecoder.cpp).

### Binary Decoder

Decodes a binary trace file which was created with the [TraceBinary](../drace-client/detectors/traceBinary/TraceBinary.cpp) detector of DRace and feeds the commands to a detector.
