function(eui_add_test name source)
    cmake_parse_arguments(TEST "" "" "INCLUDES;SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${source} ${TEST_SOURCES})
    target_include_directories(${name} PRIVATE
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TEST_INCLUDES}
    )
    target_link_libraries(${name} PRIVATE eui ${TEST_LIBS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# ---- Common / Foundation ----
eui_add_test(test_allocator     allocator/test_allocator.c)
eui_add_test(test_event         event/test_event.c)
eui_add_test(test_post          post/test_post.c)
eui_add_test(test_scene         scene/test_scene.c)
eui_add_test(test_scene_events  scene/test_scene_events.c)
eui_add_test(test_input         input/test_input.c)
eui_add_test(test_input_edge    input/test_input_edge.c)
eui_add_test(test_str           str/test_str.c)

# ---- Canvas ----
eui_add_test(test_canvas        canvas/test_canvas.c)
if(EUI_COLOR_DEPTH EQUAL 16)
    eui_add_test(test_canvas_16bpp  canvas/test_canvas_16bpp.c)
endif()
if(EUI_COLOR_DEPTH EQUAL 2)
    eui_add_test(test_canvas_2bpp   canvas/test_canvas_2bpp.c)
endif()
if(EUI_COLOR_DEPTH EQUAL 4)
    eui_add_test(test_canvas_4bpp   canvas/test_canvas_4bpp.c)
endif()
if(EUI_COLOR_DEPTH EQUAL 1 OR EUI_COLOR_DEPTH EQUAL 2)
    eui_add_test(test_canvas_1bpp   canvas/test_canvas_1bpp.c)
endif()
eui_add_test(test_canvas_render canvas/test_canvas_render.c)
eui_add_test(test_canvas_aa     canvas/test_canvas_aa.c)
eui_add_test(test_canvas_alpha  canvas/test_canvas_alpha.c)
if(EUI_COLOR_DEPTH EQUAL 16)
    eui_add_test(test_canvas_bitmap_rot canvas/test_canvas_bitmap_rot.c)
endif()

# ---- Font ----
eui_add_test(test_font_formats          font/test_font_formats.c)
eui_add_test(test_font_kerning          font/test_font_kerning.c)
eui_add_test(test_font_canvas           font/test_font_canvas.c)
eui_add_test(test_font_multiline        font/test_font_multiline.c)
eui_add_test(test_font_wqy13_render     font/test_font_wqy13_render.c)
eui_add_test(test_font_real_u8g2_render font/test_font_real_u8g2_render.c)
eui_add_test(test_font_vlw_render     font/test_font_vlw_render.c)

# ---- Selector ----
eui_add_test(test_selector          selector/test_selector.c)

# ---- View ----
eui_add_test(test_view              view/test_view.c)
eui_add_test(test_dispatcher_active view/test_dispatcher_active.c)
if(EUI_COLOR_DEPTH EQUAL 16)
    eui_add_test(test_chrome_layer   view/test_chrome_layer.c)
endif()
eui_add_test(test_widget            view/test_widget.c)
eui_add_test(test_view_transition   view/test_view_transition.c)
eui_add_test(test_view_transition_render view/test_view_transition_render.c
    LIBS eui_drv_raylib)

# ---- Drivers ----
eui_add_test(test_drivers drivers/test_drivers.c)

# ---- u8g2 reference comparison tests ----
set(U8G2_DIR "${CMAKE_SOURCE_DIR}/../u8g2" CACHE PATH "u8g2 source directory")

if(EXISTS ${U8G2_DIR})
    set(U8G2_SOURCES
        ${U8G2_DIR}/csrc/u8g2_setup.c
        ${U8G2_DIR}/csrc/u8g2_font.c
        ${U8G2_DIR}/csrc/u8g2_ll_hvline.c
        ${U8G2_DIR}/csrc/u8g2_buffer.c
        ${U8G2_DIR}/csrc/u8g2_kerning.c
        ${U8G2_DIR}/csrc/u8g2_intersection.c
        ${U8G2_DIR}/csrc/u8g2_hvline.c
        ${U8G2_DIR}/csrc/u8x8_setup.c
        ${U8G2_DIR}/csrc/u8x8_display.c
        ${U8G2_DIR}/csrc/u8x8_capture.c
        ${U8G2_DIR}/csrc/u8x8_gpio.c
        ${U8G2_DIR}/csrc/u8x8_u16toa.c
        ${U8G2_DIR}/csrc/u8x8_byte.c
        ${U8G2_DIR}/csrc/u8x8_8x8.c
        ${U8G2_DIR}/csrc/u8x8_cad.c
    )

    eui_add_test(test_font_vs_u8g2 font/test_font_vs_u8g2.c
        SOURCES  ${U8G2_SOURCES}
        INCLUDES ${U8G2_DIR}/csrc
    )
    eui_add_test(test_wqy13_kerning font/test_wqy13_kerning.c
        SOURCES  ${U8G2_SOURCES}
        INCLUDES ${U8G2_DIR}/csrc
    )
endif()
