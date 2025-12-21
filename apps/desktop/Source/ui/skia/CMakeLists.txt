if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW_lib PRIVATE
        # Core UI
        AuroraBackground.cpp
        BottomBar.cpp
        BrowserPanel.cpp
        DebugConsoleComponent.cpp
        RightSidePanel.cpp
        SkiaComponent.cpp
        SkiaMainWindowIntegration.cpp
        TransportBar.cpp
        ZenithDesignSystem.cpp
        FontManager.cpp
        ZenithLayout.cpp

        ZenithPolySynthUI.cpp
        ExportDialog.cpp
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


