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
target_link_libraries("drace.detector.tsan" PRIVATE "tsan-common" "drace-common")
install(TARGETS "drace.detector.tsan"
    RUNTIME DESTINATION ${DRACE_RUNTIME_DEST} COMPONENT RUNTIME)
