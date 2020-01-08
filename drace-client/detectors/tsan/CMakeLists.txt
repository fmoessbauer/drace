# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2019
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.

message(STATUS "Build detector tsan")

add_library("drace.detector.tsan" SHARED "tsan")
target_link_libraries("drace.detector.tsan"
    PRIVATE "tsan-common"
    PRIVATE "drace-common")
install(TARGETS "drace.detector.tsan" RUNTIME DESTINATION bin COMPONENT Runtime)

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET "drace.detector.tsan" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.tsan>"
		"${PROJECT_BINARY_DIR}/drace-client")

# copy transitive depends for tsan detector
# Copy tsan-common
add_custom_command(TARGET "drace.detector.tsan" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${TSAN_BINARY_DIR}/race_windows_amd64.dll"
		"${PROJECT_BINARY_DIR}/drace-client")
