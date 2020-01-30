SET(EXAMPLE "shoppingrush")
SET(EXAMPLE_SOL "shoppingrush-sol")

add_executable(${EXAMPLE} "ShoppingRush")
add_executable(${EXAMPLE_SOL} "ShoppingRushSolution")
set_target_properties(${EXAMPLE} PROPERTIES CXX_STANDARD 11)
target_link_libraries(${EXAMPLE} Threads::Threads)
target_link_libraries(${EXAMPLE_SOL} Threads::Threads)

if(${DRACE_INSTALL_TESTS})
    install(TARGETS ${EXAMPLE} DESTINATION bin)
    install(TARGETS ${EXAMPLE_SOL} DESTINATION bin)
endif()
