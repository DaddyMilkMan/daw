# ============================================================================
# Robust Skia Integration (Audited)
# ============================================================================
# 1. Tries standard vcpkg integration (unofficial-skia)
# 2. Falls back to manual search with configurable paths
# 3. ONLY enables Skia UI components if the library is actually found.
# ============================================================================

message(STATUS "============================================")
message(STATUS "Configuring Skia Integration (Audited)")
message(STATUS "============================================")

set(SKIA_FOUND_AND_READY OFF)

# ------------------------------------------------------------------------
# 1. Graphics Library Discovery
# ------------------------------------------------------------------------

# A. Try Standard vcpkg package (unofficial-skia)
# find_package(unofficial-skia CONFIG QUIET)

if(unofficial-skia_FOUND)
    message(STATUS "  Strategy: vcpkg (unofficial-skia)")
    target_link_libraries(ZenithDAW PRIVATE unofficial::skia::skia)
    target_compile_definitions(ZenithDAW PRIVATE SK_GL=1)
    set(SKIA_FOUND_AND_READY ON)
    message(STATUS "  Skia graphics library: LINKED (unofficial::skia::skia)")
    
else()
    # B. Manual Search Fallback
    message(STATUS "  Strategy: Manual Search (unofficial-skia not found)")
    
    # Smart default for SKIA_DIR
    if(DEFINED ENV{VCPKG_ROOT})
        set(DEFAULT_SKIA_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows")
    else()
        set(DEFAULT_SKIA_DIR "C:/vcpkg/installed/x64-windows")
    endif()

    set(SKIA_DIR "${DEFAULT_SKIA_DIR}" CACHE PATH "Path to Skia installation")

    find_path(SKIA_INCLUDE_DIR
        NAMES include/core/SkCanvas.h
        PATHS ${SKIA_DIR}/include/skia
        NO_DEFAULT_PATH
    )

    find_library(SKIA_LIBRARY
        NAMES skia.dll skia
        PATHS ${SKIA_DIR}/lib ${SKIA_DIR}/bin
        NO_DEFAULT_PATH
    )

    if(SKIA_INCLUDE_DIR AND SKIA_LIBRARY)
        message(STATUS "  Skia headers: ${SKIA_INCLUDE_DIR}")
        message(STATUS "  Skia library: ${SKIA_LIBRARY}")

        target_include_directories(ZenithDAW PRIVATE
            ${SKIA_INCLUDE_DIR}
            ${SKIA_DIR}/include
            ${SKIA_DIR}/include/skia
            ${SKIA_DIR}/include/skia/include
            ${SKIA_DIR}
        )

        target_link_libraries(ZenithDAW PRIVATE ${SKIA_LIBRARY})
        target_compile_definitions(ZenithDAW PRIVATE SK_GL=1)
        set(SKIA_FOUND_AND_READY ON)
        message(STATUS "  Skia graphics library: LINKED (Manual)")

        # ------------------------------------------------------------------------
        # AUTOMATIC DLL DEPLOYMENT (User-Friendly Fix)
        # ------------------------------------------------------------------------
        # ------------------------------------------------------------------------
        # AUTOMATIC DLL DEPLOYMENT (User-Friendly Fix)
        # ------------------------------------------------------------------------
        if(WIN32)
            # Try to locate the DLL associated with the import library
            get_filename_component(SKIA_LIB_DIR "${SKIA_LIBRARY}" DIRECTORY)
            
            # Common dependencies for Skia (vcpkg build)
            # We explicitly look for these because Skia doesn't statically link them in this configuration
            set(SKIA_DEPENDENCY_DLLS 
                skia.dll
                libpng16.dll
                jpeg62.dll
                libwebpdecoder.dll
                libwebpdemux.dll
                libwebpmux.dll
                libwebp.dll
                libsharpyuv.dll
                libexpat.dll
                zlib1.dll
            )

            foreach(DLL_NAME ${SKIA_DEPENDENCY_DLLS})
                find_file(FOUND_${DLL_NAME}
                    NAMES ${DLL_NAME}
                    PATHS 
                        "${SKIA_LIB_DIR}/../bin"
                        "${SKIA_LIB_DIR}"
                        "${SKIA_DIR}/bin"
                    NO_DEFAULT_PATH
                )
                
                if(FOUND_${DLL_NAME})
                    message(STATUS "  Found dependency: ${FOUND_${DLL_NAME}} - Configuring auto-copy")
                    
                    # Copy to build directory (for running from IDE/CLI)
                    add_custom_command(TARGET ZenithDAW POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${FOUND_${DLL_NAME}}"
                        "$<TARGET_FILE_DIR:ZenithDAW>"
                        COMMENT "Deploying ${DLL_NAME}..."
                    )
                    
                    # Copy to install directory (for packaging)
                    install(FILES "${FOUND_${DLL_NAME}}" DESTINATION bin)
                else()
                    # Only warn for skia.dll as others might be statically linked in some builds
                    if("${DLL_NAME}" STREQUAL "skia.dll")
                        message(WARNING "  Could not find skia.dll! Users may need to install it manually.")
                    endif()
                endif()
                
                unset(FOUND_${DLL_NAME} CACHE) 
            endforeach()
        endif()

    else()
        message(WARNING "  Skia graphics library NOT FOUND. Skia UI components will be DISABLED.")
        message(STATUS "  (Hint: Set SKIA_DIR to your vcpkg installation or use the vcpkg toolchain)")
    endif()
endif()

# ------------------------------------------------------------------------
# 2. UI Components (Conditional on Skia Library)
# ------------------------------------------------------------------------

if(SKIA_FOUND_AND_READY)
    message(STATUS "  Enabling Skia UI Components...")
    
    # Define ZENITH_USE_SKIA globally
    target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)

    # Add Skia UI source files
    target_sources(ZenithDAW PRIVATE
        apps/desktop/Source/ui/skia/SkiaComponent.cpp
        apps/desktop/Source/ui/skia/SkiaButton.cpp
        apps/desktop/Source/ui/skia/SkiaKnob.cpp
        apps/desktop/Source/ui/skia/SkiaSlider.cpp
        apps/desktop/Source/ui/skia/SkiaMainWindowIntegration.cpp
        apps/desktop/Source/ui/skia/TransportBar.cpp
        apps/desktop/Source/ui/skia/BottomBar.cpp
        apps/desktop/Source/ui/skia/BrowserPanel.cpp
        apps/desktop/Source/ui/skia/RightSidePanel.cpp
        apps/desktop/Source/ui/skia/ZenithPolySynthUI.cpp
        apps/desktop/Source/ui/skia/ZenithDesignSystem.cpp
        apps/desktop/Source/ui/skia/views/PianoKeyboardViewSkia.cpp
        apps/desktop/Source/ui/skia/ZenithUIComponents.h
    )

    target_include_directories(ZenithDAW PRIVATE
        apps/desktop/Source/ui/skia
    )
    
    message(STATUS "  Skia UI components: ENABLED")
else()
    message(STATUS "  Skia UI components: DISABLED (Fallback to JUCE UI)")
endif()

message(STATUS "============================================")
