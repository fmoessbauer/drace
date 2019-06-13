SET(EXAMPLE "gp-memchecker")

add_executable(${EXAMPLE} "main")
target_link_libraries(${EXAMPLE} Threads::Threads)
set_target_properties(${EXAMPLE} PROPERTIES CXX_STANDARD 11)

if(${DRACE_INSTALL_TESTS})
    install(TARGETS ${EXAMPLE} DESTINATION bin)
endif()
