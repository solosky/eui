# port/web/gen_index.cmake — scans build tree for .html example files,
# generates index.html linking to each.
# Invoked: cmake -DBUILD_DIR=<dir> -DOUTPUT=<path> -P gen_index.cmake

file(GLOB_RECURSE html_files "${BUILD_DIR}/examples/cross/*.html")
list(SORT html_files)

set(items "")
foreach(f ${html_files})
    get_filename_component(name "${f}" NAME_WE)
    string(APPEND items "<li><a href=\"../examples/cross/${name}/${name}.html\">${name}</a></li>\n")
endforeach()

set(html "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n")
string(APPEND html "<meta charset=\"utf-8\">\n")
string(APPEND html "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n")
string(APPEND html "<title>EUI Web Examples</title>\n")
string(APPEND html "<style>")
string(APPEND html "body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;")
string(APPEND html "max-width:400px;margin:40px auto;padding:0 16px}")
string(APPEND html "li{margin:8px 0}")
string(APPEND html "a{text-decoration:none;color:#2563eb}a:hover{text-decoration:underline}")
string(APPEND html "</style>\n</head>\n<body>\n")
string(APPEND html "<h1>EUI Web Examples</h1>\n<ul>\n${items}</ul>\n")
string(APPEND html "</body>\n</html>\n")

file(WRITE "${OUTPUT}" "${html}")
message(STATUS "Generated index.html")
