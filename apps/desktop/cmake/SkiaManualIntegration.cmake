# ============================================================================
# Skia Integration (Audited)
# ============================================================================
# Manual integration strategy for Zenith DAW.
# Scans vcpkg standard paths for Skia headers and libraries.
# ============================================================================

message(STATUS "============================================")
message(STATUS "Configuring Skia Integration (Audited)")
message(STATUS "============================================")

set(SKIA_ROOT "C:/vcpkg/installed/x64-windows")
set(SKIA_INCLUDE_DIR "${SKIA_ROOT}/include/skia")
set(SKIA_LIB_DIR "${SKIA_ROOT}/lib")
set(SKIA_BIN_DIR "${SKIA_ROOT}/bin")

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