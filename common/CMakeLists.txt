# burn-in git commit hash
set(PRE_CONFIGURE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/version/version.h.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/version/version.h")
include(${PROJECT_SOURCE_DIR}/CMakeExt/git_watcher.cmake)

# Common header-only target
add_library("drace-common" INTERFACE)
target_include_directories("drace-common" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)