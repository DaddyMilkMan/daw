# ============================================================================
# Skia Integration (Audited - Build Optimized)
# ============================================================================
# Manual integration strategy for Zenith DAW.
# Uses CMake's standard path discovery to find Skia from vcpkg.
#
# Configuration:
#   - SKIA_ROOT: Override via -DSKIA_ROOT=/path/to/skia
#   - Falls back to VCPKG_INSTALLED_DIR or CMAKE_PREFIX_PATH
# ============================================================================

message(STATUS "============================================")
message(STATUS "Configuring Skia Integration (Audited)")
message(STATUS "============================================")

# ==============================================================================
# Auto-detect SKIA_ROOT from vcpkg or allow override
# ==============================================================================
if(NOT DEFINED SKIA_ROOT)
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(SKIA_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
        message(STATUS "  SKIA_ROOT: From VCPKG_INSTALLED_DIR")
    elseif(DEFINED ENV{VCPKG_ROOT})
        if(WIN32)
            set(_triplet "x64-windows")
        elseif(APPLE)
            set(_triplet "x64-osx")
        else()
            set(_triplet "x64-linux")
        endif()
        set(SKIA_ROOT "$ENV{VCPKG_ROOT}/installed/${_triplet}")
        message(STATUS "  SKIA_ROOT: From VCPKG_ROOT environment variable")
    elseif(CMAKE_PREFIX_PATH)
        list(GET CMAKE_PREFIX_PATH 0 SKIA_ROOT)
        message(STATUS "  SKIA_ROOT: From CMAKE_PREFIX_PATH")
    else()
        # Platform-specific fallbacks
        if(WIN32)
            set(SKIA_ROOT "C:/vcpkg/installed/x64-windows")
        elseif(APPLE)
            set(SKIA_ROOT "/usr/local/opt/skia")
        else()
            set(SKIA_ROOT "/usr/local")
        endif()
        message(STATUS "  SKIA_ROOT: Using platform default fallback")
    endif()
endif()

set(SKIA_INCLUDE_DIR "${SKIA_ROOT}/include/skia")
set(SKIA_LIB_DIR "${SKIA_ROOT}/lib")
set(SKIA_BIN_DIR "${SKIA_ROOT}/bin")
message(STATUS "  SKIA_ROOT: ${SKIA_ROOT}")

# Check for headers
if(EXISTS "${SKIA_INCLUDE_DIR}/core/SkCanvas.h")
    message(STATUS "  Strategy: Manual Search (unofficial-skia not found)")
    message(STATUS "  Skia headers: ${SKIA_INCLUDE_DIR}")
    include_directories(${SKIA_ROOT}/include)
    include_directories(${SKIA_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Skia headers not found at ${SKIA_INCLUDE_DIR}")
endif()

# Check for library
find_library(SKIA_LIBRARY NAMES skia skia.dll PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
if(SKIA_LIBRARY)
    message(STATUS "  Skia library: ${SKIA_LIBRARY}")
    target_link_libraries(ZenithDAW PRIVATE ${SKIA_LIBRARY})
    message(STATUS "  Skia graphics library: LINKED (Manual)")
else()
    message(FATAL_ERROR "Skia library not found in ${SKIA_LIB_DIR}")
endif()

# Auto-copy DLLs
file(GLOB SKIA_DLLS "${SKIA_BIN_DIR}/*.dll")
foreach(DLL ${SKIA_DLLS})
    get_filename_component(DLL_NAME ${DLL} NAME)
    message(STATUS "  Found dependency: ${DLL} - Configuring auto-copy")
    add_custom_command(TARGET ZenithDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL}"
        "$<TARGET_FILE_DIR:ZenithDAW>/${DLL_NAME}"
    )
endforeach()

# Add Skia UI source files
message(STATUS "  Enabling Skia UI Components...")
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
    apps/desktop/Source/ui/skia/SkiaSpectrumComponent.cpp
    apps/desktop/Source/ui/skia/ZenithDesignSystem.cpp
    apps/desktop/Source/ui/skia/views/PianoKeyboardViewSkia.cpp
    apps/desktop/Source/ui/skia/ZenithUIComponents.h
)

target_include_directories(ZenithDAW PRIVATE
    apps/desktop/Source/ui/skia
)

target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)

message(STATUS "  Skia UI components: ENABLED")
message(STATUS "============================================")