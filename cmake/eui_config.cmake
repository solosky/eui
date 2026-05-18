function(eui_target_configure target)
    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(${target} PUBLIC eui)
endfunction()
