SET(EXAMPLE "gp-annotations")

add_executable(${EXAMPLE} "main")
target_link_libraries(${EXAMPLE} Threads::Threads)
set_target_properties(${EXAMPLE} PROPERTIES CXX_STANDARD 11)
target_compile_definitions(${EXAMPLE} PRIVATE "-DDRACE_ANNOTATION")

add_executable("${EXAMPLE}-racy" "main")
target_link_libraries(${EXAMPLE} Threads::Threads)
set_target_properties(${EXAMPLE} PROPERTIES CXX_STANDARD 11)