cmake_minimum_required(VERSION 3.22)

# Project definition
project(ZenithDAW VERSION 0.1.0 LANGUAGES C CXX)

# C++20 standard required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Fetch JUCE 8.0.9 using FetchContent
# ============================================================================
include(FetchContent)

message(STATUS "Fetching JUCE 8.0.9...")

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(JUCE)

message(STATUS "JUCE fetched successfully!")

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
    src/Main.cpp
    src/MainWindow.cpp
    src/Engine.cpp
    src/ProjectState.cpp
    src/ProjectManager.cpp
    src/Track.cpp
)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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

    # Debug builds
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>

    # Release builds
    $<$<CONFIG:Release>:NDEBUG=1>
)

# ============================================================================
# Compiler Warnings
# ============================================================================

if(MSVC)
    target_compile_options(ZenithDAW PRIVATE /W4)
else()
    target_compile_options(ZenithDAW PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter  # JUCE callbacks often have unused params
    )
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
