set(SOURCES
	"src/main.cpp"
	"src/Controller.cpp"
	"src/ManagedResolver.cpp"
	"src/ProtocolHandler.cpp")

add_executable("msr" ${SOURCES})
target_include_directories("msr" PRIVATE "include")
target_link_libraries("msr" "drace-common" "spdlog" "clipp")
if(WIN32)
  target_compile_options("msr" PRIVATE "-EHsc")
endif()

if(DRACE_ENABLE_CPPCHECK)
    set_target_properties("msr" PROPERTIES
        CXX_CPPCHECK ${DRACE_CPPCHECK_CALL})
endif()

# TODO: remove extsan support or re-enable it
# Link with tsan if detector is Extsan
if("${DRACE_DETECTOR}" STREQUAL "extsan")
	message("use tsan in msr")
	target_compile_definitions("msr" PRIVATE -DEXTSAN)

	target_sources("msr" PRIVATE "src/QueueHandler.cpp")
	target_link_libraries("msr" "tsan-common" "parallel-hashmap")
endif()

install(TARGETS "msr"
    RUNTIME DESTINATION ${DRACE_RUNTIME_DEST} COMPONENT RUNTIME
    LIBRARY DESTINATION ${DRACE_ARCHIVE_DEST} COMPONENT ARCHIVE)
