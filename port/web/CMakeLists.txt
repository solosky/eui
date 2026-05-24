# port/web/CMakeLists.txt

add_library(eui_port_web STATIC eui_port_bootstrap.c)
target_include_directories(eui_port_web PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(eui_port_web PUBLIC eui)

if(TARGET gen_profile_config)
    add_dependencies(eui_port_web gen_profile_config)
endif()

# Emscripten flags injected into every executable that links this lib
target_link_options(eui_port_web INTERFACE
    "SHELL:-s WASM=1"
    "SHELL:-s EXPORTED_FUNCTIONS=[_main,_eui_web_push_key_event,_malloc,_free]"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=ccall,cwrap"
    "SHELL:-s EXIT_RUNTIME=0"
    "SHELL:-s INITIAL_MEMORY=16MB"
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
    set(html_out      "${CMAKE_BINARY_DIR}/examples/cross/${target_name}/${target_name}.html.in")
    configure_file(
        "${CMAKE_SOURCE_DIR}/port/web/shell.html.in"
        "${html_out}"
        @ONLY
        NEWLINE_STYLE LF
    )
    # Copy to executable's output dir (next to .js/.wasm)
    add_custom_target(html_${target_name} ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${html_out}"
            "${CMAKE_BINARY_DIR}/examples/cross/${target_name}/${target_name}.html"
        DEPENDS ${target_name}
        COMMENT "Generating ${target_name}.html"
        VERBATIM
    )
endfunction()

# Copy shell.js to examples/cross/ so HTML pages can reference it as ../shell.js
add_custom_target(copy_shell_js ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/port/web/shell.js"
        "${CMAKE_BINARY_DIR}/examples/cross/shell.js"
    COMMENT "Copying shell.js to output"
    VERBATIM
)
