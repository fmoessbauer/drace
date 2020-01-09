# build all mini-applications

add_subdirectory("concurrent-inc")
add_subdirectory("inc-mutex")
add_subdirectory("empty-main")
add_subdirectory("single-thread")
add_subdirectory("annotations")
add_subdirectory("atomics")
add_subdirectory("sampler")
add_subdirectory("memchecker")

if(WIN32)
    add_subdirectory("lock-kinds")
    # CSharp examples
    add_subdirectory("cs-sync")
endif()
