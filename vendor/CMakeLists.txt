# External CMake vendor targets
if(DRACE_ENABLE_RUNTIME)
	# tsan + wrapper
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		message(STATUS "Use debug version of TSAN backend")
		set(TSAN_BINARY_DIR "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/debug" CACHE INTERNAL "path to tsan binary")
	else()
		message(STATUS "Use release version of TSAN backend")
		set(TSAN_BINARY_DIR "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/release" CACHE INTERNAL "path to tsan binary")
	endif()

	find_library(TSAN_LIB NAMES "race_windows_amd64" HINTS "${TSAN_BINARY_DIR}" NO_DEFAULT_PATH)
	add_library("tsan-common" INTERFACE)
	target_include_directories("tsan-common" INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tsan/include)
	target_link_libraries("tsan-common" INTERFACE ${TSAN_LIB})
	install(FILES "${TSAN_BINARY_DIR}/race_windows_amd64.dll" DESTINATION bin)
	file(READ "tsan/LICENSE.TXT" LIC_FILE_TSAN)
	file(APPEND ${LIC_FILE} "${LIC_SEP}LLVM-ThreadSanitizer\n\n${LIC_FILE_TSAN}")

	# inih is mandatory
	if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jtilly/inih/INIReader.h")
		message(FATAL_ERROR "inih submodule not available")
	endif()
	add_library("jtilly-inih" INTERFACE)
	target_include_directories("jtilly-inih" INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/jtilly/inih>)
	target_compile_definitions("jtilly-inih" INTERFACE "-D_CRT_SECURE_NO_WARNINGS")
	file(READ "jtilly/inih/LICENSE.txt" LIC_FILE_INIH)
	file(APPEND ${LIC_FILE} "${LIC_SEP}jtilly/inih\n\n${LIC_FILE_INIH}")

	# TinyXML2 library for Valkyrie output
	if(DRACE_XML_EXPORTER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/leethomason/tinyxml2/tinyxml2.cpp")
		add_library("tinyxml2" SHARED "${CMAKE_CURRENT_SOURCE_DIR}/leethomason/tinyxml2/tinyxml2.cpp")
		target_include_directories("tinyxml2" INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/leethomason/tinyxml2/)
		install(TARGETS "tinyxml2"
		RUNTIME DESTINATION bin COMPONENT Runtime
		LIBRARY DESTINATION lib COMPONENT Libraries)
		file(READ "leethomason/tinyxml2/LICENSE.txt" LIC_FILE_TINYXML2)
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
endif()

# muellan clipp
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/muellan/clipp/include")
	message(FATAL_ERROR "HowardHinnant Date submodule not available")
endif()
add_library("clipp" INTERFACE)
target_include_directories("clipp" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/muellan/clipp/include>)
file(READ "muellan/clipp/LICENSE" LIC_FILE_CLIPP)
file(APPEND ${LIC_FILE} "${LIC_SEP}muellan/clipp\n\n${LIC_FILE_CLIPP}")

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/google/googletest/googletest/CMakeLists.txt")
	# force gtest to build static lib
	set(gtest_force_shared_crt ON CACHE BOOL "static lib" FORCE)
	add_subdirectory("google/googletest")
	else()
	set(BUILD_TESTING OFF CACHE BOOL "Enable Testing" FORCE)
endif()

if(DRACE_ENABLE_BENCH AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/google/benchmark/CMakeLists.txt")
	set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
	set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Do not install benchmark" FORCE)
	add_subdirectory("google/benchmark")
else()
	set(DRACE_ENABLE_BENCH OFF CACHE BOOL "Enable benchmark" FORCE)
endif()

# Used in FastTrack2 detector backend
# greg7mdp parallel-hashmap
add_library("parallel-hashmap" INTERFACE)
target_include_directories("parallel-hashmap" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/greg7mdp/parallel-hashmap>)
if(WIN32)
	target_compile_options("parallel-hashmap" INTERFACE "/arch:AVX2")
endif()
file(READ "greg7mdp/parallel-hashmap/LICENSE" LIC_FILE_SPARSEPP)
file(APPEND ${LIC_FILE} "${LIC_SEP}greg7mdp/parallel-hashmap\n\n${LIC_FILE_SPARSEPP}")

# gabime/spdlog for logging
add_library("spdlog" INTERFACE)
target_include_directories("spdlog" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gabime/spdlog/include>)
file(READ "gabime/spdlog/LICENSE" LIC_FILE_SPDLOG)
file(APPEND ${LIC_FILE} "${LIC_SEP}gabime/spdlog\n\n${LIC_FILE_SPDLOG}")
