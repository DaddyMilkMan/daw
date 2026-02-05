# =============================================================================
# ZENITH MODULE DEFINITIONS
# =============================================================================

# Include individual module definitions
include(ZenithEngineCore)
include(ZenithUIUnified)

# Additional module includes would go here
# include(ZenithDSP)
# include(ZenithAudioEngine)
# include(ZenithUIFramework)
# include(ZenithNetwork)
# include(ZenithAI)

# Main application sources
set(ZENITH_APP_SOURCES
    apps/desktop/Source/Main.cpp
    apps/desktop/Source/ZenithApplication.cpp
    apps/desktop/Source/Settings.cpp
)

# Application components
add_executable(ZenithDAW
    ${ZENITH_APP_SOURCES}
)

# Application target properties
target_include_directories(ZenithDAW
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/desktop/Source
        ${CMAKE_SOURCE_DIR}/modules
)

target_link_libraries(ZenithDAW
    zenith_engine_core
    zenith_ui_unified
    zenith_core
    zenith_audio_utils
    juce_gui_basics
    juce_audio_devices
    juce_audio_formats
    juce_audio_processors
    juce_data_structures
    juce_events
    juce_graphics
    juce_gui_basics
)

# Platform-specific configurations
if(APPLE)
    target_link_libraries(ZenithDAW "-framework Cocoa -framework IOKit -framework CoreAudio -framework CoreMIDI")
elseif(UNIX)
    target_link_libraries(ZenithDAW "-lpthread -lasound -ldl")
elseif(WIN32)
    target_link_libraries(ZenithDAW "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib")
endif()

# Installation
install(TARGETS ZenithDAW
    RUNTIME DESTINATION bin
)