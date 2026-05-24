# examples/cross/button_test/CMakeLists.txt
add_executable(button_test button_test.c)
target_include_directories(button_test PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(button_test PRIVATE eui ${BOOTSTRAP_LIB})
