set(SOURCES 
	"src/main.cpp"
	"src/DetectorTest.cpp"
	"src/DrIntegrationTest.cpp"
	"src/ShmDriver.cpp")

set(TEST_TARGET "drace-tests")

# for visual studio's gtest plugin
file(WRITE "${CMAKE_BINARY_DIR}/test/${TEST_TARGET}.exe.is_google_test" "")

add_executable(${TEST_TARGET} ${SOURCES})
target_include_directories(${TEST_TARGET} PRIVATE "include")

target_link_libraries(${TEST_TARGET} gtest "drace-detector" "drace-common")
# enable dr mocks
target_compile_definitions(${TEST_TARGET} PRIVATE -DTESTING)
add_dependencies(${TEST_TARGET} "drace-client")

add_custom_target(copy_files_testing ALL
    DEPENDS "${PROJECT_SOURCE_DIR}/drace.ini"
    DEPENDS "$<TARGET_FILE:drace-detector>"
)

add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/test/data/drace_excl.ini"
		"$<TARGET_FILE_DIR:drace-tests>/data/drace_excl.ini")

if("${DRACE_DETECTOR}" STREQUAL "tsan")
	# Copy tsan-common
	add_custom_command(TARGET copy_files_testing POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${TSAN_BINARY_DIR}/race_windows_amd64.dll"
			"$<TARGET_FILE_DIR:drace-tests>")
endif()

# copy detector dll
add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace-detector>"
		"$<TARGET_FILE_DIR:drace-tests>"
	DEPENDS "drace-detector")

# copy gtest shared lib
add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:gtest>"
		"$<TARGET_FILE_DIR:drace-tests>")

# Prepare Guinea pigs
add_subdirectory("mini-apps")
