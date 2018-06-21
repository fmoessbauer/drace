cmake_minimum_required (VERSION 3.8)

set(SOURCES "src/drace-client" "src/function-wrapper" "src/memory-instr")

add_library("drace-client" SHARED ${SOURCES})
target_include_directories(
	"drace-client" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/drace-client> "include")

target_include_directories(
	"drace-client" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/drace-client> "detectors")

add_custom_command(TARGET "drace-client" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/race_windows_amd64.dll"
        $<TARGET_FILE_DIR:drace-client>)


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

if("${DRACE_DETECTOR}" STREQUAL "tsan")
	target_link_libraries("drace-client" "drace-tsan")
	add_custom_command(TARGET "drace-client" POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"detectors/tsan/drace-tsan.dll"
        $<TARGET_FILE_DIR:drace-client>)
endif()

if("${DRACE_DETECTOR}" STREQUAL "ft2l")
	target_link_libraries("drace-client" "drace-ft2l")
	add_custom_command(TARGET "drace-client" POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"detectors/tsan/drace-ft2l.dll"
        $<TARGET_FILE_DIR:drace-client>)
endif()

# TODO: Add Tests
# TODO: Add Benchmarks
