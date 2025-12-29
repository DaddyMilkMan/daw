# =============================================================================
# ZENITH DAW - Main Build Script
# =============================================================================

cmake_minimum_required(VERSION 3.25)

project(ZenithDAW 
    VERSION 0.1.0
    DESCRIPTION "Zenith DAW - Professional Digital Audio Workstation"
    LANGUAGES CXX C
)

# 1. Platform Detection
include(cmake/PlatformDetection.cmake OPTIONAL)

# 2. Build Configuration & Flags
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, defaulting to Release (Skia requirement)")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Compiler Flags & Options (IPO, Sanitizers, Hardening)
include(cmake/CompilerFlags.cmake)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /bigobj" CACHE STRING "Platform-specific flags for the C++ compiler." FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /bigobj" CACHE STRING "Platform-specific flags for the C compiler." FORCE)
endif()

# 3. Dependencies
# JUCE Setup
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.0)
    FetchContent_MakeAvailable(JUCE)
endif()

set(ZENITH_JUCE_MODULES
    juce::juce_core juce::juce_cryptography juce::juce_events juce::juce_graphics
    juce::juce_data_structures juce::juce_gui_basics juce::juce_gui_extra
    juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats
    juce::juce_audio_processors juce::juce_audio_utils juce::juce_dsp juce::juce_opengl
)

# ONNX Runtime Integration
include(cmake/ONNXIntegration.cmake)

# 4. Source Files
include(cmake/SourceFiles.cmake)

# 5. Target Definition
juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "Zenith Audio"
    VERSION ${PROJECT_VERSION}
    BUNDLE_ID com.zenithaudio.daw
)

juce_generate_juce_header(ZenithDAW)

# Split sources: Main application vs Library components (to avoid multiple mains in tests)
target_sources(ZenithDAW PRIVATE ${ZENITH_APP_SOURCES} ${ZENITH_ALL_SOURCES})

# 6. Include Directories
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

# 7. Library Linking
target_link_libraries(ZenithDAW PRIVATE ${ZENITH_JUCE_MODULES})

if(ENABLE_ONNX AND ONNX_RUNTIME_LIBRARIES)
    target_link_libraries(ZenithDAW PRIVATE ${ONNX_RUNTIME_LIBRARIES})
    target_include_directories(ZenithDAW PRIVATE ${ONNX_RUNTIME_INCLUDE_DIRS})
endif()

# 8. Skia Integration
include(cmake/SkiaIntegration.cmake)

# 9. Build Optimizations & PCH
include(apps/desktop/cmake/BuildOptimizations.cmake OPTIONAL)
if(COMMAND enable_zenith_build_optimizations)
    enable_zenith_build_optimizations(ZenithDAW)
endif()

include(apps/desktop/cmake/PrecompiledHeaders.cmake OPTIONAL)

# 10. Optional Tests
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    # Use console app for leaner tests as suggested by bot
    juce_add_console_app(ZenithDAWTests
        PRODUCT_NAME "ZenithDAWTests"
    )
    
    juce_generate_juce_header(ZenithDAWTests)
    
    # Critical Fix: Only link library sources, not the app's Main.cpp
    # Exclude ZENITH_APP_SOURCES which contains Main.cpp
    target_sources(ZenithDAWTests PRIVATE 
        ${ZENITH_TEST_SOURCES} 
        ${ZENITH_ENGINE_SOURCES}
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
    
    target_include_directories(ZenithDAWTests PRIVATE 
        ${CMAKE_BINARY_DIR}/ZenithDAWTests_artefacts/JuceLibraryCode
        apps/desktop/Source
        apps/desktop/Source/engine
        apps/desktop/Source/ui
        apps/desktop/Source/ui/framework
        apps/desktop/Source/ui/design-system
        apps/desktop/Source/ui/arranger
        apps/desktop/Source/ui/mixer
        apps/desktop/Source/ui/piano-roll
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
    
    target_link_libraries(ZenithDAWTests PRIVATE ${ZENITH_JUCE_MODULES})
    
    if(ENABLE_ONNX AND ONNX_RUNTIME_LIBRARIES)
        target_link_libraries(ZenithDAWTests PRIVATE ${ONNX_RUNTIME_LIBRARIES})
    endif()

    if(ZENITH_ENABLE_SKIA)
        # Apply Skia to tests as well if enabled
        target_compile_definitions(ZenithDAWTests PRIVATE ZENITH_USE_SKIA=1)
        if(SKIA_LIBRARY)
            target_link_libraries(ZenithDAWTests PRIVATE ${SKIA_LIBRARY})
        endif()
    endif()
endif()

# 11. Installation & Resources
install(TARGETS ZenithDAW BUNDLE DESTINATION . RUNTIME DESTINATION bin)

add_custom_command(TARGET ZenithDAW POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/apps/desktop/Resources/fonts
    $<TARGET_FILE_DIR:ZenithDAW>/Resources/fonts
    COMMENT "Copying Fonts to Output Directory"
)

# 12. Build Information
message(STATUS "Zenith DAW Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Skia: ${ZENITH_ENABLE_SKIA}")
message(STATUS "  ONNX: ${ENABLE_ONNX}")
message(STATUS "  Tests: ${BUILD_TESTS}")