# =============================================================================
# SKIA INTEGRATION
# =============================================================================

include_guard(GLOBAL)

# Skia rendering option (Release recommended)
option(ZENITH_ENABLE_SKIA "Enable Skia Hardware-Accelerated Rendering" ON)

if(ZENITH_ENABLE_SKIA)
    # Skia debug build compatibility warning handled in main CMakeLists.txt
    
    # Manual Skia integration
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/cmake/SkiaManualIntegration.cmake")
        include(apps/desktop/cmake/SkiaManualIntegration.cmake)
        target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)
        message(STATUS "Zenith DAW: Skia rendering ENABLED")
    else()
        message(WARNING "Skia integration file not found. Skia will be disabled.")
        set(ZENITH_ENABLE_SKIA OFF)
    endif()
else()
    message(STATUS "Zenith DAW: Skia rendering DISABLED")
endif()

# Platform-specific graphics libraries
if(ZENITH_ENABLE_SKIA)
    if(WIN32)
        set(ZENITH_GRAPHICS_LIBS d3d12 dxgi dcomp uuid)
    elseif(APPLE)
        set(ZENITH_GRAPHICS_LIBS "-framework Metal" "-framework Foundation" "-framework QuartzCore")
    elseif(UNIX AND NOT APPLE)
        find_package(Vulkan QUIET)
        if(Vulkan_FOUND)
            set(ZENITH_GRAPHICS_LIBS Vulkan::Vulkan)
        else()
            set(ZENITH_GRAPHICS_LIBS vulkan)
            message(STATUS "Vulkan not found - using system fallback")
        endif()
    endif()
endif()
