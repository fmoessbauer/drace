SET(EXAMPLE "gp-single-thread")

add_executable(${EXAMPLE} "main")
set_target_properties(${EXAMPLE} PROPERTIES CXX_STANDARD 11)

if(DRACE_INSTALL_TESTS)
    install(TARGETS ${EXAMPLE} DESTINATION ${DRACE_RUNTIME_DEST})
endif()
