SET(EXAMPLE "gp-empty-main")

add_executable(${EXAMPLE} "main")
target_link_libraries(${EXAMPLE} Threads::Threads)
set_target_properties(${EXAMPLE} PROPERTIES CXX_STANDARD 11)
