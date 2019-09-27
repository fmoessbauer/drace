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

find_package(PythonInterp)
include(UsePyInstaller)

if(PythonInterp_FOUND)
    get_filename_component(PYTHON_ROOT "${PYTHON_EXECUTABLE}" DIRECTORY)
    find_file(
      PyInstaller_EXECUTABLE
      NAME "pyinstaller.exe"
      HINTS "${PYTHON_ROOT}/scripts"
      DOC "Path to the pyinstaller directory (where to find pyinstaller.exe)"
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(PyInstaller
      FOUND_VAR PyInstaller_FOUND
      REQUIRED_VARS
        PyInstaller_EXECUTABLE
      VERSION_VAR Foo_VERSION
    )
endif()
