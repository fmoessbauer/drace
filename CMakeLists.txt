cmake_minimum_required (VERSION 3.8)

project ("drace-client" LANGUAGES "C" "CXX")

set(DRACE_ENABLE_TESTING OFF CACHE STRING "Build Tests")
set(BUILD_CSHARP_EXAMPLES OFF CACHE STRING "Build C# Examples")

# CSharp can only be build using MSVC generators
message("Generator: ${CMAKE_GENERATOR}")
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
	message("Build CSharp Examples")
	set(BUILD_CSHARP_EXAMPLES ON)
endif()

find_package(Threads REQUIRED)
find_package(DynamoRIO 7.0 REQUIRED)

add_subdirectory("vendor/tsan")
add_subdirectory("drace-client/detectors/tsan")
add_subdirectory ("drace-client")

if(${DRACE_ENABLE_TESTING})
	message("Build Testsuite")
	# force gtest to build static lib
	set(gtest_force_shared_crt ON CACHE BOOL "static lib" FORCE)
	add_subdirectory("vendor/google/googletest/googletest")
	add_subdirectory ("test")
endif()
