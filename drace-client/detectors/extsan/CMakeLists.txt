add_library("drace-detector" SHARED "extsan")
set_target_properties("drace-detector" PROPERTIES CXX_STANDARD 11)
target_link_libraries("drace-detector" "drace-common")
