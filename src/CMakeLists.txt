add_library(eui STATIC
    eui_types.c
    eui_allocator.c
)
target_include_directories(eui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../include
)
target_link_libraries(eui PUBLIC tlsf mc)
