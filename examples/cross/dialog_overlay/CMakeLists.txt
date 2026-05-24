# examples/cross/dialog_overlay/CMakeLists.txt
add_executable(dialog_overlay dialog_overlay.c)
target_include_directories(dialog_overlay PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(dialog_overlay PRIVATE eui ${BOOTSTRAP_LIB})
