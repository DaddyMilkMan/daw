cmake_minimum_required(VERSION 3.22)

project(ZenithDAW-Qt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(SOURCES
    main.cpp
    bridge/AudioEngineInterface.cpp
    bridge/AudioEngineInterface.h
)

# QML files
set(QML_FILES
    qml/main.qml
    qml/components/TopBar.qml
    qml/components/TrackView.qml
    qml/components/TrackItem.qml
    qml/components/MixerPanel.qml
    qml/components/MixerChannel.qml
    qml/components/WaveformView.qml
    qml/components/TransportBar.qml
    qml/components/SidebarPanel.qml
    qml/components/PluginRack.qml
    qml/components/Timeline.qml
    qml/styles/AppTheme.qml
)

# Create executable with QML module
qt_add_executable(ZenithDAW
    ${SOURCES}
)

# Add QML module to the executable
qt_add_qml_module(ZenithDAW
    URI ZenithDAW
    VERSION 1.0
    QML_FILES ${QML_FILES}
    NO_RESOURCE_TARGET_PATH
)

# Link libraries
target_link_libraries(ZenithDAW PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    Qt6::Multimedia
    JuceAudioEngine  # JUCE audio engine library
)

# Include directories
target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/bridge
    ${CMAKE_SOURCE_DIR}/src/juce-engine/Source
)

# Compiler warnings
if(MSVC)
    target_compile_options(ZenithDAW PRIVATE /W4)
else()
    target_compile_options(ZenithDAW PRIVATE -Wall -Wextra -Wpedantic)
endif()

# macOS bundle configuration
if(APPLE)
    set_target_properties(ZenithDAW PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.zenith.daw
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    )
endif()

# Windows application settings
if(WIN32)
    set_target_properties(ZenithDAW PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
