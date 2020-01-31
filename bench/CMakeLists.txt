set(SOURCES "main" "Detector" "Containers")

add_executable("drace-bench" ${SOURCES})
set_target_properties("drace-bench" PROPERTIES CXX_STANDARD 14)

target_link_libraries("drace-bench" benchmark "$<$<BOOL:${WIN32}>:drace.detector.tsan>" "drace-common")

# put all sample applications into dedicated folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/samples")

if(WIN32)
    add_subdirectory(apps)
endif()
