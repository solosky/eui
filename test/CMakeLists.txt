add_executable(test_allocator test_allocator.c)
target_include_directories(test_allocator PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_allocator PRIVATE eui)
add_test(NAME allocator COMMAND test_allocator)

add_executable(test_event test_event.c)
target_include_directories(test_event PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_event PRIVATE eui)
add_test(NAME event COMMAND test_event)

add_executable(test_input test_input.c)
target_include_directories(test_input PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_input PRIVATE eui)
add_test(NAME input COMMAND test_input)

add_executable(test_canvas test_canvas.c)
target_include_directories(test_canvas PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_canvas PRIVATE eui)
add_test(NAME canvas COMMAND test_canvas)

add_executable(test_font test_font.c)
target_include_directories(test_font PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_font PRIVATE eui)
add_test(NAME font COMMAND test_font)

add_executable(test_font_u8g2 test_font_u8g2.c)
target_include_directories(test_font_u8g2 PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_font_u8g2 PRIVATE eui)
add_test(NAME font_u8g2 COMMAND test_font_u8g2)

add_executable(test_font_kerning test_font_kerning.c)
target_include_directories(test_font_kerning PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_font_kerning PRIVATE eui)
add_test(NAME font_kerning COMMAND test_font_kerning)

add_executable(test_font_multiline test_font_multiline.c)
target_include_directories(test_font_multiline PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_font_multiline PRIVATE eui)
add_test(NAME font_multiline COMMAND test_font_multiline)

add_executable(test_font_canvas test_font_canvas.c)
target_include_directories(test_font_canvas PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_font_canvas PRIVATE eui)
add_test(NAME font_canvas COMMAND test_font_canvas)

add_executable(test_view test_view.c)
target_include_directories(test_view PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_view PRIVATE eui)
add_test(NAME view COMMAND test_view)

add_executable(test_widget test_widget.c)
target_include_directories(test_widget PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_widget PRIVATE eui)
add_test(NAME widget COMMAND test_widget)

add_executable(test_font_real_u8g2_render test_font_real_u8g2_render.c)
target_include_directories(test_font_real_u8g2_render PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_font_real_u8g2_render PRIVATE eui)
add_test(NAME font_real_u8g2_render COMMAND test_font_real_u8g2_render)

add_executable(test_font_wqy12_chinese test_font_wqy12_chinese.c)
target_include_directories(test_font_wqy12_chinese PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_font_wqy12_chinese PRIVATE eui)
add_test(NAME font_wqy12_chinese COMMAND test_font_wqy12_chinese)

add_executable(test_drivers test_drivers.c)
target_include_directories(test_drivers PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_drivers PRIVATE eui)
add_test(NAME drivers COMMAND test_drivers)
