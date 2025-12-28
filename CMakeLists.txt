cmake_minimum_required(VERSION 3.25)

# 1. Project Definition
project(ZenithDAW 
    VERSION 0.1.0
    LANGUAGES CXX C
)

# 2. Build Performance Optimizations
# Early-stage optimizations that must be set before target creation
include(apps/desktop/cmake/BuildOptimizations.cmake)

# 3. Default Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /bigobj" CACHE STRING "Flags used by the CXX compiler during all build types." FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /bigobj" CACHE STRING "Flags used by the C compiler during all build types." FORCE)
endif()





# 4. Options
option(ENABLE_IPO "Enable Interprocedural Optimization (LTO)" OFF)
option(ENABLE_SPECTRE "Enable Spectre Mitigations (MSVC)" OFF)

# Auto-enable sanitizers and hardening in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" ON)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" ON)
else()
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
endif()

# Add sanitizer flags if enabled
if(ENABLE_SANITIZERS)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# 5. C++ Standard (Enforced later via target_compile_features)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always enable Skia (Phase 2 decision)
# add_definitions(-DZENITH_USE_SKIA=1) - Removed: Handled via target_compile_definitions in SkiaManualIntegration.cmake

# JUCE Setup - prefer local directory if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    message(STATUS "Using local JUCE from external/JUCE")
    add_subdirectory(external/JUCE)
else()
    message(STATUS "Fetching JUCE from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.0
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Find Skia (from vcpkg)
# find_package(unofficial-skia CONFIG REQUIRED) - Using Manual Integration

# Source files
set(ZENITH_SOURCES
    apps/desktop/Source/Main.cpp
    apps/desktop/Source/utils/SampleGenerator.cpp
)

# Test sources (optional, can be disabled)
set(ZENITH_TEST_SOURCES
    apps/desktop/Source/tests/TestMain.cpp
    apps/desktop/Source/tests/AudioEngineTests.cpp
    apps/desktop/Source/tests/ProjectStateTests.cpp
    apps/desktop/Source/tests/EngineTests.cpp
    apps/desktop/Source/tests/ConcurrencyTests.cpp
    apps/desktop/Source/tests/ThemeTests.cpp
    apps/desktop/Source/tests/UITests.cpp
    apps/desktop/Source/tests/Midi2Tests.cpp
    apps/desktop/Source/tests/AITests.cpp
    apps/desktop/Source/tests/AccessibilityTests.cpp
    apps/desktop/Source/tests/CRDTSyncTests.cpp
    apps/desktop/Source/tests/OnnxPipelineTests.cpp
    apps/desktop/Source/tests/PluginAutomationTests.cpp
    apps/desktop/Source/tests/SampleEditorRecordingTests.cpp
    apps/desktop/Source/tests/CollaborationMultiPeerTest.cpp
    apps/desktop/Source/tests/CollaborationSecurityTest.cpp
    apps/desktop/Source/tests/DSPVoiceChangerTest.cpp
    apps/desktop/Source/tests/RecordingTempoTest.cpp
    # apps/desktop/Source/tests/PluginBufferingTests.cpp  # Still broken in JUCE 8
    apps/desktop/Source/tests/RecordingTests.cpp
    apps/desktop/Source/tests/RoutingGraphTest.cpp
    apps/desktop/Source/tests/ExportTests.cpp
)

# Common sources used by both DAW and Tests
set(ZENITH_COMMON_SOURCES
    apps/desktop/Source/ui/design-system/ZenithLayout.cpp
    apps/desktop/Source/ui/framework/UITestFramework.cpp
    
    # Engine sources
    apps/desktop/Source/engine/Engine.cpp
    apps/desktop/Source/engine/EngineSync.cpp
    apps/desktop/Source/engine/EnginePlugins.cpp
    apps/desktop/Source/engine/ProjectState.cpp
    apps/desktop/Source/engine/PluginChain.cpp
    apps/desktop/Source/engine/PluginAutomationBinding.cpp
    apps/desktop/Source/engine/Track.cpp
    apps/desktop/Source/engine/Clip.cpp
    apps/desktop/Source/engine/MixerChannel.cpp
    apps/desktop/Source/engine/AudioFilePool.cpp
    apps/desktop/Source/engine/PluginHost.cpp
    apps/desktop/Source/engine/AuxBus.cpp
    apps/desktop/Source/engine/ZenithLogger.cpp
    apps/desktop/Source/engine/RealTimeGarbageCollector.cpp
    apps/desktop/Source/engine/TrackPluginState.cpp
    apps/desktop/Source/engine/AudioRecorder.cpp
    apps/desktop/Source/engine/RoutingGraph.cpp
    # apps/desktop/Source/engine/ScriptableProcessor.cpp - Missing Lua dependency
    apps/desktop/Source/engine/GrokGodModeHelper.cpp
    apps/desktop/Source/ai/ReferenceMatcher.cpp
    apps/desktop/Source/ai/GrokJourney.cpp

    # Deprecated synchronizers (for reflog/legacy support)
    apps/desktop/Source/engine/ClipSynchronizer.cpp
    apps/desktop/Source/engine/TrackStateSynchronizer.cpp
    apps/desktop/Source/engine/TrackAutomationSynchronizer.cpp
    apps/desktop/Source/engine/TempoMapSynchronizer.cpp
    apps/desktop/Source/engine/TempoMap.cpp
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
    apps/desktop/Source/plugins/modulation/ZenithTremolo.cpp
    # Instruments
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

    # AI (FIXED for JUCE 8 compatibility)
    apps/desktop/Source/ai/GrokAPIClient.h
    apps/desktop/Source/ai/GrokAPIClient.cpp
    apps/desktop/Source/ai/VisualAnalyzer.h
    apps/desktop/Source/ai/VisualAnalyzer.cpp
    apps/desktop/Source/ai/CreativePartner.h
    apps/desktop/Source/ai/CreativePartner.cpp
    apps/desktop/Source/ai/SessionDebuggerAgent.cpp
    apps/desktop/Source/ai/AIMasteringAgent.cpp
    apps/desktop/Source/ai/SampleHunterAgent.cpp
    apps/desktop/Source/ai/UXDirectorAgent.cpp
    apps/desktop/Source/ai/ProjectRefactorerAgent.cpp
    apps/desktop/Source/ai/PresetGeneticistAgent.cpp
    apps/desktop/Source/ai/GrokJourney.cpp
    
    # Audio Components
    apps/desktop/Source/audio/RealTimeAudioBuffer.cpp
    apps/desktop/Source/ai/ReferenceMatcher.cpp
    apps/desktop/Source/ai/GenreDetector.cpp
    apps/desktop/Source/ai/ProjectContext.cpp
    apps/desktop/Source/ai/PresetSuggestionService.cpp
    apps/desktop/Source/ai/AudioThreadSafeProcessor.cpp
    apps/desktop/Source/ai/RealNeuralNetwork.cpp
    apps/desktop/Source/ai/MLGenreClassifier.cpp
    apps/desktop/Source/ai/CreativeNeuralNetwork.cpp
    apps/desktop/Source/ai/PredictiveMastering.cpp
    apps/desktop/Source/ai/GrokAPIReal.cpp
    apps/desktop/Source/engine/ClipTrack.cpp
    apps/desktop/Source/engine/AuxBusTrack.cpp
    apps/desktop/Source/engine/ClipStateManager.cpp
    apps/desktop/Source/engine/TrackStateManager.cpp
    apps/desktop/Source/engine/AutomationManager.cpp
    apps/desktop/Source/engine/PluginChain.cpp
    apps/desktop/Source/engine/RecentProjectManager.cpp
    apps/desktop/Source/engine/ProjectFileIO.cpp
    apps/desktop/Source/engine/ProjectEngineBridge.cpp
    apps/desktop/Source/engine/AudioExporter.cpp
    apps/desktop/Source/engine/ExportJob.cpp
    apps/desktop/Source/utils/AudioAnalysisUtils.cpp
    apps/desktop/Source/ui/common/PluginEditorWindow.cpp
    # apps/desktop/Source/tests/FlecsSyncTests.cpp - Removed
    # Dependencies for tests
    apps/desktop/Source/ui/mixer/MixerChannelComponent.cpp
    apps/desktop/Source/ui/mixer/MixerComponent.cpp
    apps/desktop/Source/ui/views/MixerView.cpp
    apps/desktop/Source/ui/mixer/PluginBrowser.cpp
    apps/desktop/Source/network/AudioAnalysisService.cpp
    apps/desktop/Source/network/GrokDAWController.cpp
    apps/desktop/Source/ui/framework/ConfigurationManager.cpp
    apps/desktop/Source/ui/framework/SkiaComponent.cpp
    apps/desktop/Source/ui/framework/AuroraBackground.cpp
    apps/desktop/Source/ui/framework/SkiaLayout.cpp
    apps/desktop/Source/ui/design-system/ZenithTheme.cpp
    apps/desktop/Source/ui/design-system/ZenithDesignSystem.cpp
    apps/desktop/Source/ui/design-system/ZenithLookAndFeel.cpp
    apps/desktop/Source/ui/design-system/FontManager.cpp
    apps/desktop/Source/ui/controls/SkiaSpectrumComponent.cpp
    apps/desktop/Source/ui/controls/ContextMenuManager.cpp
    apps/desktop/Source/ui/controls/SkiaPopupMenu.cpp
    apps/desktop/Source/platform/linux/fonts/PlatformFontUtils_Linux.cpp

    # Arranger
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

    # Browser
    apps/desktop/Source/ui/panels/BrowserFilterBar.cpp
    apps/desktop/Source/ui/panels/BrowserListView.cpp
    apps/desktop/Source/ui/panels/BrowserPreviewPanel.cpp
    apps/desktop/Source/ui/panels/BrowserSearchBar.cpp
    apps/desktop/Source/ui/panels/BrowserHoverPreview.cpp
    apps/desktop/Source/ui/panels/BrowserPanel.cpp
    apps/desktop/Source/ui/panels/BrowserRecentSidebar.cpp
    apps/desktop/Source/ui/panels/BrowserWaveformLoader.cpp
    apps/desktop/Source/ui/panels/InstrumentBrowserPanel.cpp
    apps/desktop/Source/ui/panels/PluginBrowserComponent.cpp

    # Dialogs
    apps/desktop/Source/ui/dialogs/AdaptiveUISettings.cpp
    apps/desktop/Source/ui/dialogs/ExportDialog.cpp
    apps/desktop/Source/ui/dialogs/HardwareControlPanel.cpp
    apps/desktop/Source/ui/dialogs/SettingsComponent.cpp

    # Transport
    apps/desktop/Source/ui/transport/TransportBar.cpp

    # Piano Roll
    apps/desktop/Source/ui/piano-roll/ModulationMatrixView.cpp
    apps/desktop/Source/ui/piano-roll/PianoRollComponent.cpp
    apps/desktop/Source/ui/piano-roll/QuantizeSettingsComponent.cpp

    # Common & Framework
    apps/desktop/Source/ui/common/AIAssistantPanel.cpp
    apps/desktop/Source/ui/common/AudioFeedback.cpp
    apps/desktop/Source/ui/common/BottomBar.cpp
    apps/desktop/Source/ui/common/HelpViewPanel.cpp
    apps/desktop/Source/ui/common/MacroToolbar.cpp
    apps/desktop/Source/ui/controls/CollabPanel.cpp
    apps/desktop/Source/ui/framework/LayoutManager.cpp
    apps/desktop/Source/ui/controls/SpectraAnalyzerComponent.cpp
    apps/desktop/Source/ui/controls/MarkdownComponent.cpp
    apps/desktop/Source/ui/framework/SkiaMainWindowIntegration.cpp
    apps/desktop/Source/ui/framework/ConfigurationManager.cpp
    apps/desktop/Source/browser/BrowserModel.cpp
    apps/desktop/Source/browser/BrowserPreviewEngine.cpp
    apps/desktop/Source/browser/BrowserScanner.cpp
    apps/desktop/Source/network/GrokDAWController.cpp
    apps/desktop/Source/ui/views/SessionViewComponent.cpp

    apps/desktop/Source/ui/controls/ZenithButton.cpp
    apps/desktop/Source/ui/controls/ZenithSlider.cpp
    apps/desktop/Source/ui/controls/ZenithToggle.cpp
    apps/desktop/Source/ui/controls/ZenithTextInput.cpp
    apps/desktop/Source/ui/controls/ZenithDropdown.cpp
    apps/desktop/Source/ui/controls/ZenithModMatrix.cpp
    apps/desktop/Source/ui/controls/ZenithTooltipOverlay.cpp
    apps/desktop/Source/ui/controls/SkiaListBox.cpp
    apps/desktop/Source/ui/controls/SkiaComboBox.cpp
    apps/desktop/Source/ui/controls/SkiaLabel.cpp
    apps/desktop/Source/ui/controls/SkiaAlertWindow.cpp
    apps/desktop/Source/ui/controls/SkiaFileChooser.cpp
    apps/desktop/Source/ui/controls/SkiaTextEditor.cpp
    apps/desktop/Source/ui/controls/DeviceChainComponent.cpp
    apps/desktop/Source/ui/controls/DebugConsoleComponent.cpp
    apps/desktop/Source/ui/controls/ExportProgressBar.cpp
    apps/desktop/Source/ui/controls/FreezeProgressOverlay.cpp
    apps/desktop/Source/ui/session/DrumPadComponent.cpp
    apps/desktop/Source/platform/linux/window/PlatformWindowUtils_Linux.cpp
    apps/desktop/Source/ui/common/MainLayoutComponent.cpp
    apps/desktop/Source/ui/common/MainWindow.cpp
    apps/desktop/Source/ui/common/MenuBar.cpp
    apps/desktop/Source/ui/common/PianoKeyboardViewSkia.cpp
    apps/desktop/Source/ui/common/ResizablePanelContainer.cpp
    apps/desktop/Source/ui/common/RightSidePanel.cpp
    apps/desktop/Source/ui/common/UndoHistoryPanel.cpp
    apps/desktop/Source/ui/common/WingmanPanel.cpp
    apps/desktop/Source/ui/common/ZenithHubComponent.cpp
    apps/desktop/Source/ai/ZenithStyleApplicator.cpp
    apps/desktop/Source/network/SecureKeyStore.cpp
    apps/desktop/Source/network/AuthenticationService.cpp
    apps/desktop/Source/ui/common/LoginComponent.cpp
    apps/desktop/Source/platform/linux/network/SecureKeyStore_Linux.cpp
    apps/desktop/Source/ui/panels/PresetBrowserComponent.cpp

    apps/desktop/Source/effects/ConsoleEmulation.cpp
    apps/desktop/Source/plugins/InternalPluginFormat.cpp
    apps/desktop/Source/effects/ZenithChannelStrip.cpp
    apps/desktop/Source/effects/ZenithDeEsser.cpp
    apps/desktop/Source/effects/ZenithTransientShaper.cpp
    apps/desktop/Source/effects/ZenithVoiceChanger.cpp
    apps/desktop/Source/plugins/ZenithPlugin.cpp
    apps/desktop/Source/plugins/ZenithPluginEditor.cpp
    apps/desktop/Source/dsp/DSPVoiceChanger.cpp
    apps/desktop/Source/ui/instruments/ZenithPolySynthUI.cpp
    apps/desktop/Source/ui/controls/ZenithControl.cpp
    apps/desktop/Source/ui/controls/ZenithKnob.cpp
    apps/desktop/Source/ui/controls/ZenithButton.cpp
    apps/desktop/Source/ui/controls/ZenithSlider.cpp
    apps/desktop/Source/ui/controls/ZenithToggle.cpp
    apps/desktop/Source/ui/controls/ZenithTextInput.cpp
    apps/desktop/Source/ui/controls/ZenithDropdown.cpp
    apps/desktop/Source/ui/controls/ZenithModMatrix.cpp
    apps/desktop/Source/ui/controls/ZenithTooltipOverlay.cpp
    apps/desktop/Source/rendering/SkiaRenderer.cpp
    apps/desktop/Source/rendering/SkiaLinkerFix.cpp
    apps/desktop/Source/ui/controls/ZenithVisualizer.cpp
    apps/desktop/Source/ui/controls/DeviceChainComponent.cpp
    apps/desktop/Source/ui/controls/DebugConsoleComponent.cpp
    apps/desktop/Source/ui/controls/ExportProgressBar.cpp
    apps/desktop/Source/ui/controls/FreezeProgressOverlay.cpp
    apps/desktop/Source/ui/controls/SkiaComboBox.cpp
    apps/desktop/Source/ui/controls/SkiaLabel.cpp
    apps/desktop/Source/ui/controls/SkiaTextEditor.cpp
    apps/desktop/Source/ui/controls/SkiaAlertWindow.cpp
    apps/desktop/Source/ui/controls/SkiaListBox.cpp
    apps/desktop/Source/ui/controls/SkiaFileChooser.cpp
    apps/desktop/Source/ui/controls/MarkdownComponent.cpp
    apps/desktop/Source/ui/controls/SpectraAnalyzerComponent.cpp
    apps/desktop/Source/ui/controls/CollabPanel.cpp
    apps/desktop/Source/ai/AIEventBus.cpp
    apps/desktop/Source/ai/AIResponseCache.cpp
    apps/desktop/Source/ai/AIStatusManager.cpp
    apps/desktop/Source/ai/AudioFitnessEvaluator.cpp
    apps/desktop/Source/platform/linux/engine/PlatformAudioUtils_Linux.cpp
    apps/desktop/Source/network/GrokUtils.cpp

    apps/desktop/Source/network/GrokDAWController.cpp
    apps/desktop/Source/network/GrokDAWClient.cpp
    apps/desktop/Source/network/CollaborationManager.cpp
    apps/desktop/Source/network/FreesoundClient.cpp
    apps/desktop/Source/network/AIPrompts.cpp
    apps/desktop/Source/network/AITools.cpp
    apps/desktop/Source/network/MCPServer.cpp
    apps/desktop/Source/dsp/DSPStemSeparator.cpp
    apps/desktop/Source/dsp/ONNXStemSeparator.cpp
    apps/desktop/Source/ui/sample-editor/SampleEditorComponent.cpp
    apps/desktop/Source/engine/MixerController.cpp
    apps/desktop/Source/engine/TrackProcessor.cpp
    apps/desktop/Source/utils/StemSeparationJob.cpp
    apps/desktop/Source/dsp/SpectralProcessor.cpp
    apps/desktop/Source/dsp/TimeStretcher.cpp
    apps/desktop/Source/engine/PropertyExchangeManager.cpp
    apps/desktop/Source/engine/Midi2DiscoveryService.cpp
    apps/desktop/Source/engine/AudioAsMidiBridge.cpp
    # TEMPORARILY DISABLED - depends on disabled AI agents
    apps/desktop/Source/commands/CommandAPI.cpp
    apps/desktop/Source/commands/TrackCommands.cpp
    apps/desktop/Source/commands/TransportCommands.cpp
    apps/desktop/Source/commands/ClipCommands.cpp
    apps/desktop/Source/commands/SessionGraph.cpp
    apps/desktop/Source/platform/linux/ui/PlatformDisplayUtils_Linux.cpp
    apps/desktop/Source/platform/linux/system/PlatformSystemUtils_Linux.cpp
    apps/desktop/Source/platform/linux/config/PlatformPathUtils_Linux.cpp
    apps/desktop/Source/platform/linux/window/PlatformWindowUtils_Linux.cpp
)


# Combine sources for the main app
list(APPEND ZENITH_SOURCES ${ZENITH_COMMON_SOURCES})

# Explicitly add SkiaLinkerFix to ensure it's compiled on Linux
if(UNIX AND NOT APPLE AND ZENITH_USE_SKIA)
    list(APPEND ZENITH_SOURCES apps/desktop/Source/rendering/SkiaLinkerFix.cpp)
endif()

message(STATUS "Main App Sources: ${ZENITH_SOURCES}")

# ==============================================================================
# ONNX Runtime Integration (Moved early for global visibility)
# ==============================================================================
option(ENABLE_ONNX "Enable ONNX Runtime" ON)
if(ENABLE_ONNX)
    if(UNIX AND NOT APPLE)
        include(FetchContent)
        FetchContent_Declare(
            onnxruntime
            URL https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-linux-x64-1.17.1.tgz
        )
        FetchContent_MakeAvailable(onnxruntime)
        
        # Use global compile definitions to ensure all modular sources see it
        add_compile_definitions(ZENITH_USE_ONNX_RUNTIME=1)
        message(STATUS "Zenith DAW: ONNX Runtime support ENABLED")
    else()
        message(STATUS "Zenith DAW: ONNX Runtime support disabled for this platform")
    endif()
endif()


# Create main executable
juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "Zenith Audio"
    VERSION ${PROJECT_VERSION}
    BUNDLE_ID com.zenithaudio.daw
)



# Generate JuceHeader.h for compatibility
juce_generate_juce_header(ZenithDAW)

target_sources(ZenithDAW PRIVATE ${ZENITH_SOURCES})

# Skia Integration (must be defined before subdirectories)
option(ZENITH_ENABLE_SKIA "Enable Skia Rendering" ON)

# Modularized Source Groups (Complaint #6 Fix)
add_subdirectory(apps/desktop/Source/plugins)
add_subdirectory(apps/desktop/Source/engine)
add_subdirectory(apps/desktop/Source/ui)
add_subdirectory(apps/desktop/Source/instruments)
add_subdirectory(apps/desktop/Source/network)
add_subdirectory(apps/desktop/Source/commands)
add_subdirectory(apps/desktop/Source/rendering)
add_subdirectory(apps/desktop/Source/dsp)
add_subdirectory(apps/desktop/Source/browser)
add_subdirectory(apps/desktop/Source/ai)
add_subdirectory(apps/desktop/Source/utils)
add_subdirectory(apps/desktop/Source/platform)
add_subdirectory(apps/scanner)


# Include directories (updated for new domain-based structure)
target_include_directories(ZenithDAW PUBLIC
    ${CMAKE_BINARY_DIR}/ZenithDAW_artefacts/JuceLibraryCode
    apps/desktop/Source
    apps/desktop/Source/engine
    apps/desktop/Source/ui
    apps/desktop/Source/ui/framework
    apps/desktop/Source/ui/design-system
    apps/desktop/Source/ui/arranger
    apps/desktop/Source/ui/mixer
    apps/desktop/Source/ui/piano-roll
    apps/desktop/Source/ui/browser
    apps/desktop/Source/ui/transport
    apps/desktop/Source/ui/common
    apps/desktop/Source/ui/widgets
    apps/desktop/Source/ui/session
    apps/desktop/Source/ui/instruments
    apps/desktop/Source/ui/dialogs
    apps/desktop/Source/ui/sample-editor
    apps/desktop/Source/commands
    apps/desktop/Source/instruments
    apps/desktop/Source/network
    apps/desktop/Source/dsp
    apps/desktop/Source/rendering
    apps/desktop/Source/ai
    apps/desktop/Source/ui/skia
    apps/desktop/Source/utils
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_BINARY_DIR}/JuceLibraryCode
    "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules"
)

target_link_libraries(ZenithDAW PRIVATE
    juce::juce_core
    juce::juce_cryptography
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
)

# Link ONNX Runtime if enabled
if(UNIX AND NOT APPLE AND ENABLE_ONNX)
    target_link_libraries(ZenithDAW PRIVATE ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so)
    target_include_directories(ZenithDAW PRIVATE ${onnxruntime_SOURCE_DIR}/include)
    
    # Copy shared library to output directory
    add_custom_command(TARGET ZenithDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.1.17.1"
        "$<TARGET_FILE_DIR:ZenithDAW>/libonnxruntime.so.1.17.1"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "libonnxruntime.so.1.17.1"
        "$<TARGET_FILE_DIR:ZenithDAW>/libonnxruntime.so.1"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "libonnxruntime.so.1.17.1"
        "$<TARGET_FILE_DIR:ZenithDAW>/libonnxruntime.so"
        COMMENT "Copying ONNX Runtime library to output directory"
    )
endif()


# Platform-specific libraries for Graphics Backends
if(WIN32)
    target_link_libraries(ZenithDAW PRIVATE d3d12 dxgi dcomp uuid)
elseif(APPLE)
    target_link_libraries(ZenithDAW PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
elseif(UNIX AND NOT APPLE)
    # For Vulkan on Linux
    find_package(Vulkan)
    if(Vulkan_FOUND)
        target_link_libraries(ZenithDAW PRIVATE Vulkan::Vulkan)
    else()
        message(STATUS "Vulkan not found - Skia Vulkan backend might fail at runtime")
        target_link_libraries(ZenithDAW PRIVATE vulkan) # Try explicit link
    endif()
endif()

# Selective RTTI disabling removed in favor of LinkerFix.cpp


# ==============================================================================
# Build Optimization Integration
# ==============================================================================
# Apply build optimizations (parallel compile, incremental link, etc.)
enable_zenith_build_optimizations(ZenithDAW)

# Precompiled Headers - dramatically reduces incremental build times
include(apps/desktop/cmake/PrecompiledHeaders.cmake)

# Compile definitions
target_compile_definitions(ZenithDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0

    JUCE_REPORT_APP_USAGE=0
)

# Selective RTTI disabling removed in favor of LinkerFix.cpp

# Removed commented-out MSVC calling convention code (2025-01-XX)
# If needed in future, see git history for previous implementation

# ==============================================================================
# MODERN COMPILER CONFIGURATION (Standard Flags)

# 6. Modern C++ Standards & Conformance
target_compile_features(ZenithDAW PRIVATE cxx_std_20)
set_target_properties(ZenithDAW PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    # Enforce specific runtime library to prevent mismatches
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# 7. Interprocedural Optimization (LTO)
if(ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if(result)
        set_target_properties(ZenithDAW PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "IPO/LTO enabled for ZenithDAW")
    else()
        message(WARNING "IPO is not supported: ${output}")
    endif()
endif()

# 8. Standard Compiler Flags & Hardening
# if(MSVC)
#     target_compile_options(ZenithDAW PRIVATE
#         /W4 /permissive- 
#         /wd4100 # Unused parameter
#         
#         # Conformance
#         /Zc:__cplusplus 
#         /Zc:inline      
#         /Zc:preprocessor 
#         /Zc:throwingNew 
#         
#         # Hardening
#         $<$<BOOL:${ENABLE_HARDENING}>:/GS>       
#         $<$<BOOL:${ENABLE_HARDENING}>:/Guard:CF> 
#         $<$<BOOL:${ENABLE_HARDENING}>:/SDL>      
#         $<$<BOOL:${ENABLE_SPECTRE}>:/Qspectre>   
#         
#         # Ruthless Warnings
#         /w14242 /w14254 /w14263 
#         /w14265 
#         /w14287 
#         /we4289 
#         /w14296 
#         /w14311 
#         /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
#     )
    
#     if(ENABLE_HARDENING)
#         target_link_options(ZenithDAW PRIVATE 
#             /DYNAMICBASE 
#             /NXCOMPAT    
#             /CETCOMPAT   
#         )
#     endif()

# else() # GCC / Clang
#     target_compile_options(ZenithDAW PRIVATE
#         -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-align -Wformat=2
#         -Wunused -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough
        
#         $<$<BOOL:${ENABLE_HARDENING}>:-fstack-protector-strong>
#         $<$<BOOL:${ENABLE_HARDENING}>:-fPIE>
#         $<$<BOOL:${ENABLE_HARDENING}>:-U_FORTIFY_SOURCE>
#         $<$<BOOL:${ENABLE_HARDENING}>:-D_FORTIFY_SOURCE=2>
#     )

#     if(ENABLE_HARDENING)
#         target_link_options(ZenithDAW PRIVATE 
#             -Wl,-z,relro 
#             -Wl,-z,now 
#             -pie
#         )
#     endif()
# endif()

# 9. Sanitizers
if(ENABLE_SANITIZERS)
    if(MSVC)
        target_compile_options(ZenithDAW PRIVATE /fsanitize=address)
    else()
        target_compile_options(ZenithDAW PRIVATE -fsanitize=address -fsanitize=undefined)
        target_link_options(ZenithDAW PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()
    message(STATUS "Sanitizers enabled (Address, UB)")
endif()

# Optional: Test executable
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    juce_add_gui_app(ZenithDAWTests
        PRODUCT_NAME "ZenithDAWTests"
    )
    
    # Generate JuceHeader.h for test target (fixes C1083: Cannot open include file: 'JuceHeader.h')
    juce_generate_juce_header(ZenithDAWTests)
    
    message(STATUS "Common Sources for Tests: ${ZENITH_COMMON_SOURCES}")
    target_sources(ZenithDAWTests PRIVATE ${ZENITH_TEST_SOURCES} ${ZENITH_COMMON_SOURCES})
    target_include_directories(ZenithDAWTests PRIVATE 
        apps/desktop/Source 
        apps/desktop/Source/engine
        apps/desktop/Source/ui
        apps/desktop/Source/ui/framework
        apps/desktop/Source/ui/design-system
        apps/desktop/Source/ui/common
        apps/desktop/Source/ui/instruments
        apps/desktop/Source/ui/browser
        apps/desktop/Source/ui/arranger
        apps/desktop/Source/ui/mixer
        apps/desktop/Source/ui/piano-roll
        apps/desktop/Source/ui/widgets
        apps/desktop/Source/ui/session
        apps/desktop/Source/ui/dialogs
        apps/desktop/Source/ui/sample-editor
        apps/desktop/Source/commands
        apps/desktop/Source/instruments
        apps/desktop/Source/network
        apps/desktop/Source/dsp
        apps/desktop/Source/rendering
        apps/desktop/Source/ui/skia
        apps/desktop/Source/utils
        apps/desktop/Source/ai
        apps/desktop/Source/tests
        apps/desktop/include
        ${CMAKE_PREFIX_PATH}/include
        ${CMAKE_BINARY_DIR}/ZenithDAWTests_artefacts/JuceLibraryCode
        "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules"
    )
    target_link_libraries(ZenithDAWTests PRIVATE
        juce::juce_core
        juce::juce_cryptography
        juce::juce_events
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors  # Required for TestUtils.h StubAudioPlugin
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    )
    

    if(UNIX AND NOT APPLE AND ENABLE_ONNX)
        target_link_libraries(ZenithDAWTests PRIVATE ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so)
        target_include_directories(ZenithDAWTests PRIVATE ${onnxruntime_SOURCE_DIR}/include)
    endif()

    target_compile_definitions(ZenithDAWTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_compile_features(ZenithDAWTests PRIVATE cxx_std_20)

    if(ZENITH_ENABLE_SKIA)
        # The SkiaManualIntegration.cmake script handles target_link_libraries
        # and target_include_directories for BOTH ZenithDAW and ZenithDAWTests
        # as long as they are defined before it is included.
        include(apps/desktop/cmake/SkiaManualIntegration.cmake)
        # target_compile_definitions(ZenithDAWTests PRIVATE ZENITH_USE_SKIA=1) - now handled by script
    endif()
endif()

# Final Target Configuration (Skia, etc.)
if(ZENITH_ENABLE_SKIA)
    # The SkiaManualIntegration.cmake script handles target_link_libraries
    # and target_include_directories for BOTH ZenithDAW and ZenithDAWTests.
    # We include it here to ensure both targets are defined if they exist.
    include(apps/desktop/cmake/SkiaManualIntegration.cmake)
endif()

# Installation
install(TARGETS ZenithDAW
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)




get_target_property(OPTS ZenithDAW COMPILE_OPTIONS)
message(STATUS "ZenithDAW COMPILE_OPTIONS: ${OPTS}")
get_target_property(INT_OPTS ZenithDAW INTERFACE_COMPILE_OPTIONS)
message(STATUS "ZenithDAW INTERFACE_COMPILE_OPTIONS: ${INT_OPTS}")
get_target_property(LIBS ZenithDAW LINK_LIBRARIES)
foreach(LIB ${LIBS})
    if(TARGET ${LIB})
        get_target_property(LIB_OPTS ${LIB} INTERFACE_COMPILE_OPTIONS)
        message(STATUS "${LIB} INTERFACE_COMPILE_OPTIONS: ${LIB_OPTS}")
    endif()

endforeach()

# Copy Resources (Fonts) to Output Directory
add_custom_command(TARGET ZenithDAW POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/apps/desktop/Resources/fonts
    $<TARGET_FILE_DIR:ZenithDAW>/Resources/fonts
    COMMENT "Copying Fonts to Output Directory"
)
