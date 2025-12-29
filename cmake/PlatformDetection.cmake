# =============================================================================
# PLATFORM DETECTION AND CONFIGURATION
# =============================================================================

# Platform detection
if(WIN32)
    set(ZENITH_PLATFORM_WINDOWS TRUE)
    set(ZENITH_PLATFORM_NAME "Windows")
elseif(APPLE)
    set(ZENITH_PLATFORM_MACOS TRUE)
    set(ZENITH_PLATFORM_NAME "macOS")
elseif(UNIX)
    set(ZENITH_PLATFORM_LINUX TRUE)
    set(ZENITH_PLATFORM_NAME "Linux")
endif()

# Architecture detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ZENITH_ARCH_64 TRUE)
    set(ZENITH_ARCH_NAME "x64")
else()
    set(ZENITH_ARCH_32 TRUE)
    set(ZENITH_ARCH_NAME "x86")
endif()

message(STATUS "Zenith DAW: Building for ${ZENITH_PLATFORM_NAME} (${ZENITH_ARCH_NAME})")
