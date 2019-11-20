# burn-in git commit hash
set(PRE_CONFIGURE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/version/version.h.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/version/version.h")
include(${PROJECT_SOURCE_DIR}/CMakeExt/git_watcher.cmake)

# Common header-only target
add_library("drace-common" INTERFACE)
target_include_directories("drace-common" INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_options("drace-common" INTERFACE -EHsc)
add_dependencies("drace-common" check_git_repository)

################ configure test module ################
if(DRACE_ENABLE_TESTING)
    # create library which is linked in global testing module
    add_library(common_test OBJECT "test/ShmDriver")
    # use static version of stdlib (has to match gtests version)
    target_compile_options(common_test PRIVATE "/MT$<$<CONFIG:Debug>:d>")
    target_link_libraries(common_test PRIVATE gtest "drace-common")
    set_property(GLOBAL APPEND PROPERTY DRACE_TEST_SUITES common_test)
endif()