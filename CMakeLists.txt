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

# Disable JUCE tools early to prevent build issues
set(JUCE_DISABLE_JUCEAIDE ON CACHE BOOL "Disable JUCEAIDE build tool" FORCE)
set(JUCE_BUILD_HELPER_TOOLS OFF CACHE BOOL "Disable JUCE helper tools" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Disable JUCE extras" FORCE)

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

option(ZENITH_BUILD_APP "Build the main application" ON)
option(ZENITH_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(ZENITH_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
        message(STATUS "Code coverage instrumentation enabled (GCC/Clang)")
    else()
        message(WARNING "Code coverage enabled but compiler '${CMAKE_CXX_COMPILER_ID}' is not explicitly supported by this configuration.")
    endif()
endif()

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

# Disable JUCEAIDE and tools to avoid build issues
set(JUCE_DISABLE_JUCEAIDE ON CACHE BOOL "Disable JUCEAIDE build tool" FORCE)
set(JUCE_BUILD_HELPER_TOOLS OFF CACHE BOOL "Disable JUCE helper tools" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Disable JUCE extras" FORCE)

# Centralized dependency management (JUCE, ONNX, OpenSSL, etc.)
include(cmake/Dependencies.cmake)

# Disable yaml-cpp to avoid CMake version conflicts
set(YAML_CPP_BUILD OFF CACHE BOOL "Disable yaml-cpp build" FORCE)

# =============================================================================
# SOURCE FILES
# =============================================================================

# Include organized source files
include(cmake/SourceFiles.cmake OPTIONAL)

# Fallback if modular file not found
if(NOT DEFINED ZENITH_ALL_SOURCES)
    message(STATUS "Using fallback source collection")
    file(GLOB_RECURSE ZENITH_ALL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/*.cpp"
    )
endif()

# Test sources (optional)
set(ZENITH_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/TestMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/AudioEngineTests.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/ProjectStateTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/ProjectFileIOTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/EngineTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/ConcurrencyTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/ThemeTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/Midi2Tests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/AITests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/AccessibilityTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/CRDTSyncTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/OnnxPipelineTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/PluginAutomationTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/SampleEditorRecordingTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/MixerChannelBenchmark.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/TrackGroupHeaderTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/SIMDHelpersTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/ResamplingTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/WavetableLoaderBenchmark.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/AudioBufferConverterTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/ObservabilityAgentTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/SampleGeneratorBenchmark.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/SampleGeneratorTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/ZenithControlTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/OversamplingBenchmark.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/agents/RealtimeAudioEngineAgent/tests/RealtimeAudioEngineAgentTest.cpp"
)

# =============================================================================
# TARGET DEFINITION
# =============================================================================

if(ZENITH_BUILD_APP)

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
# PLUGIN SCANNER EXECUTABLE (Subprocess for crash isolation)
# =============================================================================

juce_add_console_app(ZenithPluginScanner
    PRODUCT_NAME "ZenithPluginScanner"
)

# Generate JuceHeader.h for scanner
juce_generate_juce_header(ZenithPluginScanner)

# Add scanner source file
target_sources(ZenithPluginScanner PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/PluginScanner.cpp"
)

# Use centralized interface libraries
target_include_directories(ZenithPluginScanner PRIVATE
    ${CMAKE_BINARY_DIR}/ZenithPluginScanner_artefacts/JuceLibraryCode
    ${CMAKE_BINARY_DIR}/JuceLibraryCode
)

# Link interface libraries for source includes
target_link_libraries(ZenithPluginScanner PRIVATE zenith_app_includes)

# Link minimal JUCE modules needed for plugin scanning
target_link_libraries(ZenithPluginScanner PRIVATE
    juce::juce_core
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_events
    juce::juce_gui_basics
    juce::juce_data_structures
)

# Compile definitions for scanner
target_compile_definitions(ZenithPluginScanner PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_compile_features(ZenithPluginScanner PRIVATE cxx_std_20)

# Install scanner alongside main app
install(TARGETS ZenithPluginScanner
    RUNTIME DESTINATION bin
)

message(STATUS "ZenithPluginScanner configured for out-of-process plugin scanning")

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
            # X11 for window decoration control
            find_package(X11 REQUIRED)
            target_link_libraries(ZenithDAW PRIVATE ${X11_LIBRARIES})
            target_include_directories(ZenithDAW PRIVATE ${X11_INCLUDE_DIR})
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

endif()

# =============================================================================
# OPTIONAL TESTS
# =============================================================================

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    if(ZENITH_BUILD_APP)
    juce_add_console_app(ZenithDAWTests
        PRODUCT_NAME "ZenithDAWTests"
    )

    juce_generate_juce_header(ZenithDAWTests)

    set(ZENITH_TEST_APP_SOURCES ${ZENITH_ALL_SOURCES})
    list(FILTER ZENITH_TEST_APP_SOURCES EXCLUDE REGEX "Main\.cpp$")
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

    # Configure LeakSanitizer suppression file
    if(ENABLE_SANITIZERS AND NOT MSVC)
        set_target_properties(ZenithDAWTests PROPERTIES
            ENVIRONMENT "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/lsan.supp"
        )
        message(STATUS "LeakSanitizer suppressions configured for ZenithDAWTests")
    endif()

    if(ZENITH_ENABLE_SKIA)
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
            find_package(X11 REQUIRED)
            target_link_libraries(ZenithDAWTests PRIVATE ${X11_LIBRARIES})
            target_include_directories(ZenithDAWTests PRIVATE ${X11_INCLUDE_DIR})
        endif()

        if(WIN32 AND DEFINED SKIA_BIN_DIR)
            file(GLOB _zenith_skia_runtime_dlls "${SKIA_BIN_DIR}/*.dll")
            foreach(_dll ${_zenith_skia_runtime_dlls})
                get_filename_component(_dll_name ${_dll} NAME)
                add_custom_command(TARGET ZenithDAWTests POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dll}"
                    "$<TARGET_FILE_DIR:ZenithDAWTests>/${_dll_name}"
                )
            endforeach()
        endif()
    endif()

    # =========================================================================
    # MetricsChartTests - Lightweight test for MetricsChart multi-series
    # =========================================================================
    juce_add_console_app(MetricsChartTests
        PRODUCT_NAME "MetricsChartTests"
    )

    juce_generate_juce_header(MetricsChartTests)

    target_sources(MetricsChartTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/TestRunner.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/UITests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ui/dashboards/MetricsChart.cpp"
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

    # Skia includes (if enabled)
    if(ZENITH_ENABLE_SKIA)
        target_include_directories(MetricsChartTests PRIVATE 
            ${SKIA_ROOT}/include 
            ${SKIA_INCLUDE_DIR}
        )
    endif()

    target_link_libraries(MetricsChartTests PRIVATE
        juce::juce_core
        juce::juce_events
        juce::juce_graphics
        juce::juce_data_structures
        juce::juce_gui_basics
    )


    if(ZENITH_ENABLE_SKIA)
        target_link_libraries(MetricsChartTests PRIVATE ${SKIA_LIBRARY})
        target_compile_definitions(MetricsChartTests PRIVATE ZENITH_USE_SKIA=1)
        
        if(WIN32)
            target_link_libraries(MetricsChartTests PRIVATE d3d12 dxgi dcomp uuid)
        elseif(APPLE)
            target_link_libraries(MetricsChartTests PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
        elseif(UNIX AND NOT APPLE)
            target_link_libraries(MetricsChartTests PRIVATE vulkan)
        endif()
    endif()

    target_compile_definitions(MetricsChartTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        ZENITH_TEST_MODE=1
    )

    target_compile_features(MetricsChartTests PRIVATE cxx_std_20)

    # =========================================================================
    # WavetableBenchmark - Performance benchmark for WavetableLoader
    # =========================================================================
    juce_add_console_app(WavetableBenchmark
        PRODUCT_NAME "WavetableBenchmark"
    )

    juce_generate_juce_header(WavetableBenchmark)

    target_sources(WavetableBenchmark PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/WavetableLoaderBenchmark.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/instruments/WavetableLoader.cpp"
    )

    target_include_directories(WavetableBenchmark PRIVATE
        ${CMAKE_BINARY_DIR}/WavetableBenchmark_artefacts/JuceLibraryCode
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
    )

    target_link_libraries(WavetableBenchmark PRIVATE
        zenith_juce_includes
        juce::juce_core
        juce::juce_audio_formats
        juce::juce_audio_basics
    )

    target_compile_definitions(WavetableBenchmark PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        WAVETABLE_BENCHMARK_STANDALONE=1
    )

    target_compile_features(WavetableBenchmark PRIVATE cxx_std_20)
    endif()

    # =========================================================================
    # ClockSyncAgentTests - Lightweight test for ClockSyncAgent
    # =========================================================================
    add_executable(ClockSyncAgentTests)

    target_sources(ClockSyncAgentTests PRIVATE
        "agents/ClockSyncAgent/tests/RunTests.cpp"
        "agents/ClockSyncAgent/tests/ClockSyncAgentMidiTests.cpp"
        "agents/ClockSyncAgent/ClockSyncAgent.cpp"
        "agents/ClockSyncAgent/protocols/NTPProtocol.cpp"
        "agents/ClockSyncAgent/protocols/PTPProtocol.cpp"
    )

    target_include_directories(ClockSyncAgentTests PRIVATE
        ${CMAKE_BINARY_DIR}/ClockSyncAgentTests_artefacts/JuceLibraryCode
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
        "agents/ClockSyncAgent"
    )

    target_link_libraries(ClockSyncAgentTests PRIVATE
        juce::juce_core
        juce::juce_audio_basics
    )

    target_compile_definitions(ClockSyncAgentTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
    )

    if (UNIX AND NOT APPLE)
        target_link_libraries(ClockSyncAgentTests PRIVATE pthread dl)
    endif()

    target_compile_features(ClockSyncAgentTests PRIVATE cxx_std_20)

    # =========================================================================
    # ScheduledTransportAgentTests - Lightweight test for ScheduledTransportAgent
    # =========================================================================
    add_executable(ScheduledTransportAgentTests)

    target_sources(ScheduledTransportAgentTests PRIVATE
        "agents/ScheduledTransportAgent/tests/RunTests.cpp"
        "agents/ScheduledTransportAgent/tests/ScheduledTransportAgentTest.cpp"
        "agents/ScheduledTransportAgent/ScheduledTransportAgent.cpp"
    )

    target_include_directories(ScheduledTransportAgentTests PRIVATE
        ${CMAKE_BINARY_DIR}/ScheduledTransportAgentTests_artefacts/JuceLibraryCode
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
        "agents/ScheduledTransportAgent"
    )

    target_link_libraries(ScheduledTransportAgentTests PRIVATE
        juce::juce_core
        juce::juce_audio_basics
    )

    target_compile_definitions(ScheduledTransportAgentTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
    )

    if (UNIX AND NOT APPLE)
        target_link_libraries(ScheduledTransportAgentTests PRIVATE pthread dl)
    endif()

    target_compile_features(ScheduledTransportAgentTests PRIVATE cxx_std_20)

    # =========================================================================
    # TransportProtocolAgentTests - Tests for TransportProtocolAgent
    # =========================================================================
    add_executable(TransportProtocolAgentTests)

    target_sources(TransportProtocolAgentTests PRIVATE
        "agents/TransportProtocolAgent/tests/TransportProtocolAgentTests.cpp"
        "agents/TransportProtocolAgent/TransportProtocolAgent.cpp"
    )

    target_include_directories(TransportProtocolAgentTests PRIVATE
        ${CMAKE_BINARY_DIR}/TransportProtocolAgentTests_artefacts/JuceLibraryCode
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
        "agents/TransportProtocolAgent"
    )

    target_link_libraries(TransportProtocolAgentTests PRIVATE
        juce::juce_core
        juce::juce_audio_devices
        juce::juce_events
    )

    target_compile_definitions(TransportProtocolAgentTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
    )

    if (UNIX AND NOT APPLE)
        target_link_libraries(TransportProtocolAgentTests PRIVATE pthread dl)
    endif()

    target_compile_features(TransportProtocolAgentTests PRIVATE cxx_std_20)

    # =========================================================================
    # ObservabilityTest - Lightweight test for ObservabilityAgent
    # =========================================================================
    add_executable(ObservabilityTest)

    target_sources(ObservabilityTest PRIVATE
        "agents/ObservabilityAgent/tests/RunTests.cpp"
        "agents/ObservabilityAgent/tests/ObservabilityAgentTest.cpp"
        "agents/ObservabilityAgent/ObservabilityAgent.cpp"
        "agents/ObservabilityAgent/PrometheusExporter.cpp"
    )

    target_include_directories(ObservabilityTest PRIVATE
        ${CMAKE_BINARY_DIR}/ObservabilityTest_artefacts/JuceLibraryCode
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
        "agents/ObservabilityAgent"
    )

    target_link_libraries(ObservabilityTest PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_events
    )

    target_compile_definitions(ObservabilityTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
    )

    if (UNIX AND NOT APPLE)
        target_link_libraries(ObservabilityTest PRIVATE pthread dl)
    endif()

    target_compile_features(ObservabilityTest PRIVATE cxx_std_20)

    # =========================================================================
    # =========================================================================
    # RealtimeAudioEngineAgentTest - Lightweight test for RealtimeAudioEngineAgent
    # =========================================================================
    add_executable(RealtimeAudioEngineAgentTest)

    target_sources(RealtimeAudioEngineAgentTest PRIVATE
        "agents/RealtimeAudioEngineAgent/tests/RunTests.cpp"
        "agents/RealtimeAudioEngineAgent/tests/RealtimeAudioEngineAgentTest.cpp"
        "agents/RealtimeAudioEngineAgent/RealtimeAudioEngineAgent.cpp"
    )

    target_include_directories(RealtimeAudioEngineAgentTest PRIVATE
        ${CMAKE_BINARY_DIR}/RealtimeAudioEngineAgentTest_artefacts/JuceLibraryCode
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
        "agents/RealtimeAudioEngineAgent"
        "apps/desktop/Source"
    )

    target_link_libraries(RealtimeAudioEngineAgentTest PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_events
    )

    target_compile_definitions(RealtimeAudioEngineAgentTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
    )

    if (UNIX AND NOT APPLE)
        target_link_libraries(RealtimeAudioEngineAgentTest PRIVATE pthread dl)
    endif()

    target_compile_features(RealtimeAudioEngineAgentTest PRIVATE cxx_std_20)

    # =========================================================================
    # SampleGeneratorBenchmark - Performance benchmark for SampleGenerator
    # =========================================================================
    add_executable(SampleGeneratorBenchmarkApp
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/SampleGeneratorBenchmark.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/utils/SampleGenerator.cpp"
    )

    target_include_directories(SampleGeneratorBenchmarkApp PRIVATE
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
    )

    target_link_libraries(SampleGeneratorBenchmarkApp PRIVATE
        zenith_core_includes
        zenith_juce_includes
        juce::juce_core
        juce::juce_audio_formats
        juce::juce_audio_basics
    )

    target_compile_definitions(SampleGeneratorBenchmarkApp PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        SAMPLE_GENERATOR_BENCHMARK_STANDALONE=1
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
    )

    target_compile_features(SampleGeneratorBenchmarkApp PRIVATE cxx_std_20)

    if(UNIX AND NOT APPLE)
        target_link_libraries(SampleGeneratorBenchmarkApp PRIVATE pthread dl)
    endif()

    # =========================================================================
    # SampleGeneratorTests - Verification tests for SampleGenerator
    # =========================================================================
    add_executable(SampleGeneratorTestsApp
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/tests/SampleGeneratorTests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/utils/SampleGenerator.cpp"
    )

    target_include_directories(SampleGeneratorTestsApp PRIVATE
        ${CMAKE_BINARY_DIR}/JuceLibraryCode
    )

    target_link_libraries(SampleGeneratorTestsApp PRIVATE
        zenith_core_includes
        zenith_juce_includes
        juce::juce_core
        juce::juce_audio_formats
        juce::juce_audio_basics
    )

    target_compile_definitions(SampleGeneratorTestsApp PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        SAMPLE_GENERATOR_TESTS_STANDALONE=1
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
    )

    target_compile_features(SampleGeneratorTestsApp PRIVATE cxx_std_20)

    if(UNIX AND NOT APPLE)
        target_link_libraries(SampleGeneratorTestsApp PRIVATE pthread dl)
    endif()

endif()

# =============================================================================
# INSTALLATION
# =============================================================================

if(ZENITH_BUILD_APP)

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
    "$<TARGET_FILE_DIR:ZenithDAW>/Resources/fonts"
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

endif()

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