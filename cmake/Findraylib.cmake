# - Try to find raylib
# Uses CONFIG mode first (system-installed), then pre-built installs,
# and finally builds from the submodule source.
#
# Defines:
#   raylib_FOUND  - raylib is available
#   raylib        - imported target for linking

# Try CONFIG mode first (finds system-installed raylib)
find_package(raylib CONFIG QUIET)

if(TARGET raylib OR TARGET raylib::raylib)
    return()
endif()

# Fallback: pre-built installs (manual search - raylib's config
# doesn't use HINTS properly for custom prefixes)
foreach(_dir
    "${CMAKE_SOURCE_DIR}/third_party/raylib-prebuilt"
    "${CMAKE_SOURCE_DIR}/third_party/raylib/install"
)
    find_path(raylib_INCLUDE_DIR raylib.h
        HINTS "${_dir}/include"
        NO_DEFAULT_PATH
    )
    find_library(raylib_LIBRARY
        NAMES raylib libraylib
        HINTS "${_dir}/lib"
        NO_DEFAULT_PATH
    )
    if(raylib_LIBRARY AND raylib_INCLUDE_DIR)
        break()
    endif()
endforeach()

if(NOT raylib_LIBRARY OR NOT raylib_INCLUDE_DIR)
    find_path(raylib_INCLUDE_DIR raylib.h
        PATHS /usr/include /usr/local/include
    )
    find_library(raylib_LIBRARY
        NAMES raylib libraylib
        PATHS /usr/lib /usr/local/lib
    )
endif()

if((NOT raylib_LIBRARY OR NOT raylib_INCLUDE_DIR) AND NOT TARGET raylib)
    # Build raylib from submodule source
    set(_raylib_src_dir "${CMAKE_SOURCE_DIR}/third_party/raylib")
    if(EXISTS "${_raylib_src_dir}/CMakeLists.txt" AND EXISTS "${_raylib_src_dir}/src/raylib.h")
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
            add_subdirectory("${_raylib_src_dir}" raylib_build)
        else()
            set(_raylib_patched_dir "${CMAKE_BINARY_DIR}/_deps/raylib_patched")
            file(COPY "${_raylib_src_dir}" DESTINATION "${CMAKE_BINARY_DIR}/_deps")
            file(RENAME "${CMAKE_BINARY_DIR}/_deps/raylib" "${_raylib_patched_dir}")
            file(READ "${_raylib_patched_dir}/CMakeLists.txt" _rl_cmake)
            string(REPLACE "cmake_minimum_required(VERSION 3.25)"
                   "cmake_minimum_required(VERSION 3.16)"
                   _rl_cmake_patched "${_rl_cmake}")
            file(WRITE "${_raylib_patched_dir}/CMakeLists.txt" "${_rl_cmake_patched}")
            add_subdirectory("${_raylib_patched_dir}" raylib_build)
        endif()
        if(TARGET raylib)
            set(raylib_FOUND TRUE)
        endif()
    endif()
else()
    set(raylib_FOUND TRUE)
endif()

if(raylib_FOUND AND NOT TARGET raylib)
    add_library(raylib STATIC IMPORTED GLOBAL)
    set_target_properties(raylib PROPERTIES
        IMPORTED_LOCATION "${raylib_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${raylib_INCLUDE_DIR}"
    )
    mark_as_advanced(raylib_INCLUDE_DIR raylib_LIBRARY)
endif()
