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
    # \todo port executor to linux
    add_subdirectory(DRaceGUI)
endif()

add_subdirectory(ReportConverter)
