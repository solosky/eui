# examples/cross/benchmark/CMakeLists.txt
add_executable(benchmark benchmark.c)
target_include_directories(benchmark PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(benchmark PRIVATE eui ${BOOTSTRAP_LIB})
