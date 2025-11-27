# ============================================================================
# Manual Skia Integration (instead of vcpkg)
# ============================================================================
# This file replaces the vcpkg-based Skia integration in CMakeLists.txt
# 
# Usage: Include this after line 195 in zenith-core/CMakeLists.txt
#        (replacing the "if(ZENITH_ENABLE_SKIA)" block)
# ============================================================================

if(ZENITH_ENABLE_SKIA)
    message(STATUS "============================================")
    message(STATUS "Configuring Manual Skia Integration")
    message(STATUS "============================================")

    # Skia directory
    set(SKIA_DIR "C:/zenith/skia" CACHE PATH "Path to Skia installation")
    
    if(NOT EXISTS "${SKIA_DIR}")
        message(FATAL_ERROR 
            "\n"
            "Skia directory not found: ${SKIA_DIR}\n"
            "\n"
            "Please download Skia first:\n"
            "  1. Run: C:\\zenith\\daw\\download-skia.bat\n"
            "  2. Or manually download from: https://github.com/JetBrains/skia-pack/releases\n"
            "\n"
        )
    endif()

    # Find Skia headers and library
    find_path(SKIA_INCLUDE_DIR 
        NAMES include/core/SkCanvas.h
        PATHS ${SKIA_DIR}
        NO_DEFAULT_PATH
    )

    # Try multiple possible library locations
    find_library(SKIA_LIBRARY
        NAMES skia
        PATHS 
            ${SKIA_DIR}/out/Release-windows-x64
            ${SKIA_DIR}/out/Release-x64
            ${SKIA_DIR}/out/Release
        NO_DEFAULT_PATH
    )

    if(NOT SKIA_INCLUDE_DIR)
        message(FATAL_ERROR "Could not find Skia headers in ${SKIA_DIR}/include")
    endif()

    if(NOT SKIA_LIBRARY)
        message(FATAL_ERROR 
            "Could not find Skia library\n"
            "Searched in:\n"
            "  ${SKIA_DIR}/out/Release-windows-x64\n"
            "  ${SKIA_DIR}/out/Release-x64\n"
            "  ${SKIA_DIR}/out/Release\n"
        )
    endif()

    message(STATUS "  Skia headers: ${SKIA_INCLUDE_DIR}")
    message(STATUS "  Skia library: ${SKIA_LIBRARY}")

    # Add Skia source files
    target_sources(ZenithDAW PRIVATE
        # Skia rendering core
        Source/rendering/SkiaRenderer.h
        Source/rendering/SkiaRenderer.cpp
        Source/rendering/SkiaContextManager.h
        Source/rendering/SkiaContextManager.cpp

        # Skia UI components
        Source/ui/skia/SkiaTheme.h
        Source/ui/skia/SkiaTheme.cpp
        Source/ui/skia/SkiaTextRenderer.h
        Source/ui/skia/SkiaTextRenderer.cpp
        Source/ui/skia/SkiaLabel.h
        Source/ui/skia/SkiaButtonNative.h
        Source/ui/skia/SkiaTextInput.h
        Source/ui/skia/SkiaToggleButton.h
        Source/ui/skia/SkiaPanel.h
        Source/ui/skia/SkiaListBox.h
        Source/ui/skia/SkiaComboBox.h
        Source/ui/skia/SkiaWaveformRenderer.h
        Source/ui/skia/SkiaWaveformRenderer.cpp
        Source/ui/skia/SkiaPianoRollRenderer.h
        Source/ui/skia/SkiaPianoRollRenderer.cpp
        Source/ui/skia/SkiaClipRenderer.h
        Source/ui/skia/SkiaClipRenderer.cpp
        Source/ui/skia/SkiaKnobComponent.h
        Source/ui/skia/SkiaKnobComponent.cpp
        Source/ui/skia/SkiaSliderComponent.h
        Source/ui/skia/SkiaSliderComponent.cpp
        Source/ui/skia/SkiaButtonComponent.h
        Source/ui/skia/SkiaButtonComponent.cpp
        Source/ui/skia/SkiaMixerChannelComponent.h
        Source/ui/skia/SkiaMixerChannelComponent.cpp
        Source/ui/skia/SkiaTransportControlComponent.h
        Source/ui/skia/SkiaTransportControlComponent.cpp
        Source/ui/skia/SkiaMasterOutputMeterComponent.h
        Source/ui/skia/SkiaMasterOutputMeterComponent.cpp
        Source/ui/skia/SkiaEffectsChainComponent.h
        Source/ui/skia/SkiaEffectsChainComponent.cpp
        Source/ui/skia/SkiaInstrumentBrowserComponent.h
        Source/ui/skia/SkiaInstrumentBrowserComponent.cpp
        Source/ui/skia/SkiaPresetBrowserComponent.h
        Source/ui/skia/SkiaPresetBrowserComponent.cpp
        Source/ui/skia/SkiaSettingsManager.h
        Source/ui/skia/SkiaSettingsManager.cpp
        Source/ui/skia/SkiaPerformanceDashboard.h
        Source/ui/skia/SkiaPerformanceDashboard.cpp
        Source/ui/skia/SkiaMainWindowIntegration.h
        Source/ui/skia/SkiaMainWindowIntegration.cpp
        Source/ui/skia/SkiaCanvasComponent.cpp
        Source/ui/skia/SkiaColorTestComponent.h
        Source/ui/skia/SkiaColorTestComponent.cpp
        Source/ui/views/PianoKeyboardViewSkia.cpp

        # Modern DAW Layout Components
        Source/ui/skia/TransportBar.h
        Source/ui/skia/TransportBar.cpp
        Source/ui/skia/BrowserPanel.h
        Source/ui/skia/BrowserPanel.cpp
        Source/ui/skia/RightSidePanel.h
        Source/ui/skia/RightSidePanel.cpp
        Source/ui/skia/BottomBar.h
        Source/ui/skia/BottomBar.cpp

        # Session View (Clip Launcher)
        Source/ui/views/SessionViewComponent.h
        Source/ui/views/SessionViewComponent.cpp

        # Main Layout Manager
        Source/ui/MainLayoutComponent.h
    )

    # Add include directories
    target_include_directories(ZenithDAW PRIVATE
        ${SKIA_INCLUDE_DIR}
        ${SKIA_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/skia
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/rendering
    )

    # Link Skia library
    target_link_libraries(ZenithDAW PRIVATE ${SKIA_LIBRARY})

    # Add compile definitions
    target_compile_definitions(ZenithDAW PRIVATE 
        ZENITH_ENABLE_SKIA=1 
        ZENITH_USE_SKIA=1
        SK_GL=1
    )

    # Link OpenGL
    if(WIN32)
        target_link_libraries(ZenithDAW PRIVATE opengl32)
        # Link D3D12 and D3DCompiler for Skia GPU support
        target_link_libraries(ZenithDAW PRIVATE d3d12 d3dcompiler)
    elseif(APPLE)
        target_link_libraries(ZenithDAW PRIVATE "-framework OpenGL")
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(ZenithDAW PRIVATE OpenGL::GL)
    endif()

    message(STATUS "  Skia backend: OpenGL")
    message(STATUS "Zenith DAW: Skia rendering ENABLED")
    message(STATUS "============================================")
else()
    message(STATUS "Zenith DAW: Skia rendering DISABLED (using JUCE fallback)")
endif()
