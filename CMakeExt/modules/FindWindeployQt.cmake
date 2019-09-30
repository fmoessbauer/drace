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
# Find WindeployQt

find_package(Qt5Core)

if(Qt5Core_FOUND)
    # Retrieve the absolute path to qmake and then use that path to find
    # the windeployqt binary
    get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(WindeployQt
      FOUND_VAR WindeployQt_FOUND
      REQUIRED_VARS
        WINDEPLOYQT_EXECUTABLE
    )

endif()

function(windeployqt target directory)

    message(STATUS "Deploy Qt application ${target}")

	# execute windeployqt in a temp directory after build
    add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
		COMMAND set PATH="${_qt_bin_dir}"
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" --verbose 0 --no-compiler-runtime --no-translations --no-angle --release --no-opengl-sw "$<TARGET_FILE:${target}>"
    )

	# copy deployment directory during installation
    install(
        DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
        DESTINATION ${directory}
    )

endfunction()
