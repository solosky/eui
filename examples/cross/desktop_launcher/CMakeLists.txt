# examples/cross/desktop_launcher/CMakeLists.txt
add_executable(desktop_launcher desktop_launcher.c)
target_include_directories(desktop_launcher PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(desktop_launcher PRIVATE eui ${BOOTSTRAP_LIB} m)
