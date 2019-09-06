add_library("drace.detector.fasttrack" SHARED "fasttrack")
target_link_libraries("drace.detector.fasttrack" "drace-common")
install(TARGETS "drace.detector.fasttrack" RUNTIME DESTINATION bin COMPONENT Runtime)

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET "drace.detector.fasttrack" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.fasttrack>"
		"${PROJECT_BINARY_DIR}/drace-client")
