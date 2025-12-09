# ============================================================================
# Standard Skia Integration (vcpkg)
# ============================================================================
# Relies entirely on vcpkg to provide 'skia'.
# ============================================================================

message(STATUS "============================================")
message(STATUS "Configuring Skia Integration (vcpkg)")
message(STATUS "============================================")

set(SKIA_FOUND_AND_READY OFF)

# 1. Find Package (Required) - try both package names
find_package(skia CONFIG QUIET)
if(NOT skia_FOUND)
    find_package(unofficial-skia CONFIG QUIET)
    if(unofficial-skia_FOUND)
        set(SKIA_TARGET unofficial::skia::skia)
        set(skia_FOUND TRUE)
    endif()
else()
    set(SKIA_TARGET skia::skia)
endif()

if(skia_FOUND)
    message(STATUS "  Found Skia via vcpkg")
    target_link_libraries(ZenithDAW PRIVATE ${SKIA_TARGET})
    target_compile_definitions(ZenithDAW PRIVATE SK_GL=1)
    set(SKIA_FOUND_AND_READY ON)
else()
    message(FATAL_ERROR "Skia library not found! Install it via vcpkg: 'vcpkg install skia[gl]:x64-windows'")
endif()

# 2. UI Components (Conditional on Skia Library)
if(SKIA_FOUND_AND_READY)
    message(STATUS "  Enabling Skia UI Components...")
    
    # Define ZENITH_USE_SKIA globally
    target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)

    # Add Skia UI source files
    target_sources(ZenithDAW PRIVATE
        apps/desktop/Source/ui/skia/SkiaComponent.cpp
        apps/desktop/Source/ui/skia/SkiaButton.cpp
        apps/desktop/Source/ui/skia/SkiaKnob.cpp
        apps/desktop/Source/ui/skia/SkiaSlider.cpp
        apps/desktop/Source/ui/skia/SkiaSpectrumComponent.cpp
        apps/desktop/Source/ui/skia/SkiaMainWindowIntegration.cpp
        apps/desktop/Source/ui/skia/TransportBar.cpp
        apps/desktop/Source/ui/skia/BottomBar.cpp
        apps/desktop/Source/ui/skia/BrowserPanel.cpp
        apps/desktop/Source/ui/skia/RightSidePanel.cpp
        apps/desktop/Source/ui/skia/ZenithPolySynthUI.cpp
        apps/desktop/Source/ui/skia/ZenithDesignSystem.cpp
        apps/desktop/Source/ui/skia/views/PianoKeyboardViewSkia.cpp
        apps/desktop/Source/ui/skia/ZenithUIComponents.h
        apps/desktop/Source/ui/skia/DebugConsoleComponent.cpp
    )

    target_include_directories(ZenithDAW PRIVATE
        apps/desktop/Source/ui/skia
    )
    
    message(STATUS "  Skia UI components: ENABLED")
else()
    message(FATAL_ERROR "Skia library not found! Install it via vcpkg: 'vcpkg install skia[gl]:x64-windows'")
endif()

message(STATUS "============================================")