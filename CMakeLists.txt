cmake_minimum_required(VERSION 3.25)

# 1. Project Definition
project(ZenithDAW 
    VERSION 0.1.0
    LANGUAGES CXX C
)

# 2. Build Performance: ccache
# find_program(CCACHE_PROGRAM ccache)
# if(CCACHE_PROGRAM)
#     message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
#     set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
# else()
#     message(STATUS "ccache not found. Install it for faster builds.")
# endif()

# 3. Default Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()



# 4. Options
option(ENABLE_IPO "Enable Interprocedural Optimization (LTO)" OFF)
option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
option(ENABLE_SPECTRE "Enable Spectre Mitigations (MSVC)" OFF)

# 5. C++ Standard (Enforced later via target_compile_features)
# set(CMAKE_CXX_STANDARD 20) - Removed in favor of target properties
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always enable Skia (Phase 2 decision)
# add_definitions(-DZENITH_USE_SKIA=1) - Removed: Handled via target_compile_definitions in SkiaManualIntegration.cmake

# JUCE Setup
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0
)
FetchContent_MakeAvailable(JUCE)

# Find Skia (from vcpkg)
# find_package(unofficial-skia CONFIG REQUIRED) - Using Manual Integration

# Source files
set(ZENITH_SOURCES
    apps/desktop/Source/Main.cpp
)

# Test sources (optional, can be disabled)
set(ZENITH_TEST_SOURCES
    apps/desktop/Source/tests/AudioEngineTests.cpp
    apps/desktop/Source/tests/ProjectStateTests.cpp
    apps/desktop/Source/tests/EngineTests.cpp
    apps/desktop/Source/tests/ConcurrencyTests.cpp
    apps/desktop/Source/tests/AITests.cpp
    apps/desktop/Source/tests/UITests.cpp
    apps/desktop/Source/ui/skia/ZenithLayout.cpp
    # apps/desktop/Source/tests/FlecsSyncTests.cpp - Removed
    # Dependencies for tests
    # apps/desktop/Source/network/GrokUtils.cpp
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

# Modularized Source Groups (Complaint #6 Fix)
add_subdirectory(apps/desktop/Source/engine)
add_subdirectory(apps/desktop/Source/ui)
add_subdirectory(apps/desktop/Source/instruments)
add_subdirectory(apps/desktop/Source/network)
add_subdirectory(apps/desktop/Source/commands)
add_subdirectory(apps/desktop/Source/rendering)
add_subdirectory(apps/desktop/Source/dsp)
add_subdirectory(apps/desktop/Source/browser)


# Include directories
target_include_directories(ZenithDAW PRIVATE
    apps/desktop/include
    apps/desktop/Source
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_BINARY_DIR}/JuceLibraryCode
)

# Link JUCE modules
target_link_libraries(ZenithDAW PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
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

# Platform-specific libraries for Graphics Backends
if(WIN32)
    target_link_libraries(ZenithDAW PRIVATE d3d12 dxgi dcomp uuid)
elseif(APPLE)
    target_link_libraries(ZenithDAW PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
elseif(UNIX AND NOT APPLE)
    # For Vulkan on Linux
    find_package(Vulkan)
    if(Vulkan_FOUND)
        target_link_libraries(ZenithDAW PRIVATE Vulkan::Vulkan)
    else()
        message(STATUS "Vulkan not found - Skia Vulkan backend might fail at runtime")
        target_link_libraries(ZenithDAW PRIVATE vulkan) # Try explicit link
    endif()
endif()

# Skia Integration (Manual)
option(ZENITH_ENABLE_SKIA "Enable Skia Rendering" ON)

if(ZENITH_ENABLE_SKIA)
    include(apps/desktop/cmake/SkiaManualIntegration.cmake)
    target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)
    message(STATUS "Zenith DAW: Skia rendering ENABLED")
else()
    message(STATUS "Zenith DAW: Skia rendering DISABLED")
endif()

# Compile definitions
target_compile_definitions(ZenithDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# Removed commented-out MSVC calling convention code (2025-01-XX)
# If needed in future, see git history for previous implementation

# ==============================================================================
# MODERN COMPILER CONFIGURATION (Standard Flags)

# 6. Modern C++ Standards & Conformance
target_compile_features(ZenithDAW PRIVATE cxx_std_20)
set_target_properties(ZenithDAW PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    # Enforce specific runtime library to prevent mismatches
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# 7. Interprocedural Optimization (LTO)
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

# 8. Standard Compiler Flags & Hardening
# if(MSVC)
#     target_compile_options(ZenithDAW PRIVATE
#         /W4 /permissive- 
#         /wd4100 # Unused parameter
#         
#         # Conformance
#         /Zc:__cplusplus 
#         /Zc:inline      
#         /Zc:preprocessor 
#         /Zc:throwingNew 
#         
#         # Hardening
#         $<$<BOOL:${ENABLE_HARDENING}>:/GS>       
#         $<$<BOOL:${ENABLE_HARDENING}>:/Guard:CF> 
#         $<$<BOOL:${ENABLE_HARDENING}>:/SDL>      
#         $<$<BOOL:${ENABLE_SPECTRE}>:/Qspectre>   
#         
#         # Ruthless Warnings
#         /w14242 /w14254 /w14263 
#         /w14265 
#         /w14287 
#         /we4289 
#         /w14296 
#         /w14311 
#         /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
#     )
    
#     if(ENABLE_HARDENING)
#         target_link_options(ZenithDAW PRIVATE 
#             /DYNAMICBASE 
#             /NXCOMPAT    
#             /CETCOMPAT   
#         )
#     endif()

# else() # GCC / Clang
#     target_compile_options(ZenithDAW PRIVATE
#         -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-align -Wformat=2
#         -Wunused -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough
        
#         $<$<BOOL:${ENABLE_HARDENING}>:-fstack-protector-strong>
#         $<$<BOOL:${ENABLE_HARDENING}>:-fPIE>
#         $<$<BOOL:${ENABLE_HARDENING}>:-U_FORTIFY_SOURCE>
#         $<$<BOOL:${ENABLE_HARDENING}>:-D_FORTIFY_SOURCE=2>
#     )

#     if(ENABLE_HARDENING)
#         target_link_options(ZenithDAW PRIVATE 
#             -Wl,-z,relro 
#             -Wl,-z,now 
#             -pie
#         )
#     endif()
# endif()

# 9. Sanitizers
if(ENABLE_SANITIZERS)
    if(MSVC)
        target_compile_options(ZenithDAW PRIVATE /fsanitize=address)
    else()
        target_compile_options(ZenithDAW PRIVATE -fsanitize=address -fsanitize=undefined)
        target_link_options(ZenithDAW PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()
    message(STATUS "Sanitizers enabled (Address, UB)")
endif()

# Optional: Test executable
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    juce_add_console_app(ZenithDAWTests
        PRODUCT_NAME "Zenith DAW Tests"
    )
    
    target_sources(ZenithDAWTests PRIVATE ${ZENITH_TEST_SOURCES})
    target_include_directories(ZenithDAWTests PRIVATE apps/desktop/Source apps/desktop/include)
    target_link_libraries(ZenithDAWTests PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_graphics
        juce::juce_gui_basics
    )
endif()

# Installation
install(TARGETS ZenithDAW
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)




get_target_property(OPTS ZenithDAW COMPILE_OPTIONS)
message(STATUS "ZenithDAW COMPILE_OPTIONS: ${OPTS}")
get_target_property(INT_OPTS ZenithDAW INTERFACE_COMPILE_OPTIONS)
message(STATUS "ZenithDAW INTERFACE_COMPILE_OPTIONS: ${INT_OPTS}")
get_target_property(LIBS ZenithDAW LINK_LIBRARIES)
foreach(LIB ${LIBS})
    if(TARGET ${LIB})
        get_target_property(LIB_OPTS ${LIB} INTERFACE_COMPILE_OPTIONS)
        message(STATUS "${LIB} INTERFACE_COMPILE_OPTIONS: ${LIB_OPTS}")
    endif()
endforeach()
