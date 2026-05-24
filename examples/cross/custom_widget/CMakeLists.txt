# examples/cross/custom_widget/CMakeLists.txt
add_executable(custom_widget custom_widget.c)
target_include_directories(custom_widget PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(custom_widget PRIVATE eui ${BOOTSTRAP_LIB})
