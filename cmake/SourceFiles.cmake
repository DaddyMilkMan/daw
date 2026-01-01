# =============================================================================
# SOURCE FILE ORGANIZATION
# =============================================================================

# Core application sources
set(ZENITH_APP_SOURCES
    apps/desktop/Source/Main.cpp
    apps/desktop/Source/utils/SampleGenerator.cpp
)

# Engine sources - core audio processing
set(ZENITH_ENGINE_SOURCES
    apps/desktop/Source/engine/Engine.cpp
    apps/desktop/Source/engine/EngineTransport.cpp
    apps/desktop/Source/engine/EngineTrackManagement.cpp
    apps/desktop/Source/engine/EngineMixing.cpp
    apps/desktop/Source/engine/EngineRecording.cpp
    apps/desktop/Source/engine/EngineExport.cpp
    apps/desktop/Source/engine/EngineMIDI.cpp
    apps/desktop/Source/engine/EngineSync.cpp
    apps/desktop/Source/engine/EnginePlugins.cpp
    apps/desktop/Source/engine/ProjectState.cpp
    apps/desktop/Source/engine/ProjectFileIO.cpp
    apps/desktop/Source/engine/TrackStateManager.cpp
    apps/desktop/Source/engine/PluginChain.cpp
    apps/desktop/Source/engine/PluginAutomationBinding.cpp
    apps/desktop/Source/engine/Track.cpp
    apps/desktop/Source/engine/Clip.cpp
    apps/desktop/Source/engine/ClipStateManager.cpp
    apps/desktop/Source/engine/MixerChannel.cpp
    apps/desktop/Source/engine/AudioFilePool.cpp
    apps/desktop/Source/engine/PluginHost.cpp
    apps/desktop/Source/engine/AuxBus.cpp
    apps/desktop/Source/engine/ZenithLogger.cpp
    apps/desktop/Source/engine/RealTimeGarbageCollector.cpp
    apps/desktop/Source/engine/TrackPluginState.cpp
    apps/desktop/Source/engine/AudioRecorder.cpp
    apps/desktop/Source/engine/RoutingGraph.cpp
    apps/desktop/Source/engine/TrackFreeze.cpp
    apps/desktop/Source/engine/AudioRenderer.cpp
    apps/desktop/Source/engine/RecordingManager.cpp
    apps/desktop/Source/engine/TransportController.cpp
    apps/desktop/Source/engine/MeteringSystem.cpp
    apps/desktop/Source/engine/AutomationStateManager.cpp
    apps/desktop/Source/engine/Metronome.cpp
    apps/desktop/Source/engine/AudioTrack.cpp
    apps/desktop/Source/engine/MIDITrack.cpp
    apps/desktop/Source/engine/InstrumentTrack.cpp
    apps/desktop/Source/engine/TakeFolder.cpp
    apps/desktop/Source/engine/AuxBusTrack.cpp
    apps/desktop/Source/engine/AutomationManager.cpp
    apps/desktop/Source/engine/TempoMap.cpp
    apps/desktop/Source/engine/TempoMap.cpp
    apps/desktop/Source/engine/RecentProjectManager.cpp
    apps/desktop/Source/engine/MidiNoteStateManager.cpp
    apps/desktop/Source/engine/PlatformAudioUtils.cpp
)

# Legacy/Deprecated components (kept for compatibility)
set(ZENITH_LEGACY_SOURCES
    apps/desktop/Source/engine/ClipSynchronizer.cpp
    apps/desktop/Source/engine/TrackStateSynchronizer.cpp
    apps/desktop/Source/engine/TrackAutomationSynchronizer.cpp
    apps/desktop/Source/engine/TempoMapSynchronizer.cpp
)

# UI Framework sources
set(ZENITH_UI_FRAMEWORK_SOURCES
    apps/desktop/Source/ui/design-system/ZenithLayout.cpp
    apps/desktop/Source/ui/framework/SkiaMainWindowIntegration.cpp
    apps/desktop/Source/ui/framework/AuroraBackground.cpp
    apps/desktop/Source/ui/framework/SkiaComponent.cpp
    apps/desktop/Source/ui/framework/SkiaLayout.cpp
    apps/desktop/Source/ui/framework/PlatformWindowUtils.cpp
    apps/desktop/Source/ui/framework/PlatformWindowUtils.cpp
    apps/desktop/Source/ui/framework/PlatformPathUtils.cpp
    apps/desktop/Source/ui/framework/PlatformDisplayUtils.cpp
    apps/desktop/Source/ui/design-system/ZenithTheme.cpp
    apps/desktop/Source/ui/design-system/ZenithDesignSystem.cpp
    apps/desktop/Source/ui/design-system/ZenithLookAndFeel.cpp
    apps/desktop/Source/ui/design-system/FontManager.cpp
    apps/desktop/Source/ui/design-system/PlatformFontUtils.cpp
    apps/desktop/Source/ui/framework/ConfigurationManager.cpp
    apps/desktop/Source/ui/framework/LayoutManager.cpp
    apps/desktop/Source/rendering/SkiaRenderer.cpp
    apps/desktop/Source/rendering/SkiaLinkerFix.cpp
    apps/desktop/Source/ui/framework/AnimationCoordinator.cpp
    apps/desktop/Source/ui/design-system/MeterRenderer.cpp
)

# UI Components sources
set(ZENITH_UI_COMPONENTS_SOURCES
    apps/desktop/Source/ui/controls/ZenithButton.cpp
    apps/desktop/Source/ui/controls/ZenithSlider.cpp
    apps/desktop/Source/ui/controls/ZenithKnob.cpp
    apps/desktop/Source/ui/controls/ZenithToggle.cpp
    apps/desktop/Source/ui/controls/ZenithTextInput.cpp
    apps/desktop/Source/ui/controls/ZenithDropdown.cpp
    apps/desktop/Source/ui/controls/ZenithModMatrix.cpp
    apps/desktop/Source/ui/controls/ZenithTooltipOverlay.cpp
    apps/desktop/Source/ui/controls/ZenithControl.cpp
    apps/desktop/Source/ui/controls/ZenithVisualizer.cpp
    apps/desktop/Source/ui/controls/SkiaSpectrumComponent.cpp
    apps/desktop/Source/ui/controls/ContextMenuManager.cpp
    apps/desktop/Source/ui/controls/SkiaPopupMenu.cpp
    apps/desktop/Source/ui/controls/SkiaListBox.cpp
    apps/desktop/Source/ui/controls/SkiaComboBox.cpp
    apps/desktop/Source/ui/controls/SkiaLabel.cpp
    apps/desktop/Source/ui/controls/SkiaAlertWindow.cpp
    apps/desktop/Source/ui/controls/SkiaFileChooser.cpp
    apps/desktop/Source/ui/controls/SkiaTextEditor.cpp
    apps/desktop/Source/ui/controls/ExportProgressBar.cpp
    apps/desktop/Source/ui/controls/FreezeProgressOverlay.cpp
    apps/desktop/Source/ui/controls/CollabPanel.cpp
    apps/desktop/Source/ui/controls/DebugConsoleComponent.cpp
    apps/desktop/Source/ui/controls/DeviceChainComponent.cpp
    apps/desktop/Source/ui/controls/SpectraAnalyzerComponent.cpp
    apps/desktop/Source/ui/controls/MarkdownComponent.cpp
)

# Main UI Components sources
set(ZENITH_UI_MAIN_SOURCES
    apps/desktop/Source/ui/common/MainWindow.cpp
    apps/desktop/Source/ui/common/MainLayoutComponent.cpp
    apps/desktop/Source/ui/common/BottomBar.cpp
    apps/desktop/Source/ui/common/MenuBar.cpp
    apps/desktop/Source/ui/common/PianoKeyboardViewSkia.cpp
    apps/desktop/Source/ui/common/ResizablePanelContainer.cpp
    apps/desktop/Source/ui/common/RightSidePanel.cpp
    apps/desktop/Source/ui/common/UndoHistoryPanel.cpp
    apps/desktop/Source/ui/common/WingmanPanel.cpp
    apps/desktop/Source/ui/common/AIAssistantPanel.cpp
    apps/desktop/Source/ui/common/AudioFeedback.cpp
    apps/desktop/Source/ui/common/HelpViewPanel.cpp
    apps/desktop/Source/ui/common/MacroToolbar.cpp
    apps/desktop/Source/ui/common/ZenithHubComponent.cpp
    apps/desktop/Source/ui/common/TitleBarComponent.cpp
)

# Browser UI sources
set(ZENITH_UI_BROWSER_SOURCES
    apps/desktop/Source/ui/browser/BrowserPanel.cpp
    apps/desktop/Source/ui/panels/BrowserFilterBar.cpp
    apps/desktop/Source/ui/panels/BrowserHoverPreview.cpp
    apps/desktop/Source/ui/panels/BrowserListView.cpp
    apps/desktop/Source/ui/panels/BrowserPreviewPanel.cpp
    apps/desktop/Source/ui/panels/BrowserRecentSidebar.cpp
    apps/desktop/Source/ui/panels/BrowserSearchBar.cpp
    apps/desktop/Source/ui/panels/BrowserWaveformLoader.cpp
    apps/desktop/Source/ui/panels/InstrumentBrowserPanel.cpp
    apps/desktop/Source/ui/panels/PluginBrowserComponent.cpp
    apps/desktop/Source/ui/panels/PresetBrowserComponent.cpp
)

# Specialized UI sources
set(ZENITH_UI_SPECIALIZED_SOURCES
    apps/desktop/Source/ui/arranger/ArrangerClipManager.cpp
    apps/desktop/Source/ui/arranger/ArrangerComponent.cpp
    apps/desktop/Source/ui/arranger/ArrangerGridUtils.cpp
    apps/desktop/Source/ui/arranger/ArrangerInputHandler.cpp
    apps/desktop/Source/ui/arranger/ArrangerRenderer.cpp
    apps/desktop/Source/ui/arranger/ArrangerTrackComponent.cpp
    apps/desktop/Source/ui/arranger/AutomationLaneComponent.cpp
    apps/desktop/Source/ui/arranger/ClipComponent.cpp
    apps/desktop/Source/ui/arranger/GridResolutionDropdown.cpp
    apps/desktop/Source/ui/arranger/MarkerLaneComponent.cpp
    apps/desktop/Source/ui/arranger/MiniMapComponent.cpp
    apps/desktop/Source/ui/arranger/TakeFolderComponent.cpp
    apps/desktop/Source/ui/arranger/TempoLaneComponent.cpp
    apps/desktop/Source/ui/arranger/TimelineRuler.cpp
    apps/desktop/Source/ui/mixer/MixerChannelComponent.cpp
    apps/desktop/Source/ui/mixer/TrackGroupHeader.cpp
    apps/desktop/Source/ui/mixer/MixerComponent.cpp
    apps/desktop/Source/ui/mixer/PluginBrowser.cpp
    apps/desktop/Source/ui/transport/TransportBar.cpp
    apps/desktop/Source/ui/piano-roll/PianoRollComponent.cpp
    apps/desktop/Source/ui/piano-roll/ModulationMatrixView.cpp
    apps/desktop/Source/ui/piano-roll/QuantizeSettingsComponent.cpp
    apps/desktop/Source/ui/session/DrumPadComponent.cpp
    apps/desktop/Source/ui/views/SessionViewComponent.cpp
    apps/desktop/Source/ui/sample-editor/SampleEditorComponent.cpp
    apps/desktop/Source/ui/instruments/ZenithPolySynthUI.cpp
)

# Dialog sources
set(ZENITH_UI_DIALOGS_SOURCES
    apps/desktop/Source/ui/dialogs/AdaptiveUISettings.cpp
    apps/desktop/Source/ui/dialogs/ExportDialog.cpp
    apps/desktop/Source/ui/dialogs/HardwareControlPanel.cpp
    apps/desktop/Source/ui/dialogs/SettingsComponent.cpp
    apps/desktop/Source/ui/dialogs/ProjectRecoveryModal.cpp
    apps/desktop/Source/ui/settings/GlobalSettingsPanel.cpp
    apps/desktop/Source/ui/dialogs/UnsavedChangesModal.cpp
)

# Instruments sources
set(ZENITH_INSTRUMENTS_SOURCES
    apps/desktop/Source/instruments/Instrument.cpp
    apps/desktop/Source/instruments/InstrumentRegistry.cpp
    apps/desktop/Source/instruments/RegisterBuiltInInstruments.cpp
    apps/desktop/Source/instruments/PresetGenerator.cpp
    apps/desktop/Source/instruments/ZenithOscillator.cpp
    apps/desktop/Source/instruments/ZenithFilter.cpp
    apps/desktop/Source/instruments/ZenithEffects.cpp
    apps/desktop/Source/instruments/ZenithPolySynthVoice.cpp
    apps/desktop/Source/instruments/ZenithPolySynth.cpp
    apps/desktop/Source/instruments/ZenithPolySynthParameterManager.cpp
    apps/desktop/Source/instruments/ZenithPolySynthEditor.cpp
    apps/desktop/Source/instruments/ZenithPresetManager.cpp
    apps/desktop/Source/instruments/ZenithSampler.cpp
    apps/desktop/Source/instruments/ZenithSamplerEditor.cpp
)

# Effects and Plugins sources
set(ZENITH_EFFECTS_SOURCES
    apps/desktop/Source/effects/ConsoleEmulation.cpp
    apps/desktop/Source/effects/ZenithChannelStrip.cpp
    apps/desktop/Source/effects/ZenithDeEsser.cpp
    apps/desktop/Source/effects/ZenithTransientShaper.cpp
    apps/desktop/Source/effects/ZenithVoiceChanger.cpp
    apps/desktop/Source/plugins/InternalPluginFormat.cpp
    apps/desktop/Source/plugins/ZenithPlugin.cpp
    apps/desktop/Source/plugins/ZenithPluginEditor.cpp
    apps/desktop/Source/plugins/modulation/ZenithTremolo.cpp
)

# DSP and Audio Processing sources
set(ZENITH_DSP_SOURCES
    apps/desktop/Source/dsp/DSPVoiceChanger.cpp
    apps/desktop/Source/dsp/DSPStemSeparator.cpp
    apps/desktop/Source/dsp/ONNXStemSeparator.cpp
    apps/desktop/Source/dsp/SpectralProcessor.cpp
    apps/desktop/Source/dsp/TimeStretcher.cpp
)

# AI and Network sources
set(ZENITH_AI_NETWORK_SOURCES
    apps/desktop/Source/ai/AIEventBus.cpp
    apps/desktop/Source/ai/AIStatusManager.cpp
    apps/desktop/Source/ai/AudioFitnessEvaluator.cpp
    apps/desktop/Source/ai/ZenithStyleApplicator.cpp
    apps/desktop/Source/ai/AIMasteringAgent.cpp
    apps/desktop/Source/ai/AIResponseCache.cpp
    apps/desktop/Source/ai/PresetGeneticistAgent.cpp
    apps/desktop/Source/ai/PresetSuggestionService.cpp
    apps/desktop/Source/ai/ProjectRefactorerAgent.cpp
    apps/desktop/Source/ai/SampleHunterAgent.cpp
    apps/desktop/Source/ai/SessionDebuggerAgent.cpp
    apps/desktop/Source/ai/UXDirectorAgent.cpp
    apps/desktop/Source/network/SecureKeyStore.cpp
    apps/desktop/Source/network/GrokUtils.cpp
    apps/desktop/Source/network/GrokDAWController.cpp
    apps/desktop/Source/network/GrokDAWClient.cpp
    apps/desktop/Source/network/CollaborationManager.cpp
    apps/desktop/Source/network/DTLSSocket.cpp
    apps/desktop/Source/network/FreesoundClient.cpp
    apps/desktop/Source/network/AIPrompts.cpp
    apps/desktop/Source/network/AITools.cpp
    apps/desktop/Source/network/AuthenticationService.cpp
    apps/desktop/Source/network/OAuthRedirectServer.cpp
    apps/desktop/Source/network/MCPServer.cpp
    apps/desktop/Source/network/AudioAnalysisService.cpp
    apps/desktop/Source/utils/PlatformSystemUtils.cpp
    apps/desktop/Source/dsp/PlatformModelUtils.cpp
    apps/desktop/Source/engine/ClipTrack.cpp
)

# Utility and Management sources
set(ZENITH_UTILS_SOURCES
    apps/desktop/Source/utils/AudioAnalysisUtils.cpp
    apps/desktop/Source/utils/PlatformLogUtils.cpp
    apps/desktop/Source/utils/StemSeparationJob.cpp
    apps/desktop/Source/engine/MixerController.cpp
    apps/desktop/Source/engine/TrackProcessor.cpp
    apps/desktop/Source/engine/AudioExporter.cpp
    apps/desktop/Source/engine/PropertyExchangeManager.cpp
    apps/desktop/Source/engine/Midi2DiscoveryService.cpp
    apps/desktop/Source/engine/AudioAsMidiBridge.cpp
    apps/desktop/Source/commands/CommandAPI.cpp
    apps/desktop/Source/commands/TrackCommands.cpp
    apps/desktop/Source/commands/TransportCommands.cpp
    apps/desktop/Source/commands/ClipCommands.cpp
    apps/desktop/Source/commands/SessionGraph.cpp
    apps/desktop/Source/browser/BrowserModel.cpp
    apps/desktop/Source/browser/BrowserPreviewEngine.cpp
    apps/desktop/Source/browser/BrowserScanner.cpp
    apps/desktop/Source/ui/common/PluginEditorWindow.cpp
)

# Combine all sources
set(ZENITH_ALL_SOURCES
    ${ZENITH_APP_SOURCES}
    ${ZENITH_ENGINE_SOURCES}
    ${ZENITH_LEGACY_SOURCES}
    ${ZENITH_UI_FRAMEWORK_SOURCES}
    ${ZENITH_UI_COMPONENTS_SOURCES}
    ${ZENITH_UI_MAIN_SOURCES}
    ${ZENITH_UI_BROWSER_SOURCES}
    ${ZENITH_UI_SPECIALIZED_SOURCES}
    ${ZENITH_UI_DIALOGS_SOURCES}
    ${ZENITH_INSTRUMENTS_SOURCES}
    ${ZENITH_EFFECTS_SOURCES}
    ${ZENITH_DSP_SOURCES}
    ${ZENITH_AI_NETWORK_SOURCES}
    ${ZENITH_UTILS_SOURCES}
)

# Platform-specific sources
if(UNIX AND NOT APPLE)
    list(APPEND ZENITH_ALL_SOURCES
        apps/desktop/Source/platform/linux/engine/PlatformAudioUtils_Linux.cpp
        apps/desktop/Source/platform/linux/window/PlatformWindowUtils_Linux.cpp
    )
endif()

message(STATUS "Zenith DAW: ${CMAKE_LIST_LENGTH} source files organized")
