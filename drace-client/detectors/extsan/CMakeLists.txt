add_library("drace-extsan" SHARED "extsan")
set_target_properties("drace-extsan" PROPERTIES CXX_STANDARD 11)

target_include_directories(
	"drace-extsan" PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/common>)

add_custom_command(TARGET "drace-extsan" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"drace-extsan.dll"
		"${PROJECT_BINARY_DIR}/drace-client/drace-extsan.dll"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"drace-extsan.dll"
		"${PROJECT_BINARY_DIR}/test/drace-extsan.dll")