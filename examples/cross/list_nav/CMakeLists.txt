# examples/cross/list_nav/CMakeLists.txt
add_executable(list_nav list_nav.c)
target_include_directories(list_nav PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(list_nav PRIVATE eui ${BOOTSTRAP_LIB})
