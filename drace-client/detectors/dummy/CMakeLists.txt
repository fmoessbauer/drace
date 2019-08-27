add_library("drace.detector.dummy" SHARED "dummy")
target_link_libraries("drace.detector.dummy" "drace-common")

add_custom_target(copy_detector_dummy DEPENDS "drace.detector.dummy")

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET copy_detector_dummy POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.dummy>"
		"${PROJECT_BINARY_DIR}")
