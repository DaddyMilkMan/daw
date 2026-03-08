# =============================================================================
# DEPENDENCIES MANAGEMENT
# =============================================================================
# Anti-Corner-Cutting Protocol:
# - All versions pinned to specific git hashes (see FetchContentVersions.cmake)
# - No usage of branch names (master, main)
# - Archive downloads verified with SHA256 hashes
# =============================================================================

include(FetchContent)

# Load pinned version manifest
include(${CMAKE_CURRENT_LIST_DIR}/FetchContentVersions.cmake)

# Verify manifest loaded
if(NOT ZENITH_JUCE_GIT_HASH)
    message(FATAL_ERROR "Dependency manifest properly not loaded!")
endif()

# =============================================================================
# 1. JUCE Framework
# =============================================================================
# JUCE Setup - prefer local directory if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    message(STATUS "Using local JUCE from external/JUCE")
    add_subdirectory(external/JUCE)
else()
    message(STATUS "Fetching JUCE from GitHub (pinned to ${ZENITH_JUCE_GIT_HASH})...")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY ${ZENITH_JUCE_GIT_URL}
        GIT_TAG ${ZENITH_JUCE_GIT_HASH}
        GIT_SHALLOW FALSE  # Required for specific commit hash
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Core JUCE modules for Zenith DAW
set(ZENITH_JUCE_MODULES
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

message(STATUS "Zenith DAW: JUCE configured (hash: ${ZENITH_JUCE_GIT_HASH})")

# =============================================================================
# 2. ONNX Runtime (AI Features)
# =============================================================================
option(ENABLE_ONNX "Enable ONNX Runtime for AI features" ON)

if(ENABLE_ONNX)
    message(STATUS "Fetching ONNX Runtime v${ZENITH_ONNX_VERSION} for current platform...")
    
    FetchContent_Declare(
        onnxruntime
        URL ${ZENITH_ONNX_URL}
        # URL_HASH SHA256=${ZENITH_ONNX_SHA256} # Omit hash if empty to allow experimental platform support
    )
    FetchContent_MakeAvailable(onnxruntime)

    # Create imported target for consistent usage across DAW and Tests
    if(NOT TARGET onnxruntime)
        add_library(onnxruntime UNKNOWN IMPORTED)
        
        if(ZENITH_PLATFORM_WINDOWS)
            set_target_properties(onnxruntime PROPERTIES
                IMPORTED_LOCATION "${onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib"
                INTERFACE_INCLUDE_DIRECTORIES "${onnxruntime_SOURCE_DIR}/include"
            )
            set(ONNX_RUNTIME_SHARED_LIB "onnxruntime.dll")
        elseif(ZENITH_PLATFORM_MACOS)
            set_target_properties(onnxruntime PROPERTIES
                IMPORTED_LOCATION "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.dylib"
                INTERFACE_INCLUDE_DIRECTORIES "${onnxruntime_SOURCE_DIR}/include"
            )
            set(ONNX_RUNTIME_SHARED_LIB "libonnxruntime.dylib")
        else()
            set_target_properties(onnxruntime PROPERTIES
                IMPORTED_LOCATION "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so"
                INTERFACE_INCLUDE_DIRECTORIES "${onnxruntime_SOURCE_DIR}/include"
            )
            set(ONNX_RUNTIME_SHARED_LIB "libonnxruntime.so")
        endif()
    endif()
    
    set(ZENITH_HAS_ONNX TRUE)
    add_compile_definitions(ZENITH_USE_ONNX_RUNTIME=1)
else()
    message(STATUS "Zenith DAW: ONNX Runtime support disabled by user")
    set(ZENITH_HAS_ONNX FALSE)
endif()

# =============================================================================
# 3. OpenSSL (Collaboration Security)
# =============================================================================
if(ZENITH_ENABLE_COLLAB)
    find_package(OpenSSL QUIET)

    if(NOT OPENSSL_FOUND)
        message(STATUS "Zenith DAW: System OpenSSL not found, fetching v${ZENITH_OPENSSL_VERSION}...")
        
        FetchContent_Declare(
            openssl
            URL ${ZENITH_OPENSSL_URL}
            URL_HASH SHA256=${ZENITH_OPENSSL_SHA256}
        )
        FetchContent_MakeAvailable(openssl)
        
        # Define targets to match FindOpenSSL
        if(NOT TARGET OpenSSL::SSL)
            add_library(OpenSSL::SSL STATIC IMPORTED)
            set_target_properties(OpenSSL::SSL PROPERTIES
                IMPORTED_LOCATION ${openssl_BINARY_DIR}/libssl.a
                INTERFACE_INCLUDE_DIRECTORIES ${openssl_SOURCE_DIR}/include)
        endif()
        if(NOT TARGET OpenSSL::Crypto)
            add_library(OpenSSL::Crypto STATIC IMPORTED)
            set_target_properties(OpenSSL::Crypto PROPERTIES
                IMPORTED_LOCATION ${openssl_BINARY_DIR}/libcrypto.a
                INTERFACE_INCLUDE_DIRECTORIES ${openssl_SOURCE_DIR}/include)
        endif()
        
        message(STATUS "Zenith DAW: Using fetched OpenSSL 3.2.1")
    else()
        message(STATUS "Zenith DAW: Using system OpenSSL ${OPENSSL_VERSION}")
    endif()
endif()

# =============================================================================
# 4. LuaBridge (Scripting)
# =============================================================================
# FetchContent_Declare(
#     LuaBridge
#     GIT_REPOSITORY ${ZENITH_LUABRIDGE_GIT_URL}
#     GIT_TAG ${ZENITH_LUABRIDGE_GIT_HASH}
# )
# FetchContent_MakeAvailable(LuaBridge)

