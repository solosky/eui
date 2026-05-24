# examples/cross/menu_system/CMakeLists.txt
add_executable(menu_system menu_system.c)
target_include_directories(menu_system PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(menu_system PRIVATE eui ${BOOTSTRAP_LIB})
