# examples/cross/basic_label/CMakeLists.txt
add_executable(basic_label basic_label.c)
target_include_directories(basic_label PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(basic_label PRIVATE eui ${BOOTSTRAP_LIB})
