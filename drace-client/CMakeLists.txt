set(SOURCES
	"src/globals"
	"src/drace-client"
	"src/function-wrapper.cpp"
	"src/function-wrapper/internal"
	"src/function-wrapper/event"
	"src/memory-tracker"
	"src/instr/instr-mem-fast"
	"src/instr/instr-mem-full"
	"src/module/Metadata"
	"src/module/Tracker"
	"src/MSR"
	"src/symbols"
	"src/util")

find_package(DynamoRIO 7.0 REQUIRED)

add_library("drace-client" SHARED ${SOURCES})
target_include_directories("drace-client" PRIVATE "include")

# Setup as dynamorio client (not-standalone)
# See https://github.com/DynamoRIO/dynamorio/blob/master/make/DynamoRIOConfig.cmake.in
# for details
configure_DynamoRIO_client("drace-client")
use_DynamoRIO_extension("drace-client" drmgr)
use_DynamoRIO_extension("drace-client" drutil)
use_DynamoRIO_extension("drace-client" drreg)
use_DynamoRIO_extension("drace-client" drwrap)
use_DynamoRIO_extension("drace-client" drsyms)

# Bind detectors
message("Use detector: ${DRACE_DETECTOR}")
add_subdirectory("detectors/${DRACE_DETECTOR}")
target_link_libraries(
	"drace-client"   # main drace-libs
	"drace-common"   # drace and msr common libs
	"jtilly-inih"    # ini parser
	"drace-detector" # race-detector
	"hh-date"        # C++ date lib
)

# TinyXML2
if(${DRACE_XML_EXPORTER})
	target_link_libraries("drace-client" "tinyxml2")
	target_compile_definitions("drace-client" PRIVATE -DXML_EXPORTER)
endif()

# Copy settings ini to root dir
add_custom_command(TARGET "drace-client" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/drace.ini"
		"${PROJECT_BINARY_DIR}/drace.ini")

if(${DRACE_XML_EXPORTER})
	add_custom_command(TARGET "drace-client" POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:tinyxml2>"
			"$<TARGET_FILE_DIR:drace-client>"
		DEPENDS "tinyxml2")
endif()

# copy detector dlls to drace-client binary dir
add_custom_command(TARGET "drace-client" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace-detector>"
		"$<TARGET_FILE_DIR:drace-client>"
	DEPENDS "drace-detector")

# copy transitive depends for tsan detector
if("${DRACE_DETECTOR}" STREQUAL "tsan")
	# Copy tsan-common
	add_custom_command(TARGET "drace-client" POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${PROJECT_SOURCE_DIR}/vendor/tsan/blob/race_windows_amd64.dll"
			"$<TARGET_FILE_DIR:drace-client>")
endif()
