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
# Note: This tutorial can be build directly by pointing CMake
# to this folder

cmake_minimum_required (VERSION 3.8)
project ("drace-howto" LANGUAGES "C" "CXX")

# put demo application into samples folder
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()
if(NOT DRACE_RUNTIME_DEST)
    set(DRACE_RUNTIME_DEST "bin")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/samples")
set(DRACE_RUNTIME_DEST "${DRACE_RUNTIME_DEST}/samples")

add_subdirectory(shoppingrush)
