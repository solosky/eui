# examples/cross/color_demo/CMakeLists.txt
add_executable(color_demo color_demo.c)
target_include_directories(color_demo PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(color_demo PRIVATE eui ${BOOTSTRAP_LIB})
