add_executable(test_allocator test_allocator.c)
target_include_directories(test_allocator PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_allocator PRIVATE eui)
add_test(NAME allocator COMMAND test_allocator)

add_executable(test_event test_event.c)
target_include_directories(test_event PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_event PRIVATE eui)
add_test(NAME event COMMAND test_event)

add_executable(test_input test_input.c)
target_include_directories(test_input PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_input PRIVATE eui)
add_test(NAME input COMMAND test_input)
