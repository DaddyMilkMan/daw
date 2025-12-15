cmake_minimum_required(VERSION 3.25)
project(ZenithDAW VERSION 0.1.0 LANGUAGES CXX C)

include(apps/desktop/cmake/BuildOptimizations.cmake)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS /bigobj" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS /bigobj" CACHE STRING "" FORCE)
endif()

option(ENABLE_IPO "Enable Interprocedural Optimization (LTO)" OFF)
option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
option(ENABLE_SPECTRE "Enable Spectre Mitigations (MSVC)" OFF)
option(ZENITH_ENABLE_SKIA "Enable Skia Rendering" ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.0)
    FetchContent_MakeAvailable(JUCE)
endif()

set(ZENITH_SOURCES
    apps/desktop/Source/Main.cpp
    apps/desktop/Source/utils/SampleGenerator.cpp
)

set(ZENITH_TEST_SOURCES
    apps/desktop/Source/tests/TestMain.cpp
    apps/desktop/Source/tests/AudioEngineTests.cpp
    apps/desktop/Source/tests/ProjectStateTests.cpp
    apps/desktop/Source/tests/EngineTests.cpp
    apps/desktop/Source/tests/ConcurrencyTests.cpp
    apps/desktop/Source/tests/AITests.cpp
    apps/desktop/Source/tests/UITests.cpp
    apps/desktop/Source/tests/AccessibilityTests.cpp
)

# Create main executable
juce_add_gui_app(ZenithDAW
    PRODUCT_NAME "Zenith DAW"
    COMPANY_NAME "Zenith Audio"
    VERSION ${PROJECT_VERSION}
    BUNDLE_ID com.zenithaudio.daw
)



# Generate JuceHeader.h for compatibility
juce_generate_juce_header(ZenithDAW)

target_sources(ZenithDAW PRIVATE ${ZENITH_SOURCES})

# Skia Integration (must be defined before subdirectories)
option(ZENITH_ENABLE_SKIA "Enable Skia Rendering" ON)

# Modularized Source Groups (Complaint #6 Fix)
add_subdirectory(apps/desktop/Source/engine)
add_subdirectory(apps/desktop/Source/ui)
add_subdirectory(apps/desktop/Source/instruments)
add_subdirectory(apps/desktop/Source/network)
add_subdirectory(apps/desktop/Source/commands)
add_subdirectory(apps/desktop/Source/rendering)
add_subdirectory(apps/desktop/Source/dsp)
add_subdirectory(apps/desktop/Source/browser)
add_subdirectory(apps/desktop/Source/ai)


# Include directories (updated for new domain-based structure)
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
    apps/desktop/Source/ui/browser
    apps/desktop/Source/ui/transport
    apps/desktop/Source/ui/common
    apps/desktop/Source/ui/widgets
    apps/desktop/Source/ui/session
    apps/desktop/Source/ui/instruments
    apps/desktop/Source/ui/dialogs
    apps/desktop/Source/ui/sample-editor
    ${CMAKE_PREFIX_PATH}/include
    "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules"
)

target_link_libraries(ZenithDAW PRIVATE
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

# Platform specific (Public so App and Tests inherit)
if(WIN32)
    target_link_libraries(ZenithDAW PRIVATE d3d12 dxgi dcomp uuid)
elseif(APPLE)
    target_link_libraries(ZenithDAW PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
elseif(UNIX AND NOT APPLE)
    find_package(Vulkan)
    if(Vulkan_FOUND)
        target_link_libraries(ZenithDAW PRIVATE Vulkan::Vulkan)
    else()
        target_link_libraries(ZenithDAW PRIVATE vulkan)
    endif()
endif()



enable_zenith_build_optimizations(ZenithDAW)
include(apps/desktop/cmake/PrecompiledHeaders.cmake)
target_compile_definitions(ZenithDAW PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_compile_features(ZenithDAW PRIVATE cxx_std_20)
set_target_properties(ZenithDAW PROPERTIES CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    juce_add_console_app(ZenithDAWTests PRODUCT_NAME "Zenith DAW Tests")
    juce_generate_juce_header(ZenithDAWTests)
    target_sources(ZenithDAWTests PRIVATE ${ZENITH_TEST_SOURCES})
    target_include_directories(ZenithDAWTests PRIVATE 
        apps/desktop/Source 
        apps/desktop/include
        "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules"
    )
    target_link_libraries(ZenithDAWTests PRIVATE ZenithDAW_lib)
    target_compile_definitions(ZenithDAWTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
endif()

install(TARGETS ZenithDAW BUNDLE DESTINATION . RUNTIME DESTINATION bin)
add_custom_command(TARGET ZenithDAW POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/apps/desktop/Resources/fonts $<TARGET_FILE_DIR:ZenithDAW>/Resources/fonts)

# Skia Integration (Placed at end to ensure all targets exist)
if(ZENITH_ENABLE_SKIA)
    include(apps/desktop/cmake/SkiaManualIntegration.cmake)
    target_compile_definitions(ZenithDAW_lib PUBLIC ZENITH_USE_SKIA=1)
endif()

enable_testing()
if(BUILD_TESTS)
    add_test(NAME ZenithDAWTests COMMAND ZenithDAWTests)
endif()
