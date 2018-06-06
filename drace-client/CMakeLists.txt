cmake_minimum_required (VERSION 3.8)

set(SOURCES "src/drace-client")

add_executable (draceclient ${SOURCES})
target_include_directories(
	draceclient PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/draceclient> "include")

# TODO: Add Tests
# TODO: Add Benchmarks
