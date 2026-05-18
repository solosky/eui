add_library(eui STATIC
    eui_types.c
    eui_allocator.c
    eui_event.c
    eui_input.c
    eui_canvas.c
    eui_font.c
    eui_font_bdf.c
    eui_font_vlw.c
    eui_font_builtin.c
    eui_view.c
    eui.c
)
target_include_directories(eui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../include
)
target_link_libraries(eui PUBLIC tlsf mc)
