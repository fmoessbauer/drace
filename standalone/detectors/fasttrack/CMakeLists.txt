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
"test/fasttrack_test.cpp"
"src/stacktrace"
"src/varstate.cpp"
"src/threadstate.cpp")

include(GenerateExportHeader)
# we just need BGL which is header only, hence avoid errors by not importing as a target
find_package(Boost 1.65.0)

if(Boost_FOUND)
    message(STATUS "Build detector fasttrack")
    
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_definitions(-DCOMPILE_X86)
        message(STATUS "Building for x86 Plattform")
    endif()
    
    add_library("drace.detector.fasttrack.generic" OBJECT ${FT_SOURCES})
    set_target_properties(
        "drace.detector.fasttrack.generic" PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED OFF)

    if(POLICY CMP0091)
        set_target_properties(
            "drace.detector.fasttrack.generic" PROPERTIES
            # use static runtime (required by dynamorio)
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        target_compile_options("drace.detector.fasttrack.generic" PRIVATE "/MT$<$<CONFIG:Debug>:d>")
    endif()

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
    set_target_properties(
        "drace.detector.fasttrack.standalone" PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

    if(POLICY CMP0091)
        set_target_properties(
            "drace.detector.fasttrack.standalone" PROPERTIES
            # use static runtime (required by dynamorio)
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        target_compile_options("drace.detector.fasttrack.standalone" PRIVATE "/MT$<$<CONFIG:Debug>:d>")
    endif()

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
        target_link_libraries(fasttrack_test PRIVATE gtest "drace-common" "parallel-hashmap")
        target_include_directories(fasttrack_test
        PUBLIC "include"
        PUBLIC "${Boost_INCLUDE_DIR}")
        set_property(GLOBAL APPEND PROPERTY DRACE_TEST_SUITES fasttrack_test)
    endif()
endif()
