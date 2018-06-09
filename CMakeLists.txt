cmake_minimum_required (VERSION 3.8)

project ("drace-client")

set(DRACE_ENABLE_TESTING 0 CACHE STRING "Build Tests")

find_package(Threads REQUIRED)
find_package(DynamoRIO 7.0 REQUIRED)

add_subdirectory ("drace-client")

if(${DRACE_ENABLE_TESTING})
	message("Build Testsuite")
	# force gtest to build static lib
	set(gtest_force_shared_crt ON CACHE BOOL "static lib" FORCE)
	add_subdirectory("vendor/google/googletest/googletest")
	add_subdirectory ("test")
endif()
