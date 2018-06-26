find_library(TSAN_LIB NAMES "race_windows_amd64" HINTS "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/" NO_DEFAULT_PATH)

add_library("drace-tsan" SHARED "tsan")
set_target_properties("drace-tsan" PROPERTIES CXX_STANDARD 11)
target_include_directories("drace-tsan" PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/tsan/include> "include")

target_link_libraries("drace-tsan" PRIVATE ${TSAN_LIB} Threads::Threads)

add_custom_command(TARGET "drace-tsan" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/vendor/tsan/blob/race_windows_amd64.dll"
		"${PROJECT_BINARY_DIR}/drace-client/race_windows_amd64.dll"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"drace-tsan.dll"
		"${PROJECT_BINARY_DIR}/drace-client/drace-tsan.dll")
