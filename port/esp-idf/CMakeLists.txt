add_library(eui_port_esp_idf STATIC eui_port_esp_idf.c eui_port_bootstrap.c)
target_include_directories(eui_port_esp_idf PUBLIC
    .
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)
# freertos 假配置头只给宿主构建用。它绝不能出现在 PUBLIC/PUBLIC-导出路径上，
# 否则会遮蔽 IDF 的真配置（见 include-standalone/freertos/FreeRTOSConfig.h 的说明）。
target_include_directories(eui_port_esp_idf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include-standalone
)
target_link_libraries(eui_port_esp_idf PUBLIC eui)
