find_library(TSAN_LIB NAMES "race_windows_amd64" HINTS "${PROJECT_SOURCE_DIR}/vendor/tsan/blob/")

add_library("tsan-custom" "tsan-driver")
target_include_directories("tsan-custom" PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> "include")
target_link_libraries("tsan-custom" PRIVATE ${TSAN_LIB})

# TODO: Copy Libraries
