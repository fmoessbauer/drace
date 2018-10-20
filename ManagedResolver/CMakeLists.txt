set(SOURCES
	"src/main.cpp"
	"src/Controller.cpp"
	"src/ManagedResolver.cpp"
	"src/ProtocolHandler.cpp"
	"src/QueueHandler.cpp")

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

add_custom_command(TARGET "msr" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/vendor/debug"
		"$<TARGET_FILE_DIR:msr>")
