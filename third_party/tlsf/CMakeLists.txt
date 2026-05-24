if(ESP_PLATFORM)
    idf_component_register(
        SRCS tlsf.c
        INCLUDE_DIRS .
    )
else()
    add_library(tlsf STATIC tlsf.c)
    target_include_directories(tlsf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
