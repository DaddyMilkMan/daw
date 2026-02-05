if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SkiaRenderer.cpp
    )
endif()



