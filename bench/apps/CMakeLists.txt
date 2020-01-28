add_subdirectory(stream)

if(DRACE_INSTALL_BENCH)
    install(TARGETS "bench-stream" DESTINATION ${DRACE_RUNTIME_DEST})
endif()
