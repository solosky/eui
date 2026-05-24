# examples/cross/amiibo_demo/CMakeLists.txt
add_executable(amiibo_demo amiibo_demo.c)
target_include_directories(amiibo_demo PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(amiibo_demo PRIVATE eui ${BOOTSTRAP_LIB})
