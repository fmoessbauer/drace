set(SOURCES "main" "detector" "containers")

set(RELEASE_COMPILE_FLAGS "-O3 -Wall")
set(DEBUG_COMPILE_FLAGS "-g -Wall -Werror") 

add_executable("${PROJECT_NAME}-bench" ${SOURCES})
set_target_properties("${PROJECT_NAME}-bench" PROPERTIES CXX_STANDARD 14)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set_target_properties("${PROJECT_NAME}-bench" PROPERTIES COMPILE_FLAGS "${DEBUG_COMPILE_FLAGS}")
else()
  set_target_properties("${PROJECT_NAME}-bench" PROPERTIES COMPILE_FLAGS "${RELEASE_COMPILE_FLAGS}")
endif()

target_link_libraries("${PROJECT_NAME}-bench" benchmark	"drace-detector" "drace-common")

add_subdirectory(apps)
