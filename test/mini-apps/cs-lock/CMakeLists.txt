SET(EXAMPLE "gp-cs-lock")

enable_language("CSharp")

add_executable(${EXAMPLE} "main.cs")
#target_link_libraries(${EXAMPLE} Threads::Threads)
