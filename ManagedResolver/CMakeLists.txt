set(SOURCES
	"src/main.cpp"
	"src/Controller.cpp"
	"src/ManagedResolver.cpp"
	"src/ProtocolHandler.cpp")

add_executable("msr" ${SOURCES})
target_include_directories(
	"msr" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Inter Process Communication Headers
target_include_directories(
	"msr" PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/common>)

# gabime/spdlog for logging
target_include_directories(
	"msr" PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/gabime/spdlog/include>)

# Link with tsan if detector is Extsan
if(${DRACE_DETECTOR} STREQUAL "extsan")
	message("use tsan in msr")
	target_compile_definitions("msr" PRIVATE -DEXTSAN)

	find_library(TSAN_LIB NAMES "race_windows_amd64" HINTS "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/" NO_DEFAULT_PATH)

	target_sources("msr" PRIVATE "src/QueueHandler.cpp")
	target_link_libraries("msr" "${TSAN_LIB}")

	target_include_directories("msr" PRIVATE
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/tsan/include>)

	target_include_directories("msr" PRIVATE
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/greg7mdp/sparsepp>)

	add_custom_command(TARGET "msr" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/vendor/tsan/blob/race_windows_amd64.dll"
		"$<TARGET_FILE_DIR:msr>")
endif()


add_custom_command(TARGET "msr" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/vendor/debug"
		"$<TARGET_FILE_DIR:msr>")
