SET(EXAMPLE "gp-concurrent-inc")

add_executable(${EXAMPLE} "main")
target_link_libraries(${EXAMPLE} Threads::Threads)
set_target_properties(${EXAMPLE} PROPERTIES CXX_STANDARD 11)

add_executable("${EXAMPLE}-static" "main")
target_link_libraries("${EXAMPLE}-static" Threads::Threads)
set_target_properties("${EXAMPLE}-static" PROPERTIES CXX_STANDARD 11)
target_compile_definitions("${EXAMPLE}-static" PRIVATE USE_STATIC)

if(DRACE_INSTALL_TESTS)
    install(TARGETS ${EXAMPLE} DESTINATION ${DRACE_RUNTIME_DEST})
    install(TARGETS "${EXAMPLE}-static" DESTINATION ${DRACE_RUNTIME_DEST})
endif()
