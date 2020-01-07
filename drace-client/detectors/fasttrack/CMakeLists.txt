# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2019
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.

# NOTE: We link against the generic fasttrack module
if(TARGET "drace.detector.fasttrack.generic")
    message(STATUS "Build detector Fasttrack-DRace")
    ########### FT DRACE Version ###########
    add_library("drace.detector.fasttrack" SHARED "src/fasttrack_dr.cpp")
    configure_DynamoRIO_standalone("drace.detector.fasttrack")

    generate_export_header("drace.detector.fasttrack" BASE_NAME fasttrack_dr)

    # include exports header
    target_include_directories("drace.detector.fasttrack" PUBLIC $<TARGET_FILE_DIR:drace.detector.fasttrack>)
    target_link_libraries("drace.detector.fasttrack" "drace.detector.fasttrack.generic")

    install(TARGETS "drace.detector.fasttrack" RUNTIME DESTINATION bin COMPONENT Runtime)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fasttrack_dr_export.h DESTINATION include)

    # copy detector dlls to drace-client binary dir
    add_custom_command(TARGET "drace.detector.fasttrack" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:drace.detector.fasttrack>"
            "${PROJECT_BINARY_DIR}/drace-client")
endif()