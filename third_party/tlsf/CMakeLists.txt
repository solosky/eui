if(ESP_PLATFORM)
    # ESP32-IDF 的 heap 组件内部自带一份 tlsf（FL_INDEX_MAX 被调小，池上限
    # 4KB，见 components/heap/tlsf/tlsf.c），且 libheap.a 在链接序上先于本
    # 组件——同名符号会被它抢先解析，导致大池（如 16bpp 画布需要的 196KB）
    # 被拒（"Memory size must be between 20 and 4104 bytes"）。加前缀重命名
    # 全部公开符号，两份实现互不干扰。（子项目 C 上机发现，2026-09-25）
    set(TLSF_RENAME_DEFS
        -Dtlsf_create=eui_tlsf_create
        -Dtlsf_create_with_pool=eui_tlsf_create_with_pool
        -Dtlsf_destroy=eui_tlsf_destroy
        -Dtlsf_get_pool=eui_tlsf_get_pool
        -Dtlsf_add_pool=eui_tlsf_add_pool
        -Dtlsf_remove_pool=eui_tlsf_remove_pool
        -Dtlsf_malloc=eui_tlsf_malloc
        -Dtlsf_memalign=eui_tlsf_memalign
        -Dtlsf_memalign_offs=eui_tlsf_memalign_offs
        -Dtlsf_realloc=eui_tlsf_realloc
        -Dtlsf_free=eui_tlsf_free
        -Dtlsf_block_size=eui_tlsf_block_size
        -Dtlsf_size=eui_tlsf_size
        -Dtlsf_align_size=eui_tlsf_align_size
        -Dtlsf_block_size_min=eui_tlsf_block_size_min
        -Dtlsf_block_size_max=eui_tlsf_block_size_max
        -Dtlsf_pool_overhead=eui_tlsf_pool_overhead
        -Dtlsf_alloc_overhead=eui_tlsf_alloc_overhead
        -Dtlsf_walk_pool=eui_tlsf_walk_pool
        -Dtlsf_check=eui_tlsf_check
        -Dtlsf_check_pool=eui_tlsf_check_pool
        -Dtlsf_fit_size=eui_tlsf_fit_size
        -Dtlsf_walker=eui_tlsf_walker)
    idf_component_register(
        SRCS tlsf.c
        INCLUDE_DIRS .
    )
    # PUBLIC：消费者（eui_allocator.c 等 REQUIRES tlsf 的编译单元）必须看到
    # 同样的重命名，引用才会落在本组件的符号上。
    target_compile_definitions(${COMPONENT_LIB} PUBLIC ${TLSF_RENAME_DEFS})
else()
    add_library(tlsf STATIC tlsf.c)
    target_include_directories(tlsf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
