set(SOURCES
	"src/globals"
	"src/drace-client"
	"src/function-wrapper"
	"src/mutex"
	"src/memory-tracker"
	"src/module-tracker"
	"src/symbols"
	"src/util")

add_library("drace-client" SHARED ${SOURCES})
target_include_directories(
	"drace-client" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/drace-client> "include")

target_include_directories(
	"drace-client" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/drace-client> "detectors")

# Parser for config files
target_include_directories(
	"drace-client" PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/jtilly/inih> "inih")


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
target_link_libraries("drace-client" "drace-${DRACE_DETECTOR}")

# TinyXML2
target_link_libraries("drace-client" "tinyxml2")
target_include_directories("drace-client" PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/leethomason/tinyxml2> "tinyxml2")

# HowardHinnant Date
target_include_directories("drace-client" PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/HowardHinnant/date/include/date> "date")

# Prepare Binary Dir
# Copy to test dir
add_custom_command(TARGET "drace-client" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/drace.ini"
		"${PROJECT_BINARY_DIR}/drace.ini"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_BINARY_DIR}/vendor/leethomason/tinyxml2/tinyxml2.dll"
		"${PROJECT_BINARY_DIR}/drace-tsan/tinyxml2.dll"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_BINARY_DIR}/vendor/leethomason/tinyxml2/tinyxml2.pdb"
		"${PROJECT_BINARY_DIR}/drace-tsan/tinyxml2.pdb")
