find_library(TSAN_LIB NAMES "race_windows_amd64" HINTS "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/" NO_DEFAULT_PATH)

add_library("drace-tsan" SHARED "tsan")
set_target_properties("drace-tsan" PROPERTIES CXX_STANDARD 11)
target_include_directories("drace-tsan" PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vendor/tsan/include> "include")

target_link_libraries("drace-tsan" PRIVATE ${TSAN_LIB})