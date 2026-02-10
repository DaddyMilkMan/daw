# =============================================================================
# ZENITH DAW - MODULAR CMAKE CONFIGURATION
# =============================================================================
#
# Welcome to the Zenith DAW build system!
#
# Quick Start:
#   mkdir build && cd build
#   cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build .
#
# For detailed instructions, see BUILDING.md
# =============================================================================

cmake_minimum_required(VERSION 3.25)

# =============================================================================
# PROJECT DEFINITION
# =============================================================================

project(ZenithDAW
    VERSION 0.1.0
    DESCRIPTION "Zenith DAW - Professional Digital Audio Workstation"
    LANGUAGES CXX C
)

# Set Global C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Print welcome message
message(STATUS "")
message(STATUS "🎵 Welcome to Zenith DAW Build System!")
message(STATUS "========================================")
message(STATUS "")
message(STATUS "For detailed build instructions, see: BUILDING.md")
message(STATUS "For development guide, see: docs/DEVELOPER.md")
message(STATUS "")

# =============================================================================
# LICENSE INFORMATION
# =============================================================================

# License information
set(CMAKE_PROJECT_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION} (AGPL v3)")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://www.gnu.org/licenses/agpl-3.0.html")

# Add license file to installation
install(FILES LICENSE NOTICE
    DESTINATION share/doc/zenithdaw
)

# =============================================================================
# CONTRIBUTOR-FRIENDLY BUILD OPTIONS
# =============================================================================

# User-friendly option descriptions with help text
option(ZENITH_BUILD_APP "Build the main application (ZenithDAW executable)" ON)

option(ZENITH_BUILD_UI_UNIFIED
    "Build the experimental unified UI library (zenith_ui_unified)"
    OFF
)

option(ZENITH_BUILD_TESTS
    "Build test suite (ZenithTests executable)"
    OFF
)
# "Enable building of unit and integration tests. Use with -DCMAKE_BUILD_TYPE=Debug for best results"

option(ZENITH_ENABLE_SKIA
    "Enable Skia GPU-accelerated rendering"
    OFF
)
# "Disabling Skia will fallback to basic rendering. Not recommended for production use"

option(ZENITH_ENABLE_COVERAGE
    "Enable code coverage reporting (requires gcov/lcov)"
    OFF
)
# "Enables coverage reports. Use with -DCMAKE_BUILD_TYPE=Debug for best results"

option(ZENITH_ENABLE_WCET
    "Enable Worst-Case Execution Time monitoring for real-time audio threads"
    ON
)
# "WCET monitoring helps identify real-time audio thread violations. Should remain ON for production"

option(ZENITH_FETCH_DEPS
    "Fetch dependencies automatically if not found locally"
    ON
)

option(ENABLE_TESTS
    "Enable unit testing"
    ON
)
# "When enabled, the build system will automatically download missing dependencies"

option(ZENITH_ENABLE_DEBUG_SYMBOLS
    "Generate debug symbols even in Release builds"
    OFF
)
# "When enabled, adds debug symbols to Release builds for production debugging"

option(ZENITH_ENABLE_ASSERTIONS
    "Enable runtime assertions for debugging"
    ON
)
# "Disabling assertions may improve performance but reduces debuggability"

option(ZENITH_ENABLE_SANITIZERS
    "Enable AddressSanitizer and ThreadSanitizer for debugging"
    OFF
)
# "When enabled, adds memory and thread safety checking - significantly impacts performance but provides detailed error reports"

option(ZENITH_ENABLE_THREAD_SANITIZER
    "Enable ThreadSanitizer specifically for data race detection"
    OFF
)
# "ThreadSanitizer detects data races and deadlocks in multithreaded code"

option(ZENITH_ENABLE_ADDRESS_SANITIZER
    "Enable AddressSanitizer for memory error detection"
    OFF
)
# "AddressSanitizer detects memory errors like buffer overflows, use-after-free, etc."

option(ZENITH_ENABLE_LUA_SCRIPTING
    "Enable experimental Lua scripting processor (NOT RT-safe)"
    OFF
)

# =============================================================================
# BUILD CONFIGURATION HELPERS
# =============================================================================

# Function to provide user-friendly build information
function(print_build_info)
    message(STATUS "")
    message(STATUS "📋 Zenith DAW Build Configuration:")
    message(STATUS "─────────────────────────────────────────")
    message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Build application: ${ZENITH_BUILD_APP}")
    message(STATUS "  Build tests: ${ZENITH_BUILD_TESTS}")
    message(STATUS "  Enable Skia rendering: ${ZENITH_ENABLE_SKIA}")
    message(STATUS "  Enable coverage: ${ZENITH_ENABLE_COVERAGE}")
    message(STATUS "  Enable WCET monitoring: ${ZENITH_ENABLE_WCET}")
    message(STATUS "  Auto-fetch dependencies: ${ZENITH_FETCH_DEPS}")
    message(STATUS "  Debug symbols: ${ZENITH_ENABLE_DEBUG_SYMBOLS}")
    message(STATUS "  Assertions: ${ZENITH_ENABLE_ASSERTIONS}")
    message(STATUS "")
endfunction()

# Function to print build targets
function(print_build_targets)
    message(STATUS "🎯 Available build targets:")
    message(STATUS "────────────────────────────")
    message(STATUS "  - all: Build everything (default)")
    message(STATUS "  - ZenithDAW: Main application executable")
    message(STATUS "  - zenith_core: Core library")
    message(STATUS "  - zenith_engine_core: Engine core library")
    message(STATUS "  - zenith_ui_unified: UI framework library")
    message(STATUS "  - zenith_audio_utils: Audio utilities library")
    message(STATUS "  - zenith_dsp: DSP processing library")
    message(STATUS "  - install: Install the application to system")
    message(STATUS "  - package: Create distributable packages")

    if(ZENITH_BUILD_TESTS)
        message(STATUS "  - ZenithTests: Test suite executable")
        message(STATUS "  - run_tests: Run all tests")
        message(STATUS "  - test_<name>: Run specific test (e.g., test_Core)")
    endif()

    message(STATUS "  - zenith_help: Print available targets and options")
    message(STATUS "  - rebuild: Clean and rebuild everything")
    message(STATUS "")
endfunction()

# Function to print build commands
function(print_build_commands)
    message(STATUS "💡 Common build commands:")
    message(STATUS "────────────────────────────")
    message(STATUS "  Configure CMake:")
    message(STATUS "    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release")
    message(STATUS "    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON")
    message(STATUS "")
    message(STATUS "  Build the project:")
    message(STATUS "    cmake --build . -j$(nproc)")
    message(STATUS "    cmake --build . --config Release -- /m  # Windows")
    message(STATUS "")
    message(STATUS "  Run tests:")
    message(STATUS "    ctest --output-on-failure")
    message(STATUS "    cmake --build . --target run_tests")
    message(STATUS "")
    message(STATUS "  Clean build:")
    message(STATUS "    cmake --build . --target clean")
    message(STATUS "    rm -rf build/*")
    message(STATUS "")
endfunction()

# =============================================================================
# PLATFORM CONFIGURATION
# =============================================================================

# Set build type with user-friendly message
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Release, Debug, RelWithDebInfo)" FORCE)
    message(STATUS "🔧 Build type set to: ${CMAKE_BUILD_TYPE}")
endif()

# Add build type options for easy configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -g3")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG=1 -g3")
    message(STATUS "🔍 Debug build with full debug info")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g3")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g3")
    message(STATUS "🎯 Release build with debug symbols")
endif()

# Platform-specific flags with helpful comments
if(MSVC)
    # MSVC specific settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /bigobj /W3" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /bigobj /W3" CACHE STRING "" FORCE)
    message(STATUS "🪟 Using MSVC compiler flags")
else()
    # GCC/Clang settings
    add_compile_options(-Wno-attributes -Wno-unknown-attributes)

    # Add debug flags for debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3 -O0 -DDEBUG=1)
        message(STATUS "🐛 Debug: -g3 -O0 -DDEBUG=1")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
        message(STATUS "⚡ Release: -O3 -DNDEBUG")
    endif()

    message(STATUS "🐧 Using GCC/Clang compiler flags")
endif()

# Add assertions control
if(ZENITH_ENABLE_ASSERTIONS)
    add_compile_definitions(ENABLE_ASSERTIONS=1)
    message(STATUS "✅ Runtime assertions enabled")
else()
    add_compile_definitions(ENABLE_ASSERTIONS=0)
    message(STATUS "❌ Runtime assertions disabled")
endif()

# Add sanitizer support
if(ZENITH_ENABLE_SANITIZERS)
    # Enable all sanitizers by default when general flag is set
    set(ZENITH_ENABLE_ADDRESS_SANITIZER ON CACHE BOOL "Enable AddressSanitizer" FORCE)
    set(ZENITH_ENABLE_THREAD_SANITIZER ON CACHE BOOL "Enable ThreadSanitizer" FORCE)
endif()

if(ZENITH_ENABLE_THREAD_SANITIZER)
    add_compile_definitions(ENABLE_THREAD_SANITIZER=1)
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=thread -g")
    set(SANITIZER_LINKER_FLAGS "${SANITIZER_LINKER_FLAGS} -fsanitize=thread")
    message(STATUS "🔍 ThreadSanitizer enabled for data race detection")
endif()

if(ZENITH_ENABLE_ADDRESS_SANITIZER)
    add_compile_definitions(ENABLE_ADDRESS_SANITIZER=1)
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined -g")
    set(SANITIZER_LINKER_FLAGS "${SANITIZER_LINKER_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined")
    message(STATUS "🔍 AddressSanitizer enabled for memory error detection")
endif()

if(ZENITH_ENABLE_SANITIZERS)
    # Apply sanitizer flags to all targets
    add_compile_options(${SANITIZER_FLAGS})
    add_link_options(${SANITIZER_LINKER_FLAGS})
    message(STATUS "🔍 All sanitizers enabled - performance will be impacted")
endif()

# =============================================================================
# DEPENDENCIES
# =============================================================================

# JUCE is configured in Dependencies.cmake
# Add JUCE modules to our targets
set(ZENITH_JUCE_MODULES
    juce_core
    juce_cryptography
    juce_events
    juce_graphics
    juce_data_structures
    juce_gui_basics
    juce_gui_extra
    juce_audio_basics
    juce_audio_devices
    juce_audio_formats
    juce_audio_processors
    juce_audio_utils
    juce_dsp
    juce_opengl
)

# Auto-fetch dependencies if enabled
if(ZENITH_FETCH_DEPS)
    message(STATUS "📦 Automatic dependency fetching enabled")
else()
    message(STATUS "📦 Automatic dependency fetching disabled")
endif()

# =============================================================================
# MODULES
# =============================================================================

# Add cmake directory to include path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Detect platform/arch before including modules that key off ZENITH_PLATFORM_* variables.
include(PlatformDetection)

# Include module definitions and dependencies
include(Dependencies)
include(ZenithCore)
include(ZenithDSP)
include(ZenithAudioUtils)
include(ZenithEngineCore)
include(ZenithAI)
include(ZenithCommands)
include(ZenithEffects)
if(ZENITH_BUILD_UI_UNIFIED)
    include(ZenithUIUnified)
endif()
include(ZenithNetwork)

# Removed conflicting ZenithTesting include
# include(ZenithTesting)

# Add help target for user-friendly information (RENAMED to zenith_help)
add_custom_target(zenith_help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "🎵 Zenith DAW Build Information"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Configuration options:"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DCMAKE_BUILD_TYPE=Release/Debug/RelWithDebInfo"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DBUILD_TESTS=ON/OFF"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DZENITH_ENABLE_SKIA=ON/OFF"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DZENITH_ENABLE_COVERAGE=ON/OFF"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DZENITH_ENABLE_WCET=ON/OFF"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DZENITH_FETCH_DEPS=ON/OFF"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DZENITH_ENABLE_DEBUG_SYMBOLS=ON/OFF"
    COMMAND ${CMAKE_COMMAND} -ECHO "  -DZENITH_ENABLE_ASSERTIONS=ON/OFF"
    COMMAND ${CMAKE_COMMAND} -ECHO ""
    COMMAND print_build_info
    COMMAND print_build_targets
    COMMAND print_build_commands
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "For more information, see: BUILDING.md"
    VERBATIM
)

# Add rebuild target
add_custom_target(rebuild
    COMMAND ${CMAKE_COMMAND} -E echo "🔄 Cleaning and rebuilding..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} --build .
    VERBATIM
)

# =============================================================================
# MAIN APPLICATION
# =============================================================================

if(ZENITH_BUILD_APP)
    # Create main executable
    add_executable(ZenithDAW
        apps/desktop/Source/Main.cpp
        apps/desktop/Source/ui/common/MainWindow.cpp
        ${ZENITH_AGENTS_SOURCES}
    )

    # Add debug symbols if requested
    if(ZENITH_ENABLE_DEBUG_SYMBOLS)
        set_target_properties(ZenithDAW PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
    endif()

    # Set include directories
    target_include_directories(ZenithDAW
        PRIVATE
            ${CMAKE_SOURCE_DIR}/apps/desktop/Source
            ${CMAKE_SOURCE_DIR}/modules
    )

    # Link libraries
    target_link_libraries(ZenithDAW
        zenith_commands
        zenith_effects
        zenith_ai
        zenith_network
        zenith_engine_core
        zenith_core
        zenith_audio_utils
        ${ZENITH_JUCE_MODULES}
    )

    if(ZENITH_BUILD_UI_UNIFIED)
        target_link_libraries(ZenithDAW
            zenith_ui_unified
        )
    endif()

    # Platform-specific linking
    if(APPLE)
        target_link_libraries(ZenithDAW
            "-framework Cocoa -framework IOKit -framework CoreAudio -framework CoreMIDI"
        )
        message(STATUS "🍎 macOS: linking Cocoa, IOKit, CoreAudio, CoreMIDI")
    elseif(UNIX)
        target_link_libraries(ZenithDAW
            "-lpthread -lasound -ldl"
        )
        message(STATUS "🐧 Linux: linking pthread, alsa, dl")
    elseif(WIN32)
        target_link_libraries(ZenithDAW
            "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib"
            "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib"
            "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib"
        )
        message(STATUS "🪟 Windows: linking Windows libraries")
    endif()

    # Installation
    install(TARGETS ZenithDAW
        RUNTIME DESTINATION bin
    )

    message(STATUS "✅ Main application target created: ZenithDAW")
endif()

# =============================================================================
# TESTS
# =============================================================================

if(ZENITH_BUILD_TESTS)
    enable_testing()

    # Add test executable
    add_executable(ZenithTests
        apps/desktop/Source/tests/TestMain.cpp
        ${ZENITH_TEST_SOURCES}
    )

    target_link_libraries(ZenithTests
        zenith_network
        zenith_core
        zenith_audio_utils
        ${ZENITH_JUCE_MODULES}
        GTest::gtest
        GTest::gtest_main
    )

    if(ZENITH_BUILD_UI_UNIFIED)
        target_link_libraries(ZenithTests
            zenith_ui_unified
        )
    endif()

    # Add tests
    add_test(NAME ZenithCoreTests COMMAND ZenithTests)
    add_test(NAME ZenithUITests COMMAND ZenithTests)
    add_test(NAME ZenithEngineTests COMMAND ZenithTests)

    # Add test target (renamed to run_tests to avoid conflict)
    add_custom_target(run_tests
        COMMAND ctest --output-on-failure
        VERBATIM
    )

    message(STATUS "✅ Test suite target created: ZenithTests")
endif()

# =============================================================================
# PACKAGE TARGET
# =============================================================================

# Create package target for distribution
add_custom_target(package
    COMMAND ${CMAKE_COMMAND} -E echo "📦 Creating packages..."
    COMMAND ${CMAKE_COMMAND} --build . --config Release
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/Release ${CMAKE_BINARY_DIR}/package/ZenithDAW
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_BINARY_DIR}/package/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_BINARY_DIR}/package/
    COMMAND ${CMAKE_COMMAND} -E tar -czf ${CMAKE_BINARY_DIR}/ZenithDAW-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz -C ${CMAKE_BINARY_DIR}/package .
    VERBATIM
)

# =============================================================================
# FINAL SUMMARY
# =============================================================================

# Print build information
print_build_info()
print_build_targets()

# Final welcome message
message(STATUS "")
message(STATUS "🎉 Zenith DAW configuration complete!")
message(STATUS "   Use 'make help' for more information")
message(STATUS "")
message(STATUS "Happy building! 🎵")
