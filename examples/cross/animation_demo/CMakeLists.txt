# examples/cross/animation_demo/CMakeLists.txt
add_executable(animation_demo animation_demo.c)
target_include_directories(animation_demo PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(animation_demo PRIVATE eui ${BOOTSTRAP_LIB})
