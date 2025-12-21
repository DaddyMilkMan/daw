# =============================================================================
# Zenith DAW - UI Module CMakeLists.txt
# Organized by domain for maintainability
# =============================================================================

# -----------------------------------------------------------------------------
# Framework (Core Skia components)
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/SkiaComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/SkiaMainWindowIntegration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/AuroraBackground.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/LayoutManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/ComponentLifecycleManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/ConfigurationManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/SkiaLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/UITestFramework.cpp
)

# -----------------------------------------------------------------------------
# Design System (Theming, fonts, icons)
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/design-system/ZenithDesignSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/design-system/FontManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/design-system/ZenithLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/design-system/ZenithLookAndFeel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/design-system/ZenithTheme.cpp
)

# -----------------------------------------------------------------------------
# Arranger View
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ArrangerComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ArrangerTrackComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/TrackHeaderComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ModernTrackHeader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ClipComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/AutomationLaneComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/TimelineRuler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ModernTimelineRuler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/MarkerLaneComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/TempoLaneComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/MiniMapComponent.cpp
)

# -----------------------------------------------------------------------------
# Mixer View
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mixer/MixerComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mixer/MixerChannelComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mixer/MixerView.cpp
)

# -----------------------------------------------------------------------------
# Piano Roll / MIDI Editor
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/piano-roll/PianoRollComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/piano-roll/ModulationMatrixView.cpp
)

# -----------------------------------------------------------------------------
# Sample Editor
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sample-editor/SampleEditorComponent.cpp
)

# -----------------------------------------------------------------------------
# Session View
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/session/SessionViewComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session/DrumPadComponent.cpp
)

# -----------------------------------------------------------------------------
# Browser Panel
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/browser/BrowserPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/browser/PluginBrowserComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/browser/PresetBrowserComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/browser/InstrumentBrowserPanel.cpp
)

# -----------------------------------------------------------------------------
# Transport
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/TransportBar.cpp
)

# -----------------------------------------------------------------------------
# Common / Layout
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/MainLayoutComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/BottomBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/RightSidePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/ResizablePanelContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/MenuBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/ZenithHubComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/WingmanPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/MacroToolbar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/AudioFeedback.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/PluginEditorWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/PianoKeyboardViewSkia.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/AIAssistantPanel.cpp
)

# -----------------------------------------------------------------------------
# Dialogs
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/ExportDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/AdaptiveUISettings.cpp
)

# -----------------------------------------------------------------------------
# Instrument UIs
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/instruments/ZenithPolySynthUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/instruments/PresetGeneticistView.cpp
)

# -----------------------------------------------------------------------------
# Widgets (Reusable controls)
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    # Core Skia widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaKnob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaSpectrumComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/DebugConsoleComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/DeviceChainComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SpectraAnalyzerComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/MarkdownComponent.cpp
    
    # Zenith widget system
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithKnob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithDropdown.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithToggle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithTextInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithModMatrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithTooltipOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/ZenithVisualizer.cpp
    
    # Dialog components
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaAlertWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaComboBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaFileChooser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaLabel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaListBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaPopupMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/SkiaTextEditor.cpp
)

# -----------------------------------------------------------------------------
# Rendering (moved from separate subdirectory)
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rendering/SkiaRenderer.cpp
)
