# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2019
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.

set(FT_SOURCES
"src/stacktrace"
"src/varstate"
"src/threadstate")

set(FT_TEST_SOURCES
"test/fasttrack_test.cpp")

include(GenerateExportHeader)
# we just need BGL which is header only, hence avoid errors by not importing as a target
find_package(Boost 1.65.0)

if(Boost_FOUND)
    message(STATUS "Build detector fasttrack")
    
    add_library("drace.detector.fasttrack.generic" OBJECT ${FT_SOURCES})
    set_target_properties(
        "drace.detector.fasttrack.generic" PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    target_compile_options("drace.detector.fasttrack.generic" PUBLIC "/MT$<$<CONFIG:Debug>:d>")

    target_include_directories("drace.detector.fasttrack.generic"
        PUBLIC "include"
        PUBLIC "${Boost_INCLUDE_DIR}")
   
    target_link_libraries("drace.detector.fasttrack.generic" "drace-common" "parallel-hashmap")

    ###########standalone Version#####################
    add_library("drace.detector.fasttrack.standalone" SHARED "src/fasttrack_st.cpp")

    generate_export_header("drace.detector.fasttrack.standalone" BASE_NAME fasttrack_st)

    # include exports header
    target_include_directories("drace.detector.fasttrack.standalone" PUBLIC $<TARGET_FILE_DIR:drace.detector.fasttrack.standalone>)
    target_link_libraries("drace.detector.fasttrack.standalone" PRIVATE "drace.detector.fasttrack.generic")

    install(TARGETS "drace.detector.fasttrack.standalone"
        RUNTIME DESTINATION bin/fasttrack COMPONENT Runtime
        LIBRARY DESTINATION lib/fasttrack COMPONENT Runtime
        ARCHIVE DESTINATION lib/fasttrack COMPONENT Development)
    install(FILES ${PROJECT_SOURCE_DIR}/common/detector/Detector.h DESTINATION include/fasttrack)

    # copy detector dlls to drace-client binary dir
    add_custom_command(TARGET "drace.detector.fasttrack.standalone" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:drace.detector.fasttrack.standalone>"
            "${PROJECT_BINARY_DIR}/drace-client")

    ################ configure test module ################
    if(DRACE_ENABLE_TESTING)
        # create library which is linked in global testing module
        add_library(fasttrack_test OBJECT ${FT_TEST_SOURCES})
        # use dynamic version of stdlib (has to match gtests version)
        target_compile_options(fasttrack_test PRIVATE "/MT$<$<CONFIG:Debug>:d>")
        target_link_libraries(fasttrack_test PRIVATE gtest)
        set_property(GLOBAL APPEND PROPERTY DRACE_TEST_SUITES fasttrack_test)
    endif()
endif()
