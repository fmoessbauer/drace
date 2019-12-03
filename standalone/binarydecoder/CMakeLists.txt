add_executable("drace.detector.tracebinary.decoder" "./BinaryDecoder.cpp")
target_link_libraries("drace.detector.tracebinary.decoder" "drace-common" "spdlog" "clipp")
target_compile_options("drace.detector.tracebinary.decoder" PUBLIC "/Zi")