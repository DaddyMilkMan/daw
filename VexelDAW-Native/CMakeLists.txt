cmake_minimum_required(VERSION 3.22)

project(VEXEL_DAW VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE (assume it's in a subdirectory or via FetchContent)
# Option 1: If JUCE is already cloned locally
# add_subdirectory(JUCE)

# Option 2: Fetch JUCE from GitHub
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

# Create the GUI application
juce_add_gui_app(VexelDAW
    PRODUCT_NAME "Vexel DAW"
    COMPANY_NAME "DaddyMilkMan"
    COMPANY_WEBSITE "https://github.com/DaddyMilkMan/daw"
    COMPANY_EMAIL "support@vexeldaw.com"
    BUNDLE_ID com.daddymilkman.vexeldaw

    # Icon files
    # ICON_BIG Resources/icon.png

    # Audio settings
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    WANTS_MIDI_INPUT TRUE

    # Plugin formats to build (none for standalone DAW)
    FORMATS Standalone
)

# Source files
target_sources(VexelDAW PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h

    # Audio engine
    Source/Audio/AudioEngine.cpp
    Source/Audio/AudioEngine.h
    Source/Audio/Track.cpp
    Source/Audio/Track.h
    Source/Audio/Clip.cpp
    Source/Audio/Clip.h
    Source/Audio/MixerChannel.cpp
    Source/Audio/MixerChannel.h
    Source/Audio/PluginHost.cpp
    Source/Audio/PluginHost.h

    # GUI components
    Source/GUI/Transport/TransportComponent.cpp
    Source/GUI/Transport/TransportComponent.h
    Source/GUI/Mixer/MixerComponent.cpp
    Source/GUI/Mixer/MixerComponent.h
    Source/GUI/Arrangement/ArrangementComponent.cpp
    Source/GUI/Arrangement/ArrangementComponent.h
    Source/GUI/Browser/BrowserComponent.cpp
    Source/GUI/Browser/BrowserComponent.h

    # State management
    Source/State/ProjectState.cpp
    Source/State/ProjectState.h

    # Utilities
    Source/Utilities/ProjectManager.cpp
    Source/Utilities/ProjectManager.h
)

# Preprocessor definitions
target_compile_definitions(VexelDAW PRIVATE
    # JUCE modules
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0

    # Application info
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:VexelDAW,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:VexelDAW,JUCE_VERSION>"

    # Plugin hosting
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_LADSPA=1

    # Debug settings
    $<$<CONFIG:Debug>:JUCE_CHECK_MEMORY_LEAKS=1>
    $<$<CONFIG:Debug>:JUCE_STRICT_REFCOUNTEDPOINTER=1>
)

# Link JUCE modules
target_link_libraries(VexelDAW
    PRIVATE
        # Audio modules
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils

        # Core modules
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events

        # GUI modules
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(VexelDAW PRIVATE
        JUCE_PLUGINHOST_AU=1
    )
endif()

if(WIN32)
    target_compile_definitions(VexelDAW PRIVATE
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=1
    )
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(VexelDAW PRIVATE
        JUCE_ALSA=1
        JUCE_JACK=1
    )
endif()

# Copy resources to build directory
# add_custom_command(TARGET VexelDAW POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_SOURCE_DIR}/Resources
#     $<TARGET_FILE_DIR:VexelDAW>/Resources
# )

# Enable testing
enable_testing()
add_subdirectory(Tests EXCLUDE_FROM_ALL)
