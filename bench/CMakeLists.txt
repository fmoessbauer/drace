set(SOURCES "main" "detector" "containers")

add_executable("${PROJECT_NAME}-bench" ${SOURCES})
set_target_properties("${PROJECT_NAME}-bench" PROPERTIES CXX_STANDARD 14)

target_link_libraries("${PROJECT_NAME}-bench" benchmark	"drace-detector" "drace-common")

add_subdirectory(apps)
