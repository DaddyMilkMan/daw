cmake_minimum_required(VERSION 3.22)

project(ZenithDAW VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE subdirectory (clone from https://github.com/juce-framework/JUCE)
# git submodule add -b master https://github.com/juce-framework/JUCE.git JUCE
add_subdirectory(JUCE)

# Optional: Build JUCE extras (AudioPluginHost for reference)
set(JUCE_BUILD_EXTRAS ON)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)

# Create the DAW executable
juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "YourCompany"
    BUNDLE_ID "com.yourcompany.zenithdaw"
    ICON_BIG "${CMAKE_SOURCE_DIR}/Resources/icon.png"

    # Microphone/camera permissions (macOS)
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "This app requires microphone access for audio recording."
)

# Add source files
target_sources(ZenithDAW PRIVATE
    Source/Main.cpp
    Source/Engine.h
    Source/Engine.cpp
    Source/ProjectState.h
    Source/ProjectState.cpp
    Source/MainWindow.h
    Source/MainWindow.cpp
)

# Link JUCE modules
target_link_libraries(ZenithDAW PRIVATE
    # Audio core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils

    # DSP
    juce::juce_dsp

    # GUI
    juce::juce_gui_basics
    juce::juce_gui_extra

    # Data structures (ValueTree, UndoManager)
    juce::juce_data_structures

    # Core utilities
    juce::juce_core
    juce::juce_events

    # Recommended flags
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Compiler definitions
target_compile_definitions(ZenithDAW PRIVATE
    # JUCE settings
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ZenithDAW,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:ZenithDAW,JUCE_VERSION>"

    # Audio thread safety
    JUCE_CHECK_MEMORY_LEAKS=1

    # VST3 SDK path (if you have it)
    # JUCE_VST3_CAN_REPLACE_VST2=0
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(ZenithDAW PRIVATE
        JUCE_COREAUDIO=1
        JUCE_COREAUDIO_ENABLED=1
    )
elseif(WIN32)
    target_compile_definitions(ZenithDAW PRIVATE
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=1
        JUCE_ASIO=1
    )
elseif(UNIX)
    target_compile_definitions(ZenithDAW PRIVATE
        JUCE_ALSA=1
        JUCE_JACK=1
    )
endif()

# Optional: Add VST3 SDK for plugin hosting
# set(JUCE_VST3_SDK_PATH "${CMAKE_SOURCE_DIR}/SDKs/VST3_SDK" CACHE PATH "Path to VST3 SDK")
# if(EXISTS "${JUCE_VST3_SDK_PATH}")
#     target_compile_definitions(ZenithDAW PRIVATE
#         JUCE_PLUGINHOST_VST3=1
#     )
# endif()

# Optional: Enable OpenGL only if profiling shows it's needed
# target_link_libraries(ZenithDAW PRIVATE juce::juce_opengl)
# target_compile_definitions(ZenithDAW PRIVATE JUCE_OPENGL=1)
