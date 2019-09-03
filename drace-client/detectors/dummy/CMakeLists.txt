add_library("drace.detector.dummy" SHARED "dummy")
target_link_libraries("drace.detector.dummy" "drace-common")
install(TARGETS "drace.detector.dummy" RUNTIME DESTINATION bin COMPONENT Runtime)

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET "drace.detector.dummy" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.dummy>"
		"${PROJECT_BINARY_DIR}/drace-client")
