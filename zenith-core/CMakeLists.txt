cmake_minimum_required(VERSION 3.22)

# Project definition
project(ZenithDAW VERSION 0.1.0 LANGUAGES C CXX)

# C++20 standard required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection
if(WIN32)
    set(CMAKE_GENERATOR_PLATFORM x64 CACHE STRING "Generator platform" FORCE)
endif()

# ============================================================================
# Fetch JUCE 8.0.9 (pinned to exact commit for reproducibility)
# ============================================================================
include(FetchContent)

message(STATUS "Fetching JUCE 8.0.9 (commit: f72bad6)...")

FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG f72bad64d29715216226685810c5196bd0d79d77  # JUCE 8.0.9
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce)

message(STATUS "JUCE 8.0.9 fetched successfully!")

# ============================================================================
# Build Configuration
# ============================================================================

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Platform-specific flags
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    # Build universal binaries for Apple Silicon + Intel
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()

# Windows-specific build options
if(WIN32)
    # Warnings as errors (OFF by default for local dev, ON in CI)
    option(ZENITH_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

    # Link-Time Code Generation (OFF by default for faster link times)
    # Enable for final release builds with: cmake -DZENITH_LTCG=ON
    option(ZENITH_LTCG "Enable whole program optimization (/GL + /LTCG)" OFF)

    # MMCSS Audio Priority (ON by default for pro audio performance)
    option(ZENITH_ENABLE_MMCSS "Enable Windows MMCSS audio thread priority boost" ON)

    # Phase 1 Audio Engine (OFF by default until wiring complete)
    option(ZENITH_ENABLE_PHASE1_AUDIO "Enable Phase 1 audio processing (Track/Clip mixer)" OFF)

    # Crash Capture (OFF by default for Release builds)
    option(ZENITH_ENABLE_CRASH_CAPTURE "Enable crash reporting and minidump capture" OFF)

    message(STATUS "Windows build - LTCG: ${ZENITH_LTCG}, Warnings as errors: ${ZENITH_WARNINGS_AS_ERRORS}, MMCSS: ${ZENITH_ENABLE_MMCSS}")
    message(STATUS "Feature flags - Phase1 Audio: ${ZENITH_ENABLE_PHASE1_AUDIO}, Crash Capture: ${ZENITH_ENABLE_CRASH_CAPTURE}")
endif()

# ============================================================================
# Zenith DAW Application
# ============================================================================

juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "ZenithAudio"
    BUNDLE_ID "com.zenithaudio.zenith-daw"
    VERSION "0.1.0"

    # Icon file (will create later)
    # ICON_BIG "resources/icon.png"

    # Microphone/camera permissions (macOS)
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Zenith DAW needs access to your audio input devices for recording."
)

# ============================================================================
# Source Files
# ============================================================================

target_sources(ZenithDAW PRIVATE
    # Core application
    src/Main.cpp
    src/MainWindow.cpp
    src/Engine.cpp
    src/ProjectState.cpp
    src/AutoSaveManager.cpp
    src/Mixer.cpp
    src/AudioTrack.cpp

    # UI Components (custom JUCE-based UI)
    Source/ui/ZenithLookAndFeel.cpp
    Source/ui/MainComponent.cpp
    Source/ui/TopBar.cpp
    Source/ui/Sidebar.cpp
    Source/ui/TrackView.cpp
    Source/ui/TransportBar.cpp

    # W6: Performance monitoring overlay (DEBUG-only)
    $<$<CONFIG:Debug>:Source/ui/StatsOverlay.cpp>

    # W10.3: Phase 1 audio engine - Track/Clip mixer (gated by ZENITH_ENABLE_PHASE1_AUDIO)
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:Source/engine/Clip.h>
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:Source/engine/Track.h>
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:Source/engine/Track.cpp>
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:Source/engine/Mixer.h>
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:Source/engine/Mixer.cpp>

    # W11.0: Plugin chain scaffold - internal FX nodes (gated by ZENITH_ENABLE_PHASE1_AUDIO)
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:include/engine/IDspNode.h>
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:Source/engine/nodes/GainPanNode.h>
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:Source/engine/nodes/GainPanNode.cpp>

    # Windows-specific sources
    $<$<PLATFORM_ID:Windows>:Source/win/WinRtAudioPriority.cpp>
    $<$<PLATFORM_ID:Windows>:Source/ui/AudioSettingsWindows.cpp>

    # Windows resources (manifest, version info)
    $<$<PLATFORM_ID:Windows>:resources/windows/app.rc>
)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ============================================================================
# JUCE Modules
# ============================================================================

target_link_libraries(ZenithDAW PRIVATE
    # Audio essentials
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils

    # Core functionality
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events

    # GUI
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra

    # Public link for GUI app config
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ============================================================================
# Generate JuceHeader.h
# ============================================================================

juce_generate_juce_header(ZenithDAW)

# ============================================================================
# Preprocessor Definitions
# ============================================================================

target_compile_definitions(ZenithDAW PRIVATE
    # JUCE app config
    JUCE_APP_VERSION="0.1.0"
    JUCE_APP_VERSION_HEX=0x000100

    # Application name
    JucePlugin_Name="Zenith DAW"

    # Audio settings
    JUCE_USE_OGGVORBIS=1
    JUCE_USE_FLAC=1
    JUCE_USE_MP3AUDIOFORMAT=1

    # Disable JUCE splash screen (requires Pro license for commercial use)
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # VST3 hosting (will enable when we add plugin hosting)
    # JUCE_PLUGINHOST_VST3=1
    # JUCE_PLUGINHOST_AU=1

    # Don't auto-open plugin windows
    JUCE_PLUGINHOST_VST3_PLUGIN_EDITOR_SHOWS_BY_DEFAULT=0

    # Strict warnings
    JUCE_STRICT_REFCOUNTEDPOINTER=1

    # Disable analytics
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0

    # Windows MMCSS
    $<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${ZENITH_ENABLE_MMCSS}>>:ZENITH_ENABLE_MMCSS=1>

    # Phase 1 Audio Engine
    $<$<BOOL:${ZENITH_ENABLE_PHASE1_AUDIO}>:ZENITH_ENABLE_PHASE1_AUDIO=1>

    # Debug builds
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>
    $<$<CONFIG:Debug>:ZENITH_DEBUG_PERF_HUD=1>  # W6.1: Enable performance HUD in Debug

    # Release builds
    $<$<CONFIG:Release>:NDEBUG=1>
)

# ============================================================================
# Compiler Warnings & Optimization
# ============================================================================

# Helper function to apply warnings to our targets only (not third-party)
function(zenith_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4                    # Warning level 4 (comprehensive)
            /permissive-           # Standards conformance mode
            /w14242                # Implicit int to narrower type conversion
            /w14254                # Different type qualifiers
            /w14263                # Override but doesn't override any base class member
            /w14265                # Class has virtual functions but no virtual destructor
            /w14287                # Unsigned/negative constant mismatch
            /we4289                # Loop control variable used outside loop (error)
            /w14296                # Expression always true/false
            /w14311                # Pointer truncation
            /w14545 /w14546 /w14547 /w14549 /w14555  # Operator precedence issues
            /w14619                # Unknown warning number
            /w14640                # Thread-unsafe static member initialization
            /w14826                # Sign extension issues
            /w14905                # Wide string literal cast issues
            /w14906                # String literal cast issues
            /w14928                # Illegal copy-initialization
        )

        # Warnings as errors (opt-in)
        if(ZENITH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
            message(STATUS "Warnings as errors ENABLED for ${target}")
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter  # JUCE callbacks often have unused params
        )
    endif()
endfunction()

# Apply warnings to our app target
zenith_apply_warnings(ZenithDAW)

# Windows-specific optimization and PDB generation
if(MSVC)
    # Generate PDBs in all configurations for crash analysis
    target_compile_options(ZenithDAW PRIVATE
        /Zi                        # Generate complete debugging information
    )

    # Release-specific optimizations
    target_compile_options(ZenithDAW PRIVATE
        $<$<CONFIG:Release>:/O2>   # Maximize speed
        $<$<CONFIG:Release>:/Oi>   # Enable intrinsic functions
        $<$<CONFIG:Release>:/Gy>   # Enable function-level linking (for better dead code removal)
        $<$<CONFIG:Release>:/Gw>   # Optimize global data (allows linker to remove unused globals)
    )

    # Optional Link-Time Code Generation (whole program optimization)
    if(ZENITH_LTCG)
        target_compile_options(ZenithDAW PRIVATE
            $<$<CONFIG:Release>:/GL>   # Whole program optimization
        )
        target_link_options(ZenithDAW PRIVATE
            $<$<CONFIG:Release>:/LTCG> # Link-time code generation
        )
        message(STATUS "LTCG enabled for Release builds (expect slower link times)")
    endif()

    # Linker flags for all configurations
    target_link_options(ZenithDAW PRIVATE
        /DEBUG:FULL                # Generate complete PDB for all configs
        /OPT:REF                   # Eliminate unreferenced functions/data
        /OPT:ICF                   # Identical COMDAT folding (merge duplicate functions)
    )

    # Release-specific linker flags
    target_link_options(ZenithDAW PRIVATE
        $<$<CONFIG:Release>:/INCREMENTAL:NO>  # Disable incremental linking for Release
    )

    message(STATUS "MSVC flags applied:")
    message(STATUS "  - PDB generation: /Zi + /DEBUG:FULL")
    message(STATUS "  - Release optimizations: /O2 /Oi /Gy /Gw")
    message(STATUS "  - Linker optimizations: /OPT:REF /OPT:ICF")
    if(ZENITH_LTCG)
        message(STATUS "  - LTCG: /GL + /LTCG (ENABLED)")
    else()
        message(STATUS "  - LTCG: OFF (enable with -DZENITH_LTCG=ON)")
    endif()
endif()

# ============================================================================
# Binary Data (Resources)
# ============================================================================

# Will add this when we have resources
# juce_add_binary_data(ZenithResources SOURCES
#     resources/fonts/Inter-Regular.ttf
#     resources/fonts/Inter-Bold.ttf
# )
#
# target_link_libraries(ZenithDAW PRIVATE ZenithResources)

# ============================================================================
# Phase 1 Audio Engine Tests (optional, gated by ZENITH_ENABLE_PHASE1_AUDIO)
# ============================================================================

if(ZENITH_ENABLE_PHASE1_AUDIO)
    message(STATUS "Building Phase 1 audio engine tests...")

    # Create console test app
    juce_add_console_app(zenith-core-phase1-tests
        PRODUCT_NAME "Zenith Phase 1 Tests"
    )

    target_sources(zenith-core-phase1-tests PRIVATE
        tests/engine/Phase1AudioTests.cpp

        # Include same engine sources as main app
        Source/engine/Clip.h
        Source/engine/Track.h
        Source/engine/Track.cpp
        Source/engine/Mixer.h
        Source/engine/Mixer.cpp

        # W11.0: FX nodes
        include/engine/IDspNode.h
        Source/engine/nodes/GainPanNode.h
        Source/engine/nodes/GainPanNode.cpp
    )

    target_include_directories(zenith-core-phase1-tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(zenith-core-phase1-tests PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
    )

    target_compile_definitions(zenith-core-phase1-tests PRIVATE
        JUCE_APP_VERSION="0.1.0"
        ZENITH_ENABLE_PHASE1_AUDIO=1
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Debug>:_DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG=1>
    )

    juce_generate_juce_header(zenith-core-phase1-tests)

    # Apply same warnings as main app
    zenith_apply_warnings(zenith-core-phase1-tests)

    message(STATUS "Phase 1 tests target: zenith-core-phase1-tests")
else()
    message(STATUS "Phase 1 tests SKIPPED (ZENITH_ENABLE_PHASE1_AUDIO=OFF)")
endif()

# ============================================================================
# Post-Build Messages
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Zenith DAW Configuration")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "JUCE Version: 8.0.9")

if(APPLE)
    message(STATUS "macOS Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    message(STATUS "macOS Architectures: ${CMAKE_OSX_ARCHITECTURES}")
endif()

message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# Installation (optional)
# ============================================================================

install(TARGETS ZenithDAW
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
