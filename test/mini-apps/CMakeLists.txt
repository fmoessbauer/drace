# build all mini-applications

add_subdirectory("concurrent-inc")
add_subdirectory("inc-mutex")
add_subdirectory("empty-main")
add_subdirectory("lock-kinds")
add_subdirectory("annotations")
add_subdirectory("atomics")
add_subdirectory("sampler")

# CSharp examples
add_subdirectory("cs-sync")
