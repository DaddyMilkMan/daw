# =============================================================================
# CMAKE MODULARIZATION
# =============================================================================

# Platform detection


# Build optimizations
include(cmake/BuildOptimizations.cmake OPTIONAL)

# =============================================================================
# PROJECT DEFINITION
# =============================================================================

cmake_minimum_required(VERSION 3.25)

project(ZenithDAW 
    VERSION 0.1.0
    DESCRIPTION "Zenith DAW - Professional Digital Audio Workstation"
    LANGUAGES CXX C
)

# Platform detection
include(cmake/PlatformDetection.cmake OPTIONAL)

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================

# Default to Release build for Skia compatibility
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, defaulting to Release (Skia requirement)")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Skia debug build warning
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ZENITH_ENABLE_SKIA)
    message(WARNING "Debug build with Skia may cause DLL issues. Use Release for stability.")
endif()

# Platform-specific flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /bigobj" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /bigobj" CACHE STRING "" FORCE)
endif()

# =============================================================================
# DEPENDENCIES
# =============================================================================

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

# Core JUCE modules for Zenith DAW
set(ZENITH_JUCE_MODULES
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

message(STATUS "Zenith DAW: JUCE 8.0.0 configured")

# =============================================================================
# SOURCE FILES
# =============================================================================

# Include organized source files
include(cmake/SourceFiles.cmake OPTIONAL)

# Fallback if modular file not found
if(NOT DEFINED ZENITH_ALL_SOURCES)
    message(STATUS "Using fallback source collection")
    file(GLOB_RECURSE ZENITH_ALL_SOURCES 
        "apps/desktop/Source/*.cpp"
        "apps/desktop/Source/engine/*.cpp"
        "apps/desktop/Source/ui/*.cpp"
    )
endif()

# Test sources (optional)
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
)

# =============================================================================
# TARGET DEFINITION
# =============================================================================

# Create main executable
juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "Zenith Audio"
    VERSION ${PROJECT_VERSION}
    BUNDLE_ID com.zenithaudio.daw
)

# Generate JuceHeader.h
juce_generate_juce_header(ZenithDAW)

# Add source files
target_sources(ZenithDAW PRIVATE ${ZENITH_ALL_SOURCES})

# =============================================================================
# INCLUDE DIRECTORIES
# =============================================================================

target_include_directories(ZenithDAW PUBLIC
    ${CMAKE_BINARY_DIR}/ZenithDAW_artefacts/JuceLibraryCode
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source
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

# =============================================================================
# LIBRARY LINKING
# =============================================================================

# Link JUCE modules
target_link_libraries(ZenithDAW PRIVATE ${ZENITH_JUCE_MODULES})

# =============================================================================
# SKIA INTEGRATION (after target creation)
# =============================================================================

# Skia rendering option (Release recommended)
option(ZENITH_ENABLE_SKIA "Enable Skia Hardware-Accelerated Rendering" ON)

if(ZENITH_ENABLE_SKIA)
    # Manual Skia integration
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/cmake/SkiaManualIntegration.cmake")
        include(apps/desktop/cmake/SkiaManualIntegration.cmake)
        target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)
        message(STATUS "Zenith DAW: Skia rendering ENABLED")
        
        # Platform-specific graphics libraries
        if(WIN32)
            target_link_libraries(ZenithDAW PRIVATE d3d12 dxgi dcomp uuid)
        elseif(APPLE)
            target_link_libraries(ZenithDAW PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
        elseif(UNIX AND NOT APPLE)
            find_package(Vulkan QUIET)
            if(Vulkan_FOUND)
                target_link_libraries(ZenithDAW PRIVATE Vulkan::Vulkan)
            else()
                target_link_libraries(ZenithDAW PRIVATE vulkan)
                message(STATUS "Vulkan not found - using system fallback")
            endif()
        endif()
    else()
        message(WARNING "Skia integration file not found. Skia will be disabled.")
        set(ZENITH_ENABLE_SKIA OFF)
    endif()
else()
    message(STATUS "Zenith DAW: Skia rendering DISABLED")
endif()

# =============================================================================
# ONNX RUNTIME INTEGRATION (after target creation)
# =============================================================================

# ONNX Runtime for AI features (Linux only currently)
option(ENABLE_ONNX "Enable ONNX Runtime for AI features" ON)

if(ENABLE_ONNX)
    if(UNIX AND NOT APPLE)
        include(FetchContent)
        FetchContent_Declare(
            onnxruntime
            URL https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-linux-x64-1.17.1.tgz
        )
        FetchContent_MakeAvailable(onnxruntime)
        
        # Global compile definitions
        target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_ONNX_RUNTIME=1)
        target_link_libraries(ZenithDAW PRIVATE ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so)
        target_include_directories(ZenithDAW PRIVATE ${onnxruntime_SOURCE_DIR}/include)
        
        # Copy ONNX library to output directory
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
        
        message(STATUS "Zenith DAW: ONNX Runtime support ENABLED (Linux)")
    else()
        message(STATUS "Zenith DAW: ONNX Runtime support disabled on this platform")
        set(ENABLE_ONNX OFF)
    endif()
else()
    message(STATUS "Zenith DAW: ONNX Runtime support disabled by user")
endif()

# =============================================================================
# COMPILE DEFINITIONS
# =============================================================================

target_compile_definitions(ZenithDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
)

# =============================================================================
# COMPILER FEATURES
# =============================================================================

target_compile_features(ZenithDAW PRIVATE cxx_std_20)
set_target_properties(ZenithDAW PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# =============================================================================
# INTERPROCEDURAL OPTIMIZATION
# =============================================================================

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

# =============================================================================
# BUILD OPTIMIZATIONS
# =============================================================================

# Apply build optimizations if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/cmake/BuildOptimizations.cmake")
    include(apps/desktop/cmake/BuildOptimizations.cmake)
    enable_zenith_build_optimizations(ZenithDAW)
endif()

# =============================================================================
# PRECOMPILED HEADERS
# =============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/cmake/PrecompiledHeaders.cmake")
    include(apps/desktop/cmake/PrecompiledHeaders.cmake)
endif()

# =============================================================================
# OPTIONAL TESTS
# =============================================================================

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    juce_add_gui_app(ZenithDAWTests
        PRODUCT_NAME "ZenithDAWTests"
    )
    
    juce_generate_juce_header(ZenithDAWTests)
    
    target_sources(ZenithDAWTests PRIVATE ${ZENITH_TEST_SOURCES} ${ZENITH_ALL_SOURCES})
    
    target_include_directories(ZenithDAWTests PRIVATE 
        ${CMAKE_BINARY_DIR}/ZenithDAWTests_artefacts/JuceLibraryCode
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/framework
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/design-system
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/arranger
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/mixer
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/piano-roll
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/transport
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/common
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/widgets
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/session
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/instruments
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/dialogs
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/sample-editor
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/commands
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/instruments
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/network
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/dsp
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ai
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/skia
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/utils
        ${CMAKE_PREFIX_PATH}/include
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
        "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules"
    )
    
    target_link_libraries(ZenithDAWTests PRIVATE ${ZENITH_JUCE_MODULES})
    
    target_compile_definitions(ZenithDAWTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    
    target_compile_features(ZenithDAWTests PRIVATE cxx_std_20)
    
    if(ZENITH_ENABLE_SKIA)
        # Re-use SKIA_ROOT/SKIA_INCLUDE_DIR detected by SkiaManualIntegration.cmake
        target_include_directories(ZenithDAWTests PRIVATE ${SKIA_ROOT}/include ${SKIA_INCLUDE_DIR} ${SKIA_INCLUDE_DIR}/include)
        if(SKIA_LIBRARY)
            target_link_libraries(ZenithDAWTests PRIVATE ${SKIA_LIBRARY})
        endif()
        target_compile_definitions(ZenithDAWTests PRIVATE ZENITH_USE_SKIA=1)
        
        # Platform libs similar to ZenithDAW
        if(WIN32)
            target_link_libraries(ZenithDAWTests PRIVATE d3d12 dxgi dcomp uuid)
        elseif(APPLE)
            target_link_libraries(ZenithDAWTests PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
        elseif(UNIX AND NOT APPLE)
            target_link_libraries(ZenithDAWTests PRIVATE vulkan) 
        endif()
    endif()
endif()

# =============================================================================
# INSTALLATION
# =============================================================================

install(TARGETS ZenithDAW
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# =============================================================================
# RESOURCE COPYING
# =============================================================================

# Copy Resources (Fonts) to Output Directory
add_custom_command(TARGET ZenithDAW POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/apps/desktop/Resources/fonts
    $<TARGET_FILE_DIR:ZenithDAW>/Resources/fonts
    COMMENT "Copying Fonts to Output Directory"
)

# =============================================================================
# BUILD INFORMATION
# =============================================================================

message(STATUS "Zenith DAW Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${ZENITH_PLATFORM_NAME}")
message(STATUS "  Architecture: ${ZENITH_ARCH_NAME}")
message(STATUS "  Skia: ${ZENITH_ENABLE_SKIA}")
message(STATUS "  ONNX: ${ENABLE_ONNX}")
message(STATUS "  IPO: ${ENABLE_IPO}")
message(STATUS "  Tests: ${BUILD_TESTS}")
