add_executable(test_allocator test_allocator.c)
target_include_directories(test_allocator PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_allocator PRIVATE eui)
add_test(NAME allocator COMMAND test_allocator)
