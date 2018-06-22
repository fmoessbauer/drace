add_library("drace-dummy" SHARED "dummy")
set_target_properties("drace-dummy" PROPERTIES CXX_STANDARD 11)

add_custom_command(TARGET "drace-dummy" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"drace-dummy.dll"
		"${PROJECT_BINARY_DIR}/drace-client/drace-dummy.dll")