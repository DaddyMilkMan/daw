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

# Platform-specific sources are handled via Source/platform/[os]/master

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
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ArrangerRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ArrangerInputHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ArrangerClipManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ArrangerGridUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/GridResolutionDropdown.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ArrangerTrackComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/ClipComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/TakeFolderComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/AutomationLaneComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arranger/TimelineRuler.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/mixer/PluginBrowser.cpp

)

# -----------------------------------------------------------------------------
# Piano Roll / MIDI Editor
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/piano-roll/PianoRollComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/piano-roll/ModulationMatrixView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/piano-roll/QuantizeSettingsComponent.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/views/SessionViewComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session/DrumPadComponent.cpp
)

# -----------------------------------------------------------------------------
# AI Views
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/views/AIMixAssistantView.cpp
)

# -----------------------------------------------------------------------------
# Browser Panel
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserWaveformLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserFilterBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserSearchBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserPreviewPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserHoverPreview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/BrowserRecentSidebar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/PluginBrowserComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/PresetBrowserComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/panels/InstrumentBrowserPanel.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/common/UndoHistoryPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/ResizablePanelContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/MenuBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/ZenithHubComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/WingmanPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/AIAssistantPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/MacroToolbar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/AudioFeedback.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/SettingsComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/HardwareControlPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/PluginEditorWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/PianoKeyboardViewSkia.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/AIAssistantPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/HelpViewPanel.cpp
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
    # TEMPORARILY DISABLED - depends on disabled PresetGeneticistAgent
    # ${CMAKE_CURRENT_SOURCE_DIR}/instruments/PresetGeneticistView.cpp
)

# -----------------------------------------------------------------------------
# Controls (Widgets)
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    # Core Skia widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaSpectrumComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/DebugConsoleComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/DeviceChainComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SpectraAnalyzerComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/MarkdownComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ExportProgressBar.cpp
    
    # Zenith widget system
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithKnob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithDropdown.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithToggle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithTextInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithModMatrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithTooltipOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ZenithVisualizer.cpp
    
    # Dialog components
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaAlertWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaComboBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaFileChooser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaLabel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaListBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaPopupMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/ContextMenuManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/SkiaTextEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/CollabPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controls/FreezeProgressOverlay.cpp
)

# -----------------------------------------------------------------------------
# Rendering (moved from separate subdirectory)
# -----------------------------------------------------------------------------
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rendering/SkiaRenderer.cpp
)
