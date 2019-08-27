add_library("drace.detector.tsan" SHARED "tsan")
target_link_libraries("drace.detector.tsan" "tsan-common" "drace-common")
install(TARGETS "drace.detector.tsan" RUNTIME DESTINATION bin COMPONENT Runtime)

add_custom_target(copy_detector_tsan DEPENDS "drace.detector.tsan")

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET copy_detector_tsan POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.tsan>"
		"${PROJECT_BINARY_DIR}")

# copy transitive depends for tsan detector
# Copy tsan-common
add_custom_command(TARGET copy_detector_tsan POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${TSAN_BINARY_DIR}/race_windows_amd64.dll"
		"${PROJECT_BINARY_DIR}")
