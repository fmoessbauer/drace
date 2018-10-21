# Common header-only target

add_library("drace-common" INTERFACE)
target_include_directories("drace-common" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
