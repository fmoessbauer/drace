# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2018
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.
#
# SPDX-License-Identifier: MIT
#
# Find PyInstaller

macro(add_pi_executable PI_TARGET_NAME PYTHON_FILENAME)
    set(exe "${CMAKE_CURRENT_BINARY_DIR}/${PI_TARGET_NAME}.exe")
    add_custom_target(${PI_TARGET_NAME} ALL DEPENDS ${exe})
    set(source_winpath "${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_FILENAME}")
    string(REGEX REPLACE "/" "\\\\" source_winpath "${source_winpath}")

    add_custom_command(
        OUTPUT ${exe}
        DEPENDS ${PYTHON_FILENAME}
        COMMAND ${PyInstaller_EXECUTABLE} ARGS -F --distpath "${CMAKE_CURRENT_BINARY_DIR}"
        "${source_winpath}" VERBATIM)
endmacro()
