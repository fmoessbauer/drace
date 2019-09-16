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

# Bind detectors prior to first "configure_DynamoRIO_client"
# as DR changes global vars
add_subdirectory("detectors")

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

target_link_libraries(
	"drace-client"   # main drace-libs
	"drace-common"   # drace and msr common libs
	"jtilly-inih"    # ini parser
	"hh-date"        # C++ date lib
	"clipp"          # option parser
)

# Set loglevel
target_compile_definitions("drace-client" PRIVATE -DLOGLEVEL=${DRACE_LOGLEVEL})
# we need a specific base addr, hence disable this warning
target_link_libraries("drace-client" -ignore:4281)
target_compile_options("drace-client" PRIVATE -EHsc)

# TinyXML2
if(${DRACE_XML_EXPORTER})
	target_link_libraries("drace-client" "tinyxml2")
	target_compile_definitions("drace-client" PRIVATE -DXML_EXPORTER)
endif()

if(${DRACE_USE_LEGACY_API})
    target_compile_definitions("drace-client" PRIVATE -DDRACE_USE_LEGACY_API)
endif()

# Copy settings ini to root dir
add_custom_target(copy_files ALL
    DEPENDS "${PROJECT_SOURCE_DIR}/drace.ini"
)

add_custom_command(TARGET copy_files POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/drace.ini"
		"${PROJECT_BINARY_DIR}/drace.ini")

if(${DRACE_XML_EXPORTER})
    add_dependencies(copy_files tinyxml2)
	add_custom_command(TARGET copy_files POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:tinyxml2>"
			"$<TARGET_FILE_DIR:drace-client>"
		DEPENDS "tinyxml2")
endif()

if(${DRACE_ENABLE_CPPCHECK})
    set_target_properties("drace-client" PROPERTIES
        CXX_CPPCHECK ${DRACE_CPPCHECK_CALL})
endif()

# Add licensing information on LCM
file(READ "include/lcm/LICENSE" LIC_FILE_LCM)
file(APPEND ${LIC_FILE} "${LIC_SEP}fmoessbauer/LossyCountingModel\n\n${LIC_FILE_LCM}")

install(TARGETS "drace-client" RUNTIME DESTINATION bin COMPONENT Runtime)
