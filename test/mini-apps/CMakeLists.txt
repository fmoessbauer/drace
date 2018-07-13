# build all mini-applications

add_subdirectory("concurrent-inc")
add_subdirectory("inc-mutex")
add_subdirectory("empty-main")
add_subdirectory("lock-kinds")

if(${BUILD_CSHARP_EXAMPLES})
	# CSharp examples
	add_subdirectory("cs-lock")
endif()
