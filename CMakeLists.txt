cmake_minimum_required (VERSION 3.8)

project ("drace-client" LANGUAGES "C" "CXX")

set(DRACE_ENABLE_TESTING OFF CACHE BOOL "Build Tests")
set(BUILD_CSHARP_EXAMPLES OFF CACHE BOOL "Build C# Examples")
set(DRACE_DETECTOR "tsan" CACHE STRING "Detector to be used")
set(DRACE_XML_EXPORTER ON CACHE BOOL "Build with Valkyrie compatible XML exporter")
set(DRACE_LOGLEVEL "2" CACHE STRING "Set loglevel of DRace (0: error, 1: warning, ... 4: trace)")

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# DR performance tuning
set(DynamoRIO_FAST_IR ON CACHE BOOL "DynamoRIO_FAST_IR")

# CSharp can only be build using MSVC generators
message("Generator: ${CMAKE_GENERATOR}")
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
	message("Build CSharp Examples")
	set(BUILD_CSHARP_EXAMPLES ON)
endif()

find_package(Threads REQUIRED)

# Common Files
add_subdirectory("common")
# external vendor targets
add_subdirectory("vendor")
# DRACE
add_subdirectory("drace-client")
# Managed Stack Resolver
add_subdirectory("ManagedResolver")

if(${DRACE_ENABLE_TESTING})
	message("Build Testsuite")
	add_subdirectory("test")
endif()

if(${DRACE_ENABLE_BENCH})
	message("Build Benchmarks")
	add_subdirectory("bench")
endif()

# Build documentation
find_package(Doxygen)
if(${Doxygen_FOUND})
	message("Build Documentation")
	set(DOXYGEN_GENERATE_HTML YES)
	set(DOXYGEN_GENERATE_MAN NO)
	set(DOXYGEN_OUTPUT_DIRECTORY docs)
	set(DOXYGEN_EXCLUDE_PATTERNS 
		"*DynamoRIO*"
		"${PROJECT_SOURCE_DIR}/ManagedResolver/include/prebuild/*"
		"${PROJECT_SOURCE_DIR}/test/*"
		"${PROJECT_SOURCE_DIR}/bench/*"
		"${PROJECT_SOURCE_DIR}/vendor/*")
	set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "README.md")
	
	# work around https://gitlab.kitware.com/cmake/cmake/issues/18738
	set(DOXYGEN_LATEX_MAKEINDEX_CMD makeindex)

	doxygen_add_docs(
	    doc
	    ${PROJECT_SOURCE_DIR}
	    COMMENT "Generate Code Documentation")
endif()

install(FILES "drace.ini" DESTINATION bin)
install(FILES "LICENSE" DESTINATION doc)
install(FILES "README.md" DESTINATION doc)
