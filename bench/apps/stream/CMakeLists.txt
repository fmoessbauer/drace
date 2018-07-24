SET(BENCH "bench-stream")

add_executable(${BENCH} "main")
set_target_properties(${BENCH} PROPERTIES C_STANDARD 99)
