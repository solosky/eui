# examples/cross/scene_view_demo/CMakeLists.txt
add_executable(scene_view_demo scene_view_demo.c)
target_include_directories(scene_view_demo PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(scene_view_demo PRIVATE eui ${BOOTSTRAP_LIB})
