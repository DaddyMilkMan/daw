# ============================================================================
# Skia Graphics Library Integration
# ============================================================================
#
# This module integrates Skia graphics library for GPU-accelerated rendering.
#
# USAGE:
#
# Option 1: Install via vcpkg (RECOMMENDED)
#   vcpkg install skia:x64-windows
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DZENITH_ENABLE_SKIA=ON
#
# Option 2: Provide Skia path manually
#   cmake .. -DSKIA_DIR=C:/path/to/skia -DZENITH_ENABLE_SKIA=ON
#
# Option 3: Build without Skia (default)
#   cmake ..
#
# ============================================================================

message(STATUS "Configuring Skia graphics library...")

# Option to enable Skia
option(ZENITH_ENABLE_SKIA "Enable Skia GPU rendering" OFF)

if(NOT ZENITH_ENABLE_SKIA)
    message(STATUS "Skia rendering is DISABLED")
    message(STATUS "  To enable: cmake .. -DZENITH_ENABLE_SKIA=ON")
    message(STATUS "  See BUILD_SKIA.md for installation instructions")

    # Create dummy target
    add_library(Skia::Skia INTERFACE IMPORTED)
    return()
endif()

message(STATUS "Skia rendering is ENABLED")

# Try to find Skia
if(DEFINED SKIA_DIR)
    # User provided SKIA_DIR
    message(STATUS "Using SKIA_DIR: ${SKIA_DIR}")
    set(SKIA_INCLUDE_DIR "${SKIA_DIR}/include")
    set(SKIA_LIBRARY_DIR "${SKIA_DIR}/out/Release-x64")

    if(NOT EXISTS "${SKIA_INCLUDE_DIR}")
        message(FATAL_ERROR "Skia include directory not found: ${SKIA_INCLUDE_DIR}")
    endif()

else()
    # Try to find via find_package (works with vcpkg)
    # Note: vcpkg uses unofficial-skia package config
    find_package(unofficial-skia CONFIG QUIET)

    if(NOT unofficial-skia_FOUND)
        # Try old package name for backward compatibility
        find_package(Skia QUIET)
    endif()

    if(NOT unofficial-skia_FOUND AND NOT Skia_FOUND)
        message(FATAL_ERROR
            "Skia not found! Please either:\n"
            "  1. Install via vcpkg: vcpkg install skia:x64-windows\n"
            "  2. Provide SKIA_DIR: cmake .. -DSKIA_DIR=C:/path/to/skia\n"
            "  3. Build without Skia: cmake .. (removes -DZENITH_ENABLE_SKIA=ON)\n"
            "See BUILD_SKIA.md for detailed instructions"
        )
    endif()

    if(unofficial-skia_FOUND)
        message(STATUS "Found Skia via find_package(unofficial-skia CONFIG)")
    else()
        message(STATUS "Found Skia via find_package(Skia)")
    endif()
endif()

# Create or use Skia interface library
if(unofficial-skia_FOUND)
    # Create an alias to vcpkg's unofficial-skia target
    message(STATUS "Creating Skia::Skia alias for unofficial::skia::skia target")
    add_library(Skia::Skia INTERFACE IMPORTED)
    target_link_libraries(Skia::Skia INTERFACE unofficial::skia::skia)
else()
    # Create manual interface library
    add_library(Skia::Skia INTERFACE IMPORTED)
endif()

# Configure manual Skia target if not using vcpkg
if(NOT unofficial-skia_FOUND)
    # Set include directories (adjust paths as needed for your Skia installation)
    if(DEFINED SKIA_INCLUDE_DIR)
        target_include_directories(Skia::Skia INTERFACE
            ${SKIA_INCLUDE_DIR}
            ${SKIA_INCLUDE_DIR}/core
            ${SKIA_INCLUDE_DIR}/gpu
            ${SKIA_INCLUDE_DIR}/effects
            ${SKIA_INCLUDE_DIR}/utils
        )
    endif()

    # Link libraries based on platform
    if(WIN32)
        # Windows: Link D3D12 and DXGI
        target_link_libraries(Skia::Skia INTERFACE
            d3d12.lib
            dxgi.lib
            d3dcompiler.lib
        )

        # If SKIA_LIBRARY_DIR is set, link Skia library
        if(DEFINED SKIA_LIBRARY_DIR AND EXISTS "${SKIA_LIBRARY_DIR}/skia.lib")
            target_link_libraries(Skia::Skia INTERFACE
                ${SKIA_LIBRARY_DIR}/skia.lib
            )
            message(STATUS "Linking Skia library: ${SKIA_LIBRARY_DIR}/skia.lib")
        endif()

    elseif(APPLE)
        # macOS: Link Metal frameworks
        target_link_libraries(Skia::Skia INTERFACE
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
        )

    elseif(UNIX)
        # Linux: Link Vulkan
        target_link_libraries(Skia::Skia INTERFACE
            pthread
            dl
        )
    endif()

    # Compiler definitions
    target_compile_definitions(Skia::Skia INTERFACE
        SK_GANESH  # Use Ganesh GPU backend
        $<$<PLATFORM_ID:Windows>:SK_D3D>
        $<$<PLATFORM_ID:Darwin>:SK_METAL>
        $<$<PLATFORM_ID:Linux>:SK_VULKAN>
    )

    # C++17 required by Skia
    target_compile_features(Skia::Skia INTERFACE cxx_std_17)
else()
    # vcpkg unofficial-skia already configures everything needed
    # Just ensure C++17 is used
    if(TARGET unofficial::skia::skia)
        message(STATUS "Skia target: unofficial::skia::skia")
    endif()
endif()

message(STATUS "Skia integration configured successfully!")
message(STATUS "  Backend: $<$<PLATFORM_ID:Windows>:Direct3D>$<$<PLATFORM_ID:Darwin>:Metal>$<$<PLATFORM_ID:Linux>:Vulkan>")

