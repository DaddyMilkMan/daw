if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW_lib PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SkiaRenderer.cpp
    )
endif()


