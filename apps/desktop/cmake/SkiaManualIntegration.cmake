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
    message(FATAL_ERROR "Skia headers not found at ${SKIA_INCLUDE_DIR}")
endif()

# Check for library (Static or Shared)
find_library(SKIA_LIBRARY NAMES skia skia.dll skia.so skia.dylib PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
if(SKIA_LIBRARY)
    message(STATUS "  Skia library: ${SKIA_LIBRARY}")
    target_link_libraries(ZenithDAW PRIVATE ${SKIA_LIBRARY})
    
    # Skia Dependencies (Required for static linking on Linux)
    if(UNIX AND NOT APPLE)
        find_library(PNG_LIBRARY NAMES png16 png PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        find_library(Z_LIBRARY NAMES z zlib PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        find_library(JPEG_LIBRARY NAMES jpeg PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        find_library(WEBP_LIBRARY NAMES webp PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        find_library(FREETYPE_LIBRARY NAMES freetype PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        find_library(FONTCONFIG_LIBRARY NAMES fontconfig PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        find_library(BZ2_LIBRARY NAMES bz2 PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH) 
        
        # Brotli (Try Skia dir first, then system)
        find_library(BROTLIDEC_LIBRARY NAMES brotlidec PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        if(NOT BROTLIDEC_LIBRARY)
            find_library(BROTLIDEC_LIBRARY NAMES brotlidec)
        endif()
        
        find_library(BROTLICOMMON_LIBRARY NAMES brotlicommon PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        if(NOT BROTLICOMMON_LIBRARY)
            find_library(BROTLICOMMON_LIBRARY NAMES brotlicommon)
        endif()
        find_library(EXPAT_LIBRARY NAMES expat PATHS ${SKIA_LIB_DIR} NO_DEFAULT_PATH)
        if(NOT EXPAT_LIBRARY)
            find_library(EXPAT_LIBRARY NAMES expat)
        endif()

        if(PNG_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${PNG_LIBRARY})
            if(TARGET ZenithDAWTests)
                target_link_libraries(ZenithDAWTests PRIVATE ${PNG_LIBRARY})
            endif()
        endif()
        if(Z_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${Z_LIBRARY})
            if(TARGET ZenithDAWTests)
                target_link_libraries(ZenithDAWTests PRIVATE ${Z_LIBRARY})
            endif()
        endif()
        if(JPEG_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${JPEG_LIBRARY}) 
        endif()
        if(WEBP_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${WEBP_LIBRARY}) 
        endif()
        if(FREETYPE_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${FREETYPE_LIBRARY}) 
        endif()
        if(FONTCONFIG_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${FONTCONFIG_LIBRARY}) 
        endif()
        if(BZ2_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${BZ2_LIBRARY}) 
        endif()
        if(BROTLIDEC_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${BROTLIDEC_LIBRARY}) 
        endif()
        if(BROTLICOMMON_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${BROTLICOMMON_LIBRARY}) 
        endif()
        if(EXPAT_LIBRARY) 
            target_link_libraries(ZenithDAW PRIVATE ${EXPAT_LIBRARY}) 
        endif()
        
        # Link system libraries for Linux Skia
        target_link_libraries(ZenithDAW PRIVATE dl pthread m)
    endif()

    if(TARGET ZenithDAWTests)
        target_link_libraries(ZenithDAWTests PRIVATE ${SKIA_LIBRARY})
    endif()
    message(STATUS "  Skia graphics library: LINKED (Manual)")
else()
    message(FATAL_ERROR "Skia library not found in ${SKIA_LIB_DIR}")
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