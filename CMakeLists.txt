cmake_minimum_required(VERSION 3.20)
project(ZenithDAW VERSION 0.1.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always enable Skia (Phase 2 decision)
add_definitions(-DZENITH_USE_SKIA=1)

# JUCE Setup
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9
)
FetchContent_MakeAvailable(JUCE)

# Find Skia (from vcpkg)
find_package(skia CONFIG REQUIRED)

# Source files
set(ZENITH_SOURCES
    # Main
    zenith-core/Source/Main.cpp
    zenith-core/Source/MainWindow.cpp
    zenith-core/Source/Engine.cpp
    zenith-core/Source/ProjectState.cpp
    zenith-core/Source/commands/CommandAPI.cpp
    
    # UI Core
    zenith-core/Source/ArrangerComponent.cpp
    zenith-core/Source/ArrangementComponent.cpp
    zenith-core/Source/ui/ArrangerView.cpp
    zenith-core/Source/MixerComponent.cpp
    zenith-core/Source/PianoRollComponent.cpp
    zenith-core/Source/PianoRollEditor.cpp
    zenith-core/Source/AutomationLaneComponent.cpp
    zenith-core/Source/MarkerLaneComponent.cpp
    zenith-core/Source/TempoLaneComponent.cpp
    zenith-core/Source/PluginBrowserComponent.cpp
    
    # Engine
    zenith-core/Source/engine/Track.cpp
    zenith-core/Source/engine/Clip.cpp
    zenith-core/Source/engine/MixerChannel.cpp
    zenith-core/Source/engine/AudioFilePool.cpp
    zenith-core/Source/engine/PluginHost.cpp
    zenith-core/Source/engine/AuxBus.cpp
    zenith-core/Source/engine/ExportEngine.cpp
    zenith-core/Source/engine/ZenithLogger.cpp
    zenith-core/Source/engine/ExportEngineImpl.cpp
    zenith-core/Source/engine/TrackPluginState.cpp
    zenith-core/Source/engine/PluginHostAsync.cpp
    
    # Instruments
    zenith-core/Source/instruments/Instrument.cpp
    zenith-core/Source/instruments/InstrumentRegistry.cpp
    zenith-core/Source/instruments/RegisterBuiltInInstruments.cpp
    zenith-core/Source/instruments/PresetGenerator.cpp
    zenith-core/Source/instruments/ZenithPolySynth.cpp
    zenith-core/Source/instruments/ZenithPolySynthEditor.cpp
    zenith-core/Source/instruments/ZenithPresetManager.cpp
    zenith-core/Source/instruments/ZenithSampler.cpp
    zenith-core/Source/instruments/ZenithSamplerEditor.cpp
    
    # UI Components (Duplicates removed - using Source locations)
    zenith-core/Source/ui/ClipComponent.cpp
    zenith-core/Source/ui/MixerView.cpp
    zenith-core/Source/ui/MixerChannelComponent.cpp
    zenith-core/Source/ui/TrackHeaderComponent.cpp
    zenith-core/Source/ui/TimelineRuler.cpp
    zenith-core/Source/ui/PluginEditorWindow.cpp
    zenith-core/Source/ui/PresetBrowserComponent.cpp
    zenith-core/Source/ui/InstrumentBrowserPanel.cpp
    zenith-core/Source/ui/WingmanPanel.cpp
    zenith-core/Source/ui/AudioFeedback.cpp
    zenith-core/Source/ui/MainLayoutComponent.cpp
    zenith-core/Source/ui/ZenithLookAndFeel.cpp
    zenith-core/Source/ui/ZenithTheme.cpp
    
    # Skia UI
    zenith-core/Source/ui/skia/SkiaComponent.cpp
    zenith-core/Source/ui/skia/SkiaButton.cpp
    zenith-core/Source/ui/skia/SkiaKnob.cpp
    zenith-core/Source/ui/skia/SkiaSlider.cpp
    zenith-core/Source/ui/skia/SkiaMainWindowIntegration.cpp
    zenith-core/Source/ui/skia/TransportBar.cpp
    zenith-core/Source/ui/skia/BottomBar.cpp
    zenith-core/Source/ui/skia/BrowserPanel.cpp
    zenith-core/Source/ui/skia/RightSidePanel.cpp
    zenith-core/Source/ui/skia/ZenithPolySynthUI.cpp
    zenith-core/Source/ui/skia/ZenithDesignSystem.cpp
    zenith-core/Source/ui/skia/views/PianoKeyboardViewSkia.cpp
    zenith-core/Source/ui/skia/views/SessionViewComponent.cpp
    
    # Network/AI
    zenith-core/Source/network/AIBridgeClient.cpp
    zenith-core/Source/network/GrokAPIClient.cpp
    zenith-core/Source/network/GrokDAWController.cpp
    zenith-core/Source/network/SecureKeyStore.cpp
    zenith-core/Source/network/AudioAnalysisService.cpp
    
    # DSP
    zenith-core/Source/dsp/DSPStemSeparator.cpp
    zenith-core/Source/dsp/DSPVoiceChanger.cpp
    zenith-core/Source/dsp/ONNXStemSeparator.cpp
    zenith-core/Source/dsp/ONNXStemSeparatorImpl.cpp
    
    # Commands (SessionGraph only - CommandAPI is already listed above)
    zenith-core/Source/commands/SessionGraph.cpp
    
    # Export
    zenith-core/Source/export/NFTMintingService.cpp
    
    # Rendering
    zenith-core/Source/rendering/SkiaContextManager.cpp
    
    # Synchronizers
    zenith-core/Source/ClipSynchronizer.cpp
    zenith-core/Source/TrackStateSynchronizer.cpp
    zenith-core/Source/TrackAutomationSynchronizer.cpp
    zenith-core/Source/TempoMapSynchronizer.cpp
    zenith-core/Source/TempoMap.cpp
)

# Test sources (optional, can be disabled)
set(ZENITH_TEST_SOURCES
    zenith-core/Source/tests/AudioEngineTests.cpp
    zenith-core/Source/tests/ProjectStateTests.cpp
)

# Create main executable
juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "Zenith Audio"
    VERSION ${PROJECT_VERSION}
    BUNDLE_ID com.zenithaudio.daw
)

target_sources(ZenithDAW PRIVATE ${ZENITH_SOURCES})

# Include directories
target_include_directories(ZenithDAW PRIVATE
    zenith-core/include
    zenith-core/Source
    ${CMAKE_PREFIX_PATH}/include
)

# Link JUCE modules
target_link_libraries(ZenithDAW PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
)

# Link Skia
target_link_libraries(ZenithDAW PRIVATE
    skia::skia
)

# Compile definitions
target_compile_definitions(ZenithDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    ZENITH_USE_SKIA=1
)

# Copy DLLs to output directory (Windows)
if(WIN32)
    add_custom_command(TARGET ZenithDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:skia::skia>"
        "$<TARGET_FILE_DIR:ZenithDAW>"
    )
endif()

# Optional: Test executable
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    juce_add_console_app(ZenithDAWTests
        PRODUCT_NAME "Zenith DAW Tests"
    )
    
    target_sources(ZenithDAWTests PRIVATE ${ZENITH_TEST_SOURCES})
    target_include_directories(ZenithDAWTests PRIVATE zenith-core/Source zenith-core/include)
    target_link_libraries(ZenithDAWTests PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_basics
    )
endif()

# Installation
install(TARGETS ZenithDAW
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
