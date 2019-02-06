set(SOURCES
	"src/main.cpp"
	"src/Controller.cpp"
	"src/ManagedResolver.cpp"
	"src/ProtocolHandler.cpp")

add_executable("msr" ${SOURCES})
target_include_directories("msr" PRIVATE "include")
target_link_libraries("msr" "drace-common" "spdlog")

# Link with tsan if detector is Extsan
if(${DRACE_DETECTOR} STREQUAL "extsan")
	message("use tsan in msr")
	target_compile_definitions("msr" PRIVATE -DEXTSAN)

	target_sources("msr" PRIVATE "src/QueueHandler.cpp")
	target_link_libraries("msr" "tsan-common" "sparsepp")

	add_custom_command(TARGET "msr" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/vendor/tsan/blob/race_windows_amd64.dll"
		"$<TARGET_FILE_DIR:msr>")
endif()
