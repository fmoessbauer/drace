# SPDX-License-Identifier: MIT

SET(BENCH "bench-stream")

add_executable(${BENCH} "main")
set_target_properties(${BENCH} PROPERTIES C_STANDARD 99)

if(${DRACE_INSTALL_BENCH})
    install(TARGETS "${PROJECT_NAME}-bench" RUNTIME DESTINATION bin COMPONENT RUNTIME)
endif()
