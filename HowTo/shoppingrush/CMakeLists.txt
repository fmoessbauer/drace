# DRace, a dynamic data race detector
#
# Copyright (c) Siemens AG, 2020
#
# Authors:
#   Felix Moessbauer <felix.moessbauer@siemens.com>
#
# This work is licensed under the terms of the MIT license.  See
# the LICENSE file in the top-level directory.

set(EXAMPLE "shoppingrush")
set(EXAMPLE_SOL "shoppingrush-sol")

if(NOT TARGET Threads::Threads)
    find_package(Threads REQUIRED)
endif()

function(ConfigureExample Output Sources)
    add_executable(${Output} ${Sources})
    set_target_properties(${Output} PROPERTIES CXX_STANDARD 11)
    target_link_libraries(${Output} Threads::Threads)
    target_compile_options(${Output} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/DEBUG:FULL>")
    # we install this always, as it's part of the tutorial
    install(TARGETS ${Output} DESTINATION ${DRACE_RUNTIME_DEST})
    if(WIN32)
        install(FILES $<TARGET_PDB_FILE:${Output}> DESTINATION  ${DRACE_RUNTIME_DEST} OPTIONAL)
    endif()
endfunction()

ConfigureExample(${EXAMPLE} "ShoppingRush")
ConfigureExample(${EXAMPLE_SOL} "ShoppingRushSolution")
