cmake_minimum_required(VERSION 3.22)

project(ZenithDAW VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build universal binary")
endif()

# Find Qt6
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Quick
    Qml
    Widgets
    Multimedia
)

# Qt setup
qt_standard_project_setup(REQUIRES 6.5)

# Add JUCE subdirectory
# Uncomment when JUCE is added as a submodule:
# add_subdirectory(JUCE)

# Or use installed JUCE:
# find_package(JUCE CONFIG REQUIRED)

# JUCE audio engine library
add_subdirectory(src/juce-engine)

# Qt/QML application
add_subdirectory(src/qt-qml)

# Installation rules
install(TARGETS ZenithDAW
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Platform-specific packaging
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "Zenith DAW")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.zenith.daw")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_ICON_FILE "icon.icns")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 Zenith")
elseif(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# CPack configuration for installers
include(CPack)
set(CPACK_PACKAGE_NAME "ZenithDAW")
set(CPACK_PACKAGE_VENDOR "Zenith")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional Digital Audio Workstation")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Zenith DAW")
    set(CPACK_NSIS_PACKAGE_NAME "ZenithDAW")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6, libqt6quick6")
endif()
