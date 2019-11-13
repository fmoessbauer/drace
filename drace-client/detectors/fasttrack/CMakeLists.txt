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
"src/stacktrace"
"src/varstate"
"src/threadstate"
)


include(GenerateExportHeader)
# we just need BGL which is header only, hence avoid errors by not importing as a target
find_package(Boost 1.65.0)

if(Boost_FOUND)
    message(STATUS "Build detector fasttrack")
    
    add_library("drace.detector.fasttrack.generic" SHARED ${SOURCES})


    set_target_properties(
        "drace.detector.fasttrack.generic" PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

    target_include_directories("drace.detector.fasttrack.generic"
        PUBLIC "include"
        PUBLIC "${Boost_INCLUDE_DIR}")
    
   
    target_link_libraries("drace.detector.fasttrack.generic" "drace-common")
    target_link_libraries("drace.detector.fasttrack.generic" "parallel-hashmap")

    add_custom_command(TARGET "drace.detector.fasttrack.generic" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:drace.detector.fasttrack.generic>"
        "${PROJECT_BINARY_DIR}/drace-client")

    ###########DRACE Version#####################
    add_library("drace.detector.fasttrack" SHARED ${SOURCES} "src/fasttrack_dr.cpp")
    configure_DynamoRIO_standalone("drace.detector.fasttrack")

    generate_export_header(
        "drace.detector.fasttrack"
        BASE_NAME fasttrack_dr)

    # include exports header
    target_include_directories("drace.detector.fasttrack" PUBLIC $<TARGET_FILE_DIR:drace.detector.fasttrack>)
    target_include_directories("drace.detector.fasttrack" PRIVATE "include")
    target_link_libraries("drace.detector.fasttrack" "drace.detector.fasttrack.generic")

    install(TARGETS "drace.detector.fasttrack" RUNTIME DESTINATION bin COMPONENT Runtime)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fasttrack_dr_export.h DESTINATION include)

    # copy detector dlls to drace-client binary dir
    add_custom_command(TARGET "drace.detector.fasttrack" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:drace.detector.fasttrack>"
            "${PROJECT_BINARY_DIR}/drace-client")


    ###########standalone Version#####################
    add_library("drace.detector.fasttrack.standalone" SHARED ${SOURCES} "src/fasttrack_st.cpp")
    
    target_include_directories("drace.detector.fasttrack.standalone"
    PRIVATE "include")

    generate_export_header(
        "drace.detector.fasttrack.standalone"
        BASE_NAME fasttrack_st)

    # include exports header
    target_include_directories("drace.detector.fasttrack.standalone" PUBLIC $<TARGET_FILE_DIR:drace.detector.fasttrack.standalone>)
    target_compile_definitions("drace.detector.fasttrack.standalone" PUBLIC -DSTD_MUTEX)
    target_link_libraries("drace.detector.fasttrack.standalone" "drace.detector.fasttrack.generic")

    install(TARGETS "drace.detector.fasttrack.standalone" RUNTIME DESTINATION bin COMPONENT Runtime)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fasttrack_st_export.h DESTINATION include)

    # copy detector dlls to drace-client binary dir
    add_custom_command(TARGET "drace.detector.fasttrack.standalone" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:drace.detector.fasttrack.standalone>"
            "${PROJECT_BINARY_DIR}/drace-client")

endif()
