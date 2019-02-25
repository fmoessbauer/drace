# burn-in git commit hash
set(PRE_CONFIGURE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/version/version.cpp.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/version/version.cpp")
include(${PROJECT_SOURCE_DIR}/CMakeExt/git_watcher.cmake)

# Common header-only target
add_library("drace-common" STATIC version/version.cpp)
target_include_directories("drace-common" INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_options("drace-common" INTERFACE -EHsc)
set_target_properties("drace-common" PROPERTIES LINKER_LANGUAGE CXX)
