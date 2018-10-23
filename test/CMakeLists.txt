set(SOURCES 
	"src/main.cpp"
	"src/algorithms.cpp"
	"src/DetectorTest.cpp"
	"src/DrIntegrationTest.cpp"
	"src/ShmDriver.cpp")

set(TEST_TARGET "drace-tests")

# for visual studio's gtest plugin
file(WRITE "${CMAKE_BINARY_DIR}/test/${TEST_TARGET}.exe.is_google_test" "")

add_executable(${TEST_TARGET} ${SOURCES})
target_include_directories(${TEST_TARGET} PRIVATE "include")

target_link_libraries(${TEST_TARGET} gtest "drace-detector" "drace-common")
add_dependencies(${TEST_TARGET} "drace-client")

add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/test/data/drace_excl.ini"
		"$<TARGET_FILE_DIR:drace-tests>/data/drace_excl.ini")

if("${DRACE_DETECTOR}" STREQUAL "tsan")
	# Copy tsan-common
	add_custom_command(TARGET "drace-tests" POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${PROJECT_SOURCE_DIR}/vendor/tsan/blob/race_windows_amd64.dll"
			"$<TARGET_FILE_DIR:drace-tests>")
endif()

# copy detector dll
add_custom_command(TARGET "drace-tests" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace-detector>"
		"$<TARGET_FILE_DIR:drace-tests>"
	DEPENDS "drace-detector")

# copy gtest shared lib
add_custom_command(TARGET "drace-tests" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:gtest>"
		"$<TARGET_FILE_DIR:drace-tests>")

# Prepare Guinea pigs
add_subdirectory("mini-apps")
