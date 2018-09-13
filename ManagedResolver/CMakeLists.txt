set(SOURCES
	"src/main.cpp"
	"src/ManagedResolver.cpp"
	"src/ProtocolHandler.cpp")

add_executable("msr" ${SOURCES})
target_include_directories(
	"msr" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Inter Process Communication Headers
target_include_directories(
	"msr" PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/common>)

target_link_libraries("msr" "dbgeng.dll")
