# port/web/CMakeLists.txt

add_library(eui_port_web STATIC eui_port_bootstrap.c)
target_include_directories(eui_port_web PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(eui_port_web PUBLIC eui)

# Emscripten flags injected into every executable that links this lib
target_link_options(eui_port_web INTERFACE
    -sWASM=1
    -sEXPORTED_FUNCTIONS='["_main","_eui_web_push_key_event","_malloc","_free"]'
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
    -sEXIT_RUNTIME=0
    -sINITIAL_MEMORY=16MB
)

# ── Per-example HTML generation ────────────────────────────────────
# Global property: collects example target names so web_index can depend on them
set_property(GLOBAL PROPERTY WEB_EXAMPLE_TARGETS "")

function(eui_web_add_html target_name title)
    # Record this example as a dependency of web_index
    set_property(GLOBAL APPEND PROPERTY WEB_EXAMPLE_TARGETS "${target_name}")

    # Configure HTML from template
    set(EXAMPLE_TITLE "${title}")
    set(EXAMPLE_JS    "${target_name}.js")
    set(html_out      "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.html")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/shell.html.in"
        "${html_out}"
        @ONLY
        NEWLINE_STYLE LF
    )
    # Copy to executable's output dir (next to .js/.wasm)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${html_out}"
            "$<TARGET_FILE_DIR:${target_name}>/${target_name}.html"
        COMMENT "Generating ${target_name}.html"
        VERBATIM
    )
endfunction()
