# External CMake vendor targets

# tsan + wrapper
find_library(TSAN_LIB NAMES "race_windows_amd64" HINTS "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/" NO_DEFAULT_PATH)
add_library("tsan-common" INTERFACE)
target_link_libraries("tsan-common" INTERFACE ${TSAN_LIB})
target_include_directories("drace-common"
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tsan/include>)


add_library("jtilly-inih" INTERFACE)
target_include_directories("jtilly-inih" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/jtilly/inih>)

# TinyXML2 library for Valkyrie output
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build static version of TinyXML2" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "disable tests" FORCE)
add_subdirectory("leethomason/tinyxml2")

# HowardHinnant Date
# shipped CMake script does not work, include as header-only
add_library("hh-date" INTERFACE)
target_include_directories("hh-date" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/HowardHinnant/date/include>)

# greg7mdp sparsepp
add_library("sparsepp" INTERFACE)
target_include_directories("sparsepp" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/greg7mdp/sparsepp>)

# gabime/spdlog for logging
add_library("spdlog" INTERFACE)
target_include_directories("spdlog" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gabime/spdlog/include>)

if(${DRACE_ENABLE_TESTING})
	# force gtest to build static lib
	set(gtest_force_shared_crt ON CACHE BOOL "static lib" FORCE)
	add_subdirectory("google/googletest/googletest")
endif()

if(${DRACE_ENABLE_BENCH})
	# force gtest to build static lib
	set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
	add_subdirectory("google/benchmark")
endif()