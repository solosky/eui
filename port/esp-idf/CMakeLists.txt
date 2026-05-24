add_library(eui_port_esp_idf STATIC eui_port_esp_idf.c eui_port_bootstrap.c)
target_include_directories(eui_port_esp_idf PUBLIC
    .
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(eui_port_esp_idf PUBLIC eui)
