set(SOURCES
	"src/main.cpp"
	"src/ManagedResolver.cpp")

add_executable("msr" ${SOURCES})
target_include_directories(
	"msr" PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries("msr" "dbgeng.dll")
