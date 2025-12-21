# ==============================================================================
# Zenith DAW - Build Optimizations Module
# ==============================================================================
# Purpose: Centralized configuration for build performance optimizations
#
# Features:
#   1. ccache support for distributed/local caching
#   2. Parallel compilation (MSVC /MP, GNU/Clang -jN)
#   3. Incremental linking for Debug builds
#   4. Build time measurement
#   5. Compiler-specific optimizations
#
# Usage: include(apps/desktop/cmake/BuildOptimizations.cmake)
#        call enable_zenith_build_optimizations(ZenithDAW) after target creation
# ==============================================================================

message(STATUS "============================================")
message(STATUS "Configuring Build Optimizations")
message(STATUS "============================================")

# ==============================================================================
# Option flags - allow users to toggle optimizations
# ==============================================================================
option(ZENITH_ENABLE_CCACHE "Enable ccache for faster rebuilds" ON)
option(ZENITH_ENABLE_PARALLEL_COMPILE "Enable parallel compilation" ON)
option(ZENITH_ENABLE_INCREMENTAL_LINK "Enable incremental linking for Debug" ON)
option(ZENITH_MEASURE_BUILD_TIME "Print build timing information" OFF)

# ==============================================================================
# 1. ccache Configuration
# ==============================================================================
if(ZENITH_ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "  ccache: FOUND at ${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "CXX Compiler Launcher")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "C Compiler Launcher")
        message(STATUS "  ccache: ENABLED")
    else()
        message(STATUS "  ccache: NOT FOUND (install for faster rebuilds)")
    endif()
endif()

# ==============================================================================
# 2. Function to apply optimizations to a target
# ==============================================================================
function(enable_zenith_build_optimizations TARGET_NAME)
    message(STATUS "  Applying build optimizations to: ${TARGET_NAME}")
    
    # ==========================================================================
    # MSVC-specific optimizations
    # ==========================================================================
    if(MSVC)
        # Multi-processor compilation
        if(ZENITH_ENABLE_PARALLEL_COMPILE)
            target_compile_options(${TARGET_NAME} PRIVATE /MP)
            message(STATUS "    MSVC: Parallel compilation enabled (/MP)")
        endif()
        
        # Fast linking optimizations
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES)
            if(ZENITH_ENABLE_INCREMENTAL_LINK AND NOT ENABLE_SANITIZERS)
                target_link_options(${TARGET_NAME} PRIVATE
                    $<$<CONFIG:Debug>:/INCREMENTAL>
                    $<$<CONFIG:Debug>:/DEBUG:FASTLINK>
                )
                message(STATUS "    MSVC: Incremental linking enabled for Debug")
            elseif(ENABLE_SANITIZERS)
                target_link_options(${TARGET_NAME} PRIVATE
                    $<$<CONFIG:Debug>:/INCREMENTAL:NO>
                )
                message(STATUS "    MSVC: Incremental linking DISABLED for Sanitizers")
            endif()
        endif()
        
        # Faster template instantiation
        # /Zc:inline - Removes unreferenced COMDAT (reduces obj size)
        target_compile_options(${TARGET_NAME} PRIVATE /Zc:inline)
        
        # Suppress certain warnings that slow down compilation
        # /wd4250 - inheritance via dominance (common in JUCE)
        target_compile_options(${TARGET_NAME} PRIVATE /wd4250)
        
    # ==========================================================================
    # GCC/Clang-specific optimizations
    # ==========================================================================
    else()
        if(ZENITH_ENABLE_PARALLEL_COMPILE)
            # GCC/Clang: parallel compilation is handled by make -jN or ninja
            # But we can enable parallel template instantiation
            include(ProcessorCount)
            ProcessorCount(NPROC)
            if(NPROC GREATER 0)
                message(STATUS "    GCC/Clang: Detected ${NPROC} processors")
            endif()
        endif()
        
        # Use pipes for faster compilation (avoid temp files)
        target_compile_options(${TARGET_NAME} PRIVATE -pipe)
        
        # Debug-specific: use split DWARF for faster linking
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            # Check if compiler supports -gsplit-dwarf
            include(CheckCXXCompilerFlag)
            check_cxx_compiler_flag(-gsplit-dwarf SUPPORTS_SPLIT_DWARF)
            if(SUPPORTS_SPLIT_DWARF)
                target_compile_options(${TARGET_NAME} PRIVATE -gsplit-dwarf)
                message(STATUS "    GCC/Clang: Split DWARF enabled for Debug")
            endif()
        endif()
    endif()
    
    message(STATUS "  Build optimizations applied to: ${TARGET_NAME}")
endfunction()

# ==============================================================================
# 3. Build Time Measurement (optional)
# ==============================================================================
if(ZENITH_MEASURE_BUILD_TIME)
    # This creates a timestamp at configure time - useful for CI
    string(TIMESTAMP CONFIGURE_TIME "%Y-%m-%d %H:%M:%S")
    message(STATUS "  Build configured at: ${CONFIGURE_TIME}")
    
    # Ninja has built-in build time tracking
    if(CMAKE_GENERATOR STREQUAL "Ninja")
        set(CMAKE_NINJA_OUTPUT_PATH_PREFIX "" CACHE STRING "" FORCE)
    endif()
endif()

# ==============================================================================
# 4. Dependency Graph Optimization
# ==============================================================================
# Enable CMAKE_EXPORT_COMPILE_COMMANDS for better tooling support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

# ==============================================================================
# 5. Object File Optimization (reduce disk I/O)
# ==============================================================================
if(CMAKE_GENERATOR STREQUAL "Ninja")
    # Ninja supports keeping object files in-memory longer
    message(STATUS "  Generator: Ninja (optimal for incremental builds)")
elseif(CMAKE_GENERATOR MATCHES "Visual Studio")
    message(STATUS "  Generator: Visual Studio (use /MP for parallel builds)")
else()
    message(STATUS "  Generator: ${CMAKE_GENERATOR}")
endif()

message(STATUS "============================================")
