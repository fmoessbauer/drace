add_library("drace.detector.tsan" SHARED "tsan")
target_link_libraries("drace.detector.tsan"
    PRIVATE "tsan-common"
    PRIVATE "drace-common")
install(TARGETS "drace.detector.tsan" RUNTIME DESTINATION bin COMPONENT Runtime)

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET "drace.detector.tsan" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.tsan>"
		"${PROJECT_BINARY_DIR}/drace-client")

# copy transitive depends for tsan detector
# Copy tsan-common
add_custom_command(TARGET "drace.detector.tsan" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${TSAN_BINARY_DIR}/race_windows_amd64.dll"
		"${PROJECT_BINARY_DIR}/drace-client")
