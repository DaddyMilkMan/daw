if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW PRIVATE
        BottomBar.cpp
        BrowserPanel.cpp
        RightSidePanel.cpp
        SkiaButton.cpp
        SkiaComponent.cpp
        SkiaKnob.cpp
        SkiaMainWindowIntegration.cpp
        SkiaSlider.cpp
        TransportBar.cpp
        ZenithDesignSystem.cpp
        ZenithLayout.cpp
        ZenithPolySynthUI.cpp
        ExportDialog.cpp
        DebugConsoleComponent.cpp
    )
endif()
