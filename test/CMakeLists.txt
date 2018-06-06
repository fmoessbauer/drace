set(SOURCES main algorithms)
set(TEST_TARGET "${PROJECT_NAME}-Tests")


# for visual studio's gtest plugin
file(WRITE "${CMAKE_BINARY_DIR}/test/${TEST_TARGET}.exe.is_google_test" "")

add_executable(${TEST_TARGET} ${SOURCES})
set_target_properties(${TEST_TARGET} PROPERTIES CXX_STANDARD 11)

target_link_libraries(${TEST_TARGET} gtest)

# Prepare Guinea pigs
add_subdirectory("mini-apps")
