# =============================================================================
# PLATFORM DETECTION AND CONFIGURATION
# =============================================================================
# Detects the build platform and architecture, setting variables for
# conditional compilation throughout the build system.
#
# Output Variables:
#   ZENITH_PLATFORM_WINDOWS / ZENITH_PLATFORM_MACOS / ZENITH_PLATFORM_LINUX
#   ZENITH_ARCH_64 / ZENITH_ARCH_32
#   ZENITH_ARCH_X64 / ZENITH_ARCH_ARM64
#   ZENITH_PLATFORM_NAME / ZENITH_ARCH_NAME
#   ZENITH_PLATFORM_DEFINES (compile definitions for platform)
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Platform detection
# -----------------------------------------------------------------------------
if(WIN32)
    set(ZENITH_PLATFORM_WINDOWS TRUE)
    set(ZENITH_PLATFORM_NAME "Windows")
    set(ZENITH_PLATFORM_DEFINES "ZENITH_PLATFORM_WINDOWS=1")
elseif(APPLE)
    set(ZENITH_PLATFORM_MACOS TRUE)
    set(ZENITH_PLATFORM_NAME "macOS")
    set(ZENITH_PLATFORM_DEFINES "ZENITH_PLATFORM_MACOS=1")
elseif(UNIX)
    set(ZENITH_PLATFORM_LINUX TRUE)
    set(ZENITH_PLATFORM_NAME "Linux")
    set(ZENITH_PLATFORM_DEFINES "ZENITH_PLATFORM_LINUX=1")
else()
    message(WARNING "Unknown platform detected - some features may not work")
    set(ZENITH_PLATFORM_NAME "Unknown")
    set(ZENITH_PLATFORM_DEFINES "")
endif()

# -----------------------------------------------------------------------------
# Architecture detection
# -----------------------------------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ZENITH_ARCH_64 TRUE)
    
    # Detect ARM64 vs x86_64
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64|ARM64")
        set(ZENITH_ARCH_ARM64 TRUE)
        set(ZENITH_ARCH_NAME "arm64")
    else()
        set(ZENITH_ARCH_X64 TRUE)
        set(ZENITH_ARCH_NAME "x64")
    endif()
else()
    set(ZENITH_ARCH_32 TRUE)
    set(ZENITH_ARCH_NAME "x86")
    message(WARNING "32-bit builds are deprecated and may have limited support")
endif()

# -----------------------------------------------------------------------------
# Cross-platform compiler flags
# -----------------------------------------------------------------------------
# These are safe defaults that apply to all platforms

if(ZENITH_PLATFORM_LINUX)
    # Linux-specific: Enable ALSA by default
    list(APPEND ZENITH_PLATFORM_DEFINES "JUCE_ALSA=1")
    
    # PipeWire support (optional, detected at runtime by JUCE)
    # list(APPEND ZENITH_PLATFORM_DEFINES "JUCE_USE_PIPEWIRE=1")
    
elseif(ZENITH_PLATFORM_MACOS)
    # macOS-specific: Enable CoreAudio
    list(APPEND ZENITH_PLATFORM_DEFINES "JUCE_USE_COREAUDIO=1")
    
    # Apple Silicon support
    if(ZENITH_ARCH_ARM64)
        list(APPEND ZENITH_PLATFORM_DEFINES "ZENITH_APPLE_SILICON=1")
    endif()
    
elseif(ZENITH_PLATFORM_WINDOWS)
    # Windows-specific: WASAPI by default, optional ASIO
    list(APPEND ZENITH_PLATFORM_DEFINES "JUCE_WASAPI=1")
    list(APPEND ZENITH_PLATFORM_DEFINES "JUCE_DIRECTSOUND=1")
endif()

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
message(STATUS "═══════════════════════════════════════════════════════════════")
message(STATUS "Zenith DAW: Platform Detection")
message(STATUS "═══════════════════════════════════════════════════════════════")
message(STATUS "  Platform: ${ZENITH_PLATFORM_NAME}")
message(STATUS "  Architecture: ${ZENITH_ARCH_NAME}")
message(STATUS "  Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Defines: ${ZENITH_PLATFORM_DEFINES}")
message(STATUS "═══════════════════════════════════════════════════════════════")
