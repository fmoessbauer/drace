cmake_minimum_required (VERSION 3.8)

set(SOURCES "src/drace-client" "src/function-wrapper" "src/memory-instr" "src/module-tracker" "src/stack-demangler")

add_library("drace-client" SHARED ${SOURCES})
target_include_directories(
	"drace-client" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/drace-client> "include")

target_include_directories(
	"drace-client" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/drace-client> "detectors")


# Setup as dynamorio client (not-standalone)
# See https://github.com/DynamoRIO/dynamorio/blob/master/make/DynamoRIOConfig.cmake.in
# for details
configure_DynamoRIO_client("drace-client")
use_DynamoRIO_extension("drace-client" drmgr)
use_DynamoRIO_extension("drace-client" drutil)
use_DynamoRIO_extension("drace-client" drreg)
use_DynamoRIO_extension("drace-client" drwrap)

# Bind detectors
message("Use detector: ${DRACE_DETECTOR}")
add_subdirectory("detectors/${DRACE_DETECTOR}")
target_link_libraries("drace-client" "drace-${DRACE_DETECTOR}")

# TODO: Add Tests
# TODO: Add Benchmarks
