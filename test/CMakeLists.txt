set(SOURCES 
	"src/main.cpp"
	"src/DetectorTest.cpp"
	"src/IntegrationTest.cpp"
	"src/ShmDriver.cpp")

set(TEST_TARGET "drace-tests")

# get path to drrun and hard-code into test binary for easier
# integration in CI
get_filename_component(DR_NORMALIZED_PATH "${DynamoRIO_DIR}/../bin64/drrun.exe" ABSOLUTE)
file(TO_NATIVE_PATH ${DR_NORMALIZED_PATH} PATH_TO_DRRUN)
message(STATUS "Test (integration): Use ${PATH_TO_DRRUN}")
configure_file(src/IntegrationTestEnv.cpp.cmake generated/IntegrationTestEnv.cpp @ONLY)

add_executable(${TEST_TARGET} ${SOURCES}
	${CMAKE_CURRENT_BINARY_DIR}/generated/IntegrationTestEnv.cpp)

target_include_directories(${TEST_TARGET} PRIVATE "include")
target_link_libraries(${TEST_TARGET} gtest "drace-common")

# enable dr mocks
target_compile_definitions(${TEST_TARGET} PRIVATE -DTESTING)
add_dependencies(${TEST_TARGET} "drace-client")

#gtest_discover_tests(${TEST_TARGET})

# manually build C# tests as CMake support is not available in ninja
find_program(CSC csc.exe DOC "Path to CSharp compiler csc.exe")
if(CSC)
    target_compile_definitions(${TEST_TARGET} PRIVATE -DDRACE_TESTING_DOTNET)
else()
    message(STATUS "csc not found: .NET Windows tests will be disabled")
endif()

add_custom_target(copy_files_testing ALL
    DEPENDS "${PROJECT_SOURCE_DIR}/drace.ini"
    DEPENDS "$<TARGET_FILE:drace.detector.tsan>"
)

if(${DRACE_XML_EXPORTER})
    target_compile_definitions(${TEST_TARGET} PRIVATE -DDRACE_XML_EXPORTER)
    target_link_libraries(${TEST_TARGET} "tinyxml2")
	add_custom_command(TARGET copy_files_testing POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:tinyxml2>"
			"$<TARGET_FILE_DIR:drace-tests>")
endif()

add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/test/data/drace_excl.ini"
		"$<TARGET_FILE_DIR:drace-tests>/data/drace_excl.ini")

# copy tsan dll
add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.tsan>"
		"$<TARGET_FILE_DIR:drace-tests>")

# Copy tsan-common
add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${TSAN_BINARY_DIR}/race_windows_amd64.dll"
		"$<TARGET_FILE_DIR:drace-tests>")

add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:drace.detector.dummy>"
		"$<TARGET_FILE_DIR:drace-tests>")

# copy gtest shared lib
add_custom_command(TARGET copy_files_testing POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:gtest>"
		"$<TARGET_FILE_DIR:drace-tests>")

# Prepare Guinea pigs
add_subdirectory("mini-apps")

if(${DRACE_INSTALL_TESTS})
    install(TARGETS ${TEST_TARGET} RUNTIME DESTINATION bin COMPONENT RUNTIME)
    install(FILES ${PROJECT_SOURCE_DIR}/test/data/drace_excl.ini DESTINATION bin)
endif()
