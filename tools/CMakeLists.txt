# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2019
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.

# Install drace gui
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ReportConverter/ReportConverter.py DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ReportConverter/resources/ DESTINATION resources)
