cmake_minimum_required (VERSION 3.8)

project ("drace-client" LANGUAGES "C" "CXX")

set(DRACE_ENABLE_TESTING OFF CACHE BOOL "Build Tests")
set(BUILD_CSHARP_EXAMPLES OFF CACHE BOOL "Build C# Examples")
set(DRACE_DETECTOR "tsan" CACHE STRING "Detector to be used")

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# DR performance tuning
set(DynamoRIO_FAST_IR ON CACHE BOOL "DynamoRIO_FAST_IR")

# CSharp can only be build using MSVC generators
message("Generator: ${CMAKE_GENERATOR}")
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
	message("Build CSharp Examples")
	set(BUILD_CSHARP_EXAMPLES ON)
endif()

find_package(Threads REQUIRED)
find_package(DynamoRIO 7.0 REQUIRED)

add_subdirectory("drace-client")

if(${DRACE_ENABLE_TESTING})
	message("Build Testsuite")
	# force gtest to build static lib
	set(gtest_force_shared_crt ON CACHE BOOL "static lib" FORCE)
	add_subdirectory("vendor/google/googletest/googletest")
	add_subdirectory("test")
endif()
