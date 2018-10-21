add_library("drace-detector" SHARED "tsan")
target_link_libraries("drace-detector" "tsan-common" "drace-common")