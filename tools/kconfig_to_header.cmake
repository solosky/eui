# tools/kconfig_to_header.cmake
# Usage: cmake -DKCONFIG_FILE=<path> -DOUTPUT_HEADER=<path> -P tools/kconfig_to_header.cmake
#
# Parses a Kconfig file and generates a C header with #define CONFIG_<NAME> <VALUE>
# for each config entry's default value.

file(STRINGS "${KCONFIG_FILE}" lines)
set(current_config "")
set(output "/* Auto-generated from ${KCONFIG_FILE} — do not edit */\n\n")

foreach(line IN LISTS lines)
    string(STRIP "${line}" line)

    # Skip comments and empty lines
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()

    # Match: config FOO
    if(line MATCHES "^config ([A-Za-z0-9_]+)")
        set(current_config "${CMAKE_MATCH_1}")
        set(has_default FALSE)
    endif()

    # Match: default VALUE
    if(current_config AND line MATCHES "^default (.+)")
        set(default_val "${CMAKE_MATCH_1}")
        set(has_default TRUE)

        # Remove trailing comment
        string(REGEX REPLACE "[ \t]*#.*$" "" default_val "${default_val}")
        string(STRIP "${default_val}" default_val)

        if(default_val STREQUAL "y")
            set(default_val 1)
        endif()

        string(APPEND output "#define CONFIG_${current_config} ${default_val}\n")
        set(current_config "")
    endif()
endforeach()

file(WRITE "${OUTPUT_HEADER}" "${output}")
message(STATUS "Generated ${OUTPUT_HEADER} from ${KCONFIG_FILE}")
