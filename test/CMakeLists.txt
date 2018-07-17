set(SOURCES main algorithms DetectorTest DrIntegrationTest)
set(TEST_TARGET "${PROJECT_NAME}-Tests")


# for visual studio's gtest plugin
file(WRITE "${CMAKE_BINARY_DIR}/test/${TEST_TARGET}.exe.is_google_test" "")

add_executable(${TEST_TARGET} ${SOURCES})
set_target_properties(${TEST_TARGET} PROPERTIES CXX_STANDARD 11)

target_link_libraries(${TEST_TARGET} gtest)

target_include_directories(
	${TEST_TARGET} PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> "include")

target_include_directories(
	${TEST_TARGET} PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/drace-client/detectors> "detectors")

target_link_libraries(${TEST_TARGET} "drace-${DRACE_DETECTOR}")

# Prepare Guinea pigs
add_subdirectory("mini-apps")
