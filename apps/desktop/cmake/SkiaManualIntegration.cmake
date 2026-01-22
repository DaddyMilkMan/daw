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
            set(SKIA_ROOT "${CMAKE_SOURCE_DIR}/external/vcpkg/installed/x64-linux")
        endif()
        message(STATUS "  SKIA_ROOT: Using platform default fallback (vcpkg)")
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
    target_include_directories(ZenithDAW PRIVATE ${SKIA_ROOT}/include ${SKIA_INCLUDE_DIR})
    
    if(TARGET ZenithDAWTests)
        target_include_directories(ZenithDAWTests PRIVATE ${SKIA_ROOT}/include ${SKIA_INCLUDE_DIR})
    endif()
else()
    message(WARNING "Skia headers not found at ${SKIA_INCLUDE_DIR} - Skia integration disabled")
    set(ZENITH_ENABLE_SKIA OFF CACHE BOOL "Enable Skia" FORCE)
endif()

# Check for library (Static or Shared)
find_library(SKIA_LIBRARY NAMES skia skia.dll skia.so skia.dylib PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
if(SKIA_LIBRARY)
    message(STATUS "  Skia library: ${SKIA_LIBRARY}")
    target_link_libraries(ZenithDAW PRIVATE ${SKIA_LIBRARY})
    if(TARGET ZenithDAWTests)
        target_link_libraries(ZenithDAWTests PRIVATE ${SKIA_LIBRARY})
    endif()
    message(STATUS "  Skia graphics library: LINKED (Manual)")
else()
    message(WARNING "Skia library not found in ${SKIA_LIB_DIR} - Skia integration disabled")
    set(ZENITH_ENABLE_SKIA OFF CACHE BOOL "Enable Skia" FORCE)
endif()

if(UNIX AND NOT APPLE)
    # EGL is required for Skia GL backend on Linux (Wayland/X11)
    find_library(EGL_LIBRARY NAMES EGL libEGL)
    find_library(GLESV2_LIBRARY NAMES GLESv2 libGLESv2)
    
    if(EGL_LIBRARY)
        message(STATUS "  Linking EGL: ${EGL_LIBRARY}")
        target_link_libraries(ZenithDAW PRIVATE ${EGL_LIBRARY})
        if(TARGET ZenithDAWTests)
            target_link_libraries(ZenithDAWTests PRIVATE ${EGL_LIBRARY})
        endif()
    else()
        message(WARNING "EGL library not found - Skia GL backend may fail to link metho eglGetProcAddress")
    endif()
    
    if(GLESV2_LIBRARY)
         target_link_libraries(ZenithDAW PRIVATE ${GLESV2_LIBRARY})
         if(TARGET ZenithDAWTests)
            target_link_libraries(ZenithDAWTests PRIVATE ${GLESV2_LIBRARY})
        endif()
    endif()
endif()

# Auto-copy Shared Libraries (DLL/SO/DYLIB)
file(GLOB SKIA_SHARED_LIBS 
    "${SKIA_BIN_DIR}/*.dll" 
    "${SKIA_LIB_DIR}/*.so*" 
    "${SKIA_LIB_DIR}/*.dylib"
)

foreach(LIB_FILE ${SKIA_SHARED_LIBS})
    get_filename_component(LIB_NAME ${LIB_FILE} NAME)
    message(STATUS "  Found dependency: ${LIB_FILE} - Configuring auto-copy")
    
    add_custom_command(TARGET ZenithDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIB_FILE}"
        "$<TARGET_FILE_DIR:ZenithDAW>/${LIB_NAME}"
    )
    
    if(TARGET ZenithDAWTests)
        add_custom_command(TARGET ZenithDAWTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIB_FILE}"
            "$<TARGET_FILE_DIR:ZenithDAWTests>/${LIB_NAME}"
        )
    endif()
endforeach()

# Skia UI source files are now included from apps/desktop/Source/ui/CMakeLists.txt
# via the domain-based organization (framework/, widgets/, etc.)
message(STATUS "  Skia UI Components: Managed by ui/CMakeLists.txt")

target_include_directories(ZenithDAW PRIVATE
    apps/desktop/Source/ui/framework
)

target_compile_definitions(ZenithDAW PRIVATE ZENITH_USE_SKIA=1)

message(STATUS "  Skia UI components: ENABLED")
message(STATUS "============================================")