if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW PRIVATE
        # Core UI
        BottomBar.cpp
        BrowserPanel.cpp
        DebugConsoleComponent.cpp
        RightSidePanel.cpp
        SkiaComponent.cpp
        SkiaMainWindowIntegration.cpp
        TransportBar.cpp
        ZenithDesignSystem.cpp
        ZenithLayout.cpp
        FontManager.cpp
        ZenithPolySynthUI.cpp
        ExportDialog.cpp
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
