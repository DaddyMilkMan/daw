if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW PRIVATE
        # Core UI
        AuroraBackground.cpp
        ../common/BottomBar.cpp
        ../browser/BrowserPanel.cpp
        DebugConsoleComponent.cpp
        ../common/RightSidePanel.cpp
        SkiaComponent.cpp
        SkiaMainWindowIntegration.cpp
        ../transport/TransportBar.cpp
        ../design-system/ZenithDesignSystem.cpp
        ../design-system/FontManager.cpp
        ../design-system/ZenithLayout.cpp

        ../instruments/ZenithPolySynthUI.cpp
        ../dialogs/ExportDialog.cpp
        DeviceChainComponent.cpp
        ../../rendering/SkiaRenderer.cpp
        
        # Legacy Skia widgets (to be deprecated)
        SkiaButton.cpp
        SkiaKnob.cpp
        SkiaSlider.cpp
        
        # New Zenith widget system
        widgets/ZenithControl.cpp
        widgets/ZenithKnob.cpp
        widgets/ZenithSlider.cpp
        widgets/ZenithButton.cpp
        widgets/ZenithDropdown.cpp
        widgets/ZenithToggle.cpp
        widgets/ZenithTextInput.cpp
        widgets/ZenithModMatrix.cpp
        widgets/ZenithTooltipOverlay.cpp
        widgets/ZenithVisualizer.cpp
    )
endif()
