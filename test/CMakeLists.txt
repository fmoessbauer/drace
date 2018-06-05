set(SOURCES main algorithms)
set(TEST_TARGET "${PROJECT_NAME}-test")

add_executable(${TEST_TARGET} ${SOURCES})
set_target_properties(${TEST_TARGET} PROPERTIES CXX_STANDARD 11)

target_link_libraries(${TEST_TARGET} gtest)
