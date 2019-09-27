# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2019
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.

set(SOURCES
"src/fasttrack"
"src/threadstate"
"src/stacktrace"
"src/varstate")

# we just need BGL which is header only, hence avoid errors by not importing as a target
find_package(Boost 1.65.0)

if(Boost_FOUND)
    message(STATUS "Build detector fasttrack")
    add_library("drace.detector.fasttrack" SHARED ${SOURCES})

    target_include_directories("drace.detector.fasttrack"
        PRIVATE "include"
        PRIVATE "${Boost_INCLUDE_DIR}")
    target_link_libraries("drace.detector.fasttrack" "drace-common" "parallel-hashmap")
    target_link_libraries("drace.detector.fasttrack")
    install(TARGETS "drace.detector.fasttrack" RUNTIME DESTINATION bin COMPONENT Runtime)

    configure_DynamoRIO_standalone("drace.detector.fasttrack")

    # copy detector dlls to drace-client binary dir
    add_custom_command(TARGET "drace.detector.fasttrack" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:drace.detector.fasttrack>"
            "${PROJECT_BINARY_DIR}/drace-client")
endif()
