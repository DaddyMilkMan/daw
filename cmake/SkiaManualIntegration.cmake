# ============================================================================
# Manual Skia Integration (instead of vcpkg)
# ============================================================================
# This file replaces the vcpkg-based Skia integration in CMakeLists.txt
#
# Usage: Include this after line 195 in zenith-core/CMakeLists.txt
#        (replacing the "if(ZENITH_ENABLE_SKIA)" block)
#
# IMPORTANT: The Skia UI components (TransportBar, BrowserPanel, etc.) use
# JUCE's OpenGL renderer and do NOT require the Skia graphics library.
# They are always included when ZENITH_ENABLE_SKIA=ON.
# ============================================================================

if(ZENITH_ENABLE_SKIA)
    message(STATUS "============================================")
    message(STATUS "Configuring Skia UI Components")
    message(STATUS "============================================")

    # Always add Skia UI source files that exist (raster Skia rendering)
    target_sources(ZenithDAW PRIVATE
        # Skia UI Components (in zenith-core/src/ui/skia/)
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/skia/ZenithUIComponents.h
        # ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/skia/ZenithPolySynthUI.h  # DISABLED: Uses OpenGL+GPU Skia (incomplete)
        # ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/skia/ZenithPolySynthUI.cpp  # DISABLED: Needs conversion to raster Skia
    )

    # Add include directories for Skia UI components
    target_include_directories(ZenithDAW PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/skia
    )

    # Always enable ZENITH_USE_SKIA for UI components
    target_compile_definitions(ZenithDAW PRIVATE
        ZENITH_USE_SKIA=1
    )

    message(STATUS "  Skia UI components: ENABLED")
    message(STATUS "  Rendering backend: JUCE OpenGL")
    message(STATUS "============================================")

endif() # ZENITH_ENABLE_SKIA

# Optional: Try to find actual Skia graphics library (NOT REQUIRED for UI)
if(ZENITH_ENABLE_SKIA)
    # Skia directory (optional) - using vcpkg installation
    set(SKIA_DIR "C:/vcpkg/installed/x64-windows" CACHE PATH "Path to Skia installation")

    if(EXISTS "${SKIA_DIR}")
        message(STATUS "============================================")
        message(STATUS "Skia Graphics Library: FOUND")
        message(STATUS "============================================")

    # Find Skia headers and library (vcpkg structure)
    find_path(SKIA_INCLUDE_DIR
        NAMES include/core/SkCanvas.h
        PATHS ${SKIA_DIR}/include/skia
        NO_DEFAULT_PATH
    )

    # Try multiple possible library locations (vcpkg uses .dll.lib)
    find_library(SKIA_LIBRARY
        NAMES skia.dll skia
        PATHS
            ${SKIA_DIR}/lib
            ${SKIA_DIR}/bin
        NO_DEFAULT_PATH
    )

        if(SKIA_INCLUDE_DIR AND SKIA_LIBRARY)
            message(STATUS "  Skia headers: ${SKIA_INCLUDE_DIR}")
            message(STATUS "  Skia library: ${SKIA_LIBRARY}")

            # Add Skia graphics library support
            # Point to the skia subdirectory so includes work as <include/core/SkCanvas.h>
            target_include_directories(ZenithDAW PRIVATE
                ${SKIA_INCLUDE_DIR}
            )

            # Link Skia library
            target_link_libraries(ZenithDAW PRIVATE ${SKIA_LIBRARY})

            # Enable Skia graphics backend
            target_compile_definitions(ZenithDAW PRIVATE
                SK_GL=1
            )

            message(STATUS "  Skia graphics library: LINKED")
        else()
            message(STATUS "  Skia headers or library not found (skipping graphics library)")
        endif()
    else()
        message(STATUS "Skia graphics library: NOT FOUND (using JUCE OpenGL only)")
        message(STATUS "  Skia UI components will still work with JUCE rendering")
    endif()

endif() # ZENITH_ENABLE_SKIA
