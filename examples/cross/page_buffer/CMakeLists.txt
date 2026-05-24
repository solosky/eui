# examples/cross/page_buffer/CMakeLists.txt
add_executable(page_buffer page_buffer.c)
target_include_directories(page_buffer PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(page_buffer PRIVATE eui ${BOOTSTRAP_LIB})
