add_library("drace.detector.tracebinary" SHARED "TraceBinary")
target_link_libraries("drace.detector.tracebinary" "drace-common")
configure_DynamoRIO_standalone("drace.detector.tracebinary")

install(TARGETS "drace.detector.tracebinary" RUNTIME DESTINATION bin COMPONENT Runtime)

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET "drace.detector.tracebinary" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.tracebinary>"
		"${PROJECT_BINARY_DIR}/drace-client")

