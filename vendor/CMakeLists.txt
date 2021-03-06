# External CMake vendor targets

# tsan + wrapper
find_library(TSAN_LIB NAMES "race_windows_amd64" HINTS "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/" NO_DEFAULT_PATH)
add_library("tsan-common" INTERFACE)
target_include_directories("tsan-common" INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tsan/include)
target_link_libraries("tsan-common" INTERFACE ${TSAN_LIB})
install(FILES "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/race_windows_amd64.dll" DESTINATION bin)
file(READ "tsan/LICENSE.txt" LIC_FILE_TSAN)
file(APPEND ${LIC_FILE} "${LIC_SEP}LLVM-ThreadSanitizer\n\n${LIC_FILE_TSAN}")

# inih is mandatory
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jtilly/inih/INIReader.h")
	message(FATAL_ERROR "inih submodule not available")
endif()
add_library("jtilly-inih" INTERFACE)
target_include_directories("jtilly-inih" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/jtilly/inih>)
file(READ "jtilly/inih/LICENSE.txt" LIC_FILE_INIH)
file(APPEND ${LIC_FILE} "${LIC_SEP}jtilly/inih\n\n${LIC_FILE_INIH}")

# TinyXML2 library for Valkyrie output
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/leethomason/tinyxml2/tinyxml2.cpp")
	add_library("tinyxml2" SHARED "${CMAKE_CURRENT_SOURCE_DIR}/leethomason/tinyxml2/tinyxml2.cpp")
	target_include_directories("tinyxml2" INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/leethomason/tinyxml2/)
	install(TARGETS "tinyxml2" DESTINATION bin)
	file(READ "license_tinyxml2.tpl" LIC_FILE_TINYXML2)
	file(APPEND ${LIC_FILE} "${LIC_SEP}leethomason/tinyxml2\n\n${LIC_FILE_TINYXML2}")
else()
	set(DRACE_XML_EXPORTER OFF CACHE BOOL "Build with Valkyrie compatible XML exporter" FORCE)
endif()

# HowardHinnant Date
# shipped CMake script does not work, include as header-only
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/HowardHinnant/date/include")
	message(FATAL_ERROR "HowardHinnant Date submodule not available")
endif()
add_library("hh-date" INTERFACE)
target_include_directories("hh-date" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/HowardHinnant/date/include>)
file(READ "HowardHinnant/date/LICENSE.txt" LIC_FILE_HHDATE)
file(APPEND ${LIC_FILE} "${LIC_SEP}HowardHinnant/date\n\n${LIC_FILE_HHDATE}")

# muellan clipp
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/muellan/clipp/include")
	message(FATAL_ERROR "HowardHinnant Date submodule not available")
endif()
add_library("clipp" INTERFACE)
target_include_directories("clipp" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/muellan/clipp/include>)
file(READ "muellan/clipp/LICENSE" LIC_FILE_CLIPP)
file(APPEND ${LIC_FILE} "${LIC_SEP}muellan/clipp\n\n${LIC_FILE_CLIPP}")

if(${DRACE_DETECTOR} STREQUAL "extsan")
	# greg7mdp sparsepp
	add_library("sparsepp" INTERFACE)
	target_include_directories("sparsepp" INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/greg7mdp/sparsepp>)
	file(READ "greg7mdp/sparsepp/LICENSE" LIC_FILE_SPARSEPP)
	file(APPEND ${LIC_FILE} "${LIC_SEP}greg7mdp/sparsepp\n\n${LIC_FILE_SPARSEPP}")
endif()

# gabime/spdlog for logging
add_library("spdlog" INTERFACE)
target_include_directories("spdlog" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gabime/spdlog/include>)
file(READ "gabime/spdlog/LICENSE" LIC_FILE_SPDLOG)
file(APPEND ${LIC_FILE} "${LIC_SEP}gabime/spdlog\n\n${LIC_FILE_SPDLOG}")

if(${DRACE_ENABLE_TESTING} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/google/googletest/googletest/CMakeLists.txt")
	# force gtest to build static lib
	set(gtest_force_shared_crt ON CACHE BOOL "static lib" FORCE)
	add_subdirectory("google/googletest/googletest")
else()
	set(DRACE_ENABLE_TESTING OFF CACHE BOOL "Enable Testing" FORCE)
endif()

if(${DRACE_ENABLE_BENCH} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/google/benchmark/CMakeLists.txt")
	# force gtest to build static lib
	set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
	set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Do not install benchmark" FORCE)
	add_subdirectory("google/benchmark")
else()
	set(DRACE_ENABLE_BENCH OFF CACHE BOOL "Enable benchmark" FORCE)
endif()