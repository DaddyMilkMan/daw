# ============================================================================
# Skia Integration via vcpkg
# ============================================================================
# This file integrates Skia from vcpkg installation
# 
# Prerequisites:
#   - Skia installed via vcpkg: vcpkg install skia:x64-windows
#   - CMake configured with: -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
# ============================================================================

if(ZENITH_ENABLE_SKIA)
    message(STATUS "============================================")
    message(STATUS "Configuring Skia Integration (vcpkg)")
    message(STATUS "============================================")

    # Find Skia via vcpkg
    find_package(unofficial-skia CONFIG)
    
    if(NOT unofficial-skia_FOUND)
        message(WARNING 
            "\n"
            "Skia package not found via vcpkg!\n"
            "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}\n"
            "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}\n"
            "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}\n"
        )
        set(ZENITH_ENABLE_SKIA OFF)
        return()
    endif()

    message(STATUS "  Skia found via vcpkg (unofficial-skia)")
    message(STATUS "  Skia target: unofficial::skia::skia")

    # Add Skia source files
    target_sources(ZenithDAW PRIVATE
        # Skia rendering core (SkiaRenderer and SkiaContextManager removed - unused)
        # Direct framebuffer rendering via SkiaMainWindowIntegration instead

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

        # =======================================================================
        # OBSOLETE COMPONENTS (Use SkiaRenderer - DELETED)
        # =======================================================================
        # These components use the OLD architecture with per-component SkiaRenderer.
        # They reference the deleted SkiaRenderer class and WILL cause compilation errors.
        # Use SkiaButtonComponent_NEW.h instead (uses SkiaComponent base class).
        #
        # Source/ui/skia/SkiaKnobComponent.h
        # Source/ui/skia/SkiaKnobComponent.cpp
        # Source/ui/skia/SkiaSliderComponent.h
        # Source/ui/skia/SkiaSliderComponent.cpp
        # Source/ui/skia/SkiaButtonComponent.h
        # Source/ui/skia/SkiaButtonComponent.cpp

        # =======================================================================
        # DISABLED COMPONENTS (Merge Conflict Damage - Need Manual Repair)
        # =======================================================================
        # TODO: These components exist on disk but are disabled due to merge
        # conflicts from previous UI transformation. To re-enable:
        # 1. Review each .h/.cpp pair for compilation errors
        # 2. Fix any broken includes or API changes
        # 3. Uncomment the lines below
        # 4. Test build
        #
        # Priority order for re-enabling:
        # - SkiaMixerChannelComponent (critical for mixing)
        # - SkiaTransportControlComponent (transport bar alternative)
        # - SkiaMasterOutputMeterComponent (master output metering)
        # - SkiaEffectsChainComponent, SkiaInstrumentBrowserComponent,
        #   SkiaPresetBrowserComponent, SkiaSettingsManager,
        #   SkiaPerformanceDashboard (nice-to-have)
        #
        # Source/ui/skia/SkiaMixerChannelComponent.h
        # Source/ui/skia/SkiaMixerChannelComponent.cpp
        # Source/ui/skia/SkiaTransportControlComponent.h
        # Source/ui/skia/SkiaTransportControlComponent.cpp
        # Source/ui/skia/SkiaMasterOutputMeterComponent.h
        # Source/ui/skia/SkiaMasterOutputMeterComponent.cpp
        # Source/ui/skia/SkiaEffectsChainComponent.h
        # Source/ui/skia/SkiaEffectsChainComponent.cpp
        # Source/ui/skia/SkiaInstrumentBrowserComponent.h
        # Source/ui/skia/SkiaInstrumentBrowserComponent.cpp
        # Source/ui/skia/SkiaPresetBrowserComponent.h
        # Source/ui/skia/SkiaPresetBrowserComponent.cpp
        # Source/ui/skia/SkiaSettingsManager.h
        # Source/ui/skia/SkiaSettingsManager.cpp
        # Source/ui/skia/SkiaPerformanceDashboard.h
        # Source/ui/skia/SkiaPerformanceDashboard.cpp
        # =======================================================================

        Source/ui/skia/SkiaMainWindowIntegration.h
        Source/ui/skia/SkiaMainWindowIntegration.cpp
        Source/ui/skia/SkiaCanvasComponent.cpp
        Source/ui/skia/SkiaColorTestComponent.h
        Source/ui/skia/SkiaColorTestComponent.cpp
        Source/ui/views/PianoKeyboardViewSkia.cpp

        # Logic Pro UI Transformation Components (NEW)
        Source/ui/skia/SkiaTrackHeaderComponent.h
        Source/ui/skia/SkiaTrackHeaderComponent.cpp
        Source/ui/skia/SkiaArrangementViewComponent.h
        Source/ui/skia/SkiaArrangementViewComponent.cpp
        Source/ui/skia/SkiaPianoRollComponent.h
        Source/ui/skia/SkiaPianoRollComponent.cpp

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

    # Add include directories for Skia UI components
    target_include_directories(ZenithDAW PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/skia
    )

    # Link Skia library via vcpkg target
    target_link_libraries(ZenithDAW PRIVATE unofficial::skia::skia)

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
