# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2019
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.

if(WIN32)
    find_package(PyInstaller)

    if(PyInstaller_FOUND)
        message(STATUS "ReportConverter: Create self-contained binary")

        add_pi_executable(ReportConverter ReportConverter.py)
        file(COPY "resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ReportConverter.exe DESTINATION ${DRACE_RUNTIME_DEST})
    endif()
endif()

file(COPY "ReportConverter.py" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# Install report converter
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ReportConverter.py DESTINATION ${DRACE_RUNTIME_DEST})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION resources)
