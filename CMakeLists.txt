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
else()
    # Suppress Skia vcpkg header warnings (clang::reinitializes attribute)
    # These come from external/vcpkg headers and cannot be fixed in our code
    add_compile_options(-Wno-attributes -Wno-unknown-attributes)
endif()

# =============================================================================
# DEPENDENCIES
# =============================================================================

# Centralized dependency management (JUCE, ONNX, OpenSSL, etc.)
include(cmake/Dependencies.cmake)

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
    apps/desktop/Source/tests/ProjectFileIOTests.cpp
    apps/desktop/Source/tests/EngineTests.cpp
    apps/desktop/Source/tests/ConcurrencyTests.cpp
    apps/desktop/Source/tests/ThemeTests.cpp
    apps/desktop/Source/tests/Midi2Tests.cpp
    apps/desktop/Source/tests/AITests.cpp
    apps/desktop/Source/tests/AccessibilityTests.cpp
    apps/desktop/Source/tests/CRDTSyncTests.cpp
    apps/desktop/Source/tests/OnnxPipelineTests.cpp
    apps/desktop/Source/tests/PluginAutomationTests.cpp
    apps/desktop/Source/tests/SampleEditorRecordingTests.cpp
    apps/desktop/Source/tests/SampleEditorRecordingTests.cpp
    apps/desktop/Source/tests/MixerChannelBenchmark.cpp
    apps/desktop/Source/tests/TrackGroupHeaderTest.cpp
    apps/desktop/Source/tests/SIMDHelpersTest.cpp
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
# juce_generate_juce_header(ZenithDAW)

# Add source files
target_sources(ZenithDAW PRIVATE ${ZENITH_ALL_SOURCES})

# =============================================================================
# COMPILE DEFINITIONS
# =============================================================================

target_compile_definitions(ZenithDAW PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_ALSA=1  # Ensure ALSA is used on Linux
)

# =============================================================================
# INCLUDE DIRECTORIES
# =============================================================================

# Load centralized interface libraries for includes
include(cmake/TargetIncludes.cmake)

# Build artifacts includes (PRIVATE - internal to ZenithDAW target)
target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_BINARY_DIR}/ZenithDAW_artefacts/JuceLibraryCode
    ${CMAKE_BINARY_DIR}/JuceLibraryCode
)

# Link interface libraries for source includes
# This provides all Source/ subdirectories via zenith_app_includes
target_link_libraries(ZenithDAW PRIVATE zenith_app_includes)

# =============================================================================
# LIBRARY LINKING
# =============================================================================

# Link JUCE modules
target_link_libraries(ZenithDAW PRIVATE ${ZENITH_JUCE_MODULES})


# =============================================================================
# SKIA INTEGRATION (after target creation)
# =============================================================================

# Skia rendering is now REQUIRED for Zenith DAW
message(STATUS "Zenith DAW: Skia rendering ENABLED (required)")

# Manual Skia integration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/cmake/SkiaManualIntegration.cmake")
    include(apps/desktop/cmake/SkiaManualIntegration.cmake)
    target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)

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
    message(FATAL_ERROR "Skia integration file not found. Skia is required for Zenith DAW rendering.")
endif()

# =============================================================================
# ONNX RUNTIME INTEGRATION (after target creation)
# =============================================================================

# =============================================================================
# ONNX RUNTIME INTEGRATION (Post-Target)
# =============================================================================

if(ZENITH_HAS_ONNX)
    # Global compile definitions
    target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_ONNX_RUNTIME=1)
    
    # Link against imported target from Dependencies.cmake
    target_link_libraries(ZenithDAW PRIVATE onnxruntime)
    
    # Copy ONNX library to output directory
    add_custom_command(TARGET ZenithDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNX_RUNTIME_LIBRARY_DIR}/${ONNX_RUNTIME_SHARED_LIB_VERSIONED}"
        "$<TARGET_FILE_DIR:ZenithDAW>/${ONNX_RUNTIME_SHARED_LIB_VERSIONED}"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${ONNX_RUNTIME_SHARED_LIB_VERSIONED}"
        "$<TARGET_FILE_DIR:ZenithDAW>/${ONNX_RUNTIME_SHARED_LIB}"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${ONNX_RUNTIME_SHARED_LIB_VERSIONED}"
        "$<TARGET_FILE_DIR:ZenithDAW>/libonnxruntime.so.1" # Legacy compat
        COMMENT "Copying ONNX Runtime library to output directory"
    )
endif()

# =============================================================================
# OPENSSL (Collaboration Security)
# =============================================================================

if(ZENITH_ENABLE_COLLAB)
    # Link against standard targets defined in Dependencies.cmake
    target_link_libraries(ZenithDAW PRIVATE OpenSSL::SSL OpenSSL::Crypto)

    if (UNIX AND NOT APPLE)
        target_link_libraries(ZenithDAW PRIVATE dl)
    endif()
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
    juce_add_console_app(ZenithDAWTests
        PRODUCT_NAME "ZenithDAWTests"
    )

    juce_generate_juce_header(ZenithDAWTests)

    set(ZENITH_TEST_APP_SOURCES ${ZENITH_ALL_SOURCES})
    list(FILTER ZENITH_TEST_APP_SOURCES EXCLUDE REGEX "Main\\.cpp$")
    target_sources(ZenithDAWTests PRIVATE ${ZENITH_TEST_SOURCES} ${ZENITH_TEST_APP_SOURCES})

    # Use centralized interface libraries (same as ZenithDAW)
    target_include_directories(ZenithDAWTests PRIVATE
        ${CMAKE_BINARY_DIR}/ZenithDAWTests_artefacts/JuceLibraryCode
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
    )
    
    # Link interface libraries for source includes
    target_link_libraries(ZenithDAWTests PRIVATE zenith_app_includes)
    target_link_libraries(ZenithDAWTests PRIVATE ${ZENITH_JUCE_MODULES})


    target_compile_definitions(ZenithDAWTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_compile_features(ZenithDAWTests PRIVATE cxx_std_20)

    # Skia is always enabled for tests
    # Re-use SKIA_ROOT/SKIA_INCLUDE_DIR detected by SkiaManualIntegration.cmake
    target_include_directories(ZenithDAWTests PRIVATE ${SKIA_ROOT}/include ${SKIA_INCLUDE_DIR})
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
        target_link_libraries(ZenithDAWTests PRIVATE vulkan EGL GLESv2)
    endif()

    # =========================================================================
    # MetricsChartTests - Lightweight test for MetricsChart multi-series
    # =========================================================================
    juce_add_console_app(MetricsChartTests
        PRODUCT_NAME "MetricsChartTests"
    )

    juce_generate_juce_header(MetricsChartTests)

    target_sources(MetricsChartTests PRIVATE
        apps/desktop/Source/tests/TestRunner.cpp
        apps/desktop/Source/tests/UITests.cpp
        apps/desktop/Source/ui/dashboards/MetricsChart.cpp
    )

    # Use centralized interface libraries
    target_include_directories(MetricsChartTests PRIVATE
        ${CMAKE_BINARY_DIR}/MetricsChartTests_artefacts/JuceLibraryCode
    )
    
    # Link minimal interface libraries (lightweight test)
    target_link_libraries(MetricsChartTests PRIVATE 
        zenith_core_includes
        zenith_ui_includes
        zenith_juce_includes
    )

    # Skia includes (always enabled)
    target_include_directories(MetricsChartTests PRIVATE 
        ${SKIA_ROOT}/include 
        ${SKIA_INCLUDE_DIR}
    )

    target_link_libraries(MetricsChartTests PRIVATE
        juce::juce_core
        juce::juce_events
        juce::juce_graphics
        juce::juce_data_structures
        juce::juce_gui_basics
    )


    # Skia is always linked for tests
    target_link_libraries(MetricsChartTests PRIVATE ${SKIA_LIBRARY})
    target_compile_definitions(MetricsChartTests PRIVATE ZENITH_USE_SKIA=1)
    
    if(WIN32)
        target_link_libraries(MetricsChartTests PRIVATE d3d12 dxgi dcomp uuid)
    elseif(APPLE)
        target_link_libraries(MetricsChartTests PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(MetricsChartTests PRIVATE vulkan)
    endif()

    target_compile_definitions(MetricsChartTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        ZENITH_TEST_MODE=1
    )

    target_compile_features(MetricsChartTests PRIVATE cxx_std_20)
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
# PYTHON INTEGRATION (Signaling Server)
# =============================================================================

option(ZENITH_ENABLE_COLLAB "Enable Collaboration Features (requires Python3)" OFF)

if(ZENITH_ENABLE_COLLAB)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    if(Python3_FOUND)
        message(STATUS "Found Python 3: ${Python3_EXECUTABLE}")

        set(PYTHON_DEPS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/backend/signaling/requirements.txt)
        set(PYTHON_SERVER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/backend/signaling/signaling_server.py)

        # Custom target to install python dependencies
        add_custom_target(
            install_python_deps
            COMMAND ${Python3_EXECUTABLE} -m pip install -r ${PYTHON_DEPS_FILE}
            COMMENT "Installing Python dependencies for signaling server"
            USES_TERMINAL
        )

        # Custom target to run the signaling server
        add_custom_target(
            run_signaling_server
            COMMAND ${Python3_EXECUTABLE} ${PYTHON_SERVER_SCRIPT}
            COMMENT "Running Python signaling server"
            USES_TERMINAL
        )

        # Make install_python_deps a dependency of the main app, so it's always considered
        add_dependencies(ZenithDAW install_python_deps)

        message(STATUS "Collaboration features enabled. Use target 'run_signaling_server' to start the server.")
    else()
        message(WARNING "Python 3 not found, collaboration features will be unavailable.")
        set(ZENITH_ENABLE_COLLAB OFF)
    endif()
else()
    message(STATUS "Collaboration features disabled by user.")
endif()

# =============================================================================
# BUILD INFORMATION
# =============================================================================

message(STATUS "Zenith DAW Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${ZENITH_PLATFORM_NAME}")
message(STATUS "  Architecture: ${ZENITH_ARCH_NAME}")
message(STATUS "  Skia: REQUIRED (always enabled)")
message(STATUS "  ONNX: ${ENABLE_ONNX}")
message(STATUS "  IPO: ${ENABLE_IPO}")
message(STATUS "  Tests: ${BUILD_TESTS}")
