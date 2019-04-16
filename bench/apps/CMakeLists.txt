add_subdirectory(stream)

if(${DRACE_INSTALL_BENCH})
    install(TARGETS "bench-stream" RUNTIME DESTINATION bin COMPONENT RUNTIME)
endif()
