# ==============================================================================
# Zenith DAW - Precompiled Header Configuration
# ==============================================================================
# Purpose: Provide a PCH that includes heavy JUCE headers, reducing
#          incremental build times significantly.
#
# Usage: Include this file from the main CMakeLists.txt after defining
#        the ZenithDAW target.
#
# Expected Build Time Improvement: 30-60% for incremental builds
# ==============================================================================

message(STATUS "============================================")
message(STATUS "Configuring Precompiled Headers (PCH)")
message(STATUS "============================================")

# Define the PCH source file path (relative to project root for consistency)
set(ZENITH_PCH_HEADER "${CMAKE_SOURCE_DIR}/apps/desktop/Source/pch/ZenithPCH.h")

# Verify the PCH file exists
if(NOT EXISTS "${ZENITH_PCH_HEADER}")
    message(WARNING "PCH header not found: ${ZENITH_PCH_HEADER}")
    message(WARNING "PCH is DISABLED - create ZenithPCH.h to enable")
    return()
endif()

# Configure PCH for ZenithDAW target
# Using PUBLIC would force all linking targets to also use the PCH
# Using PRIVATE is correct here - only ZenithDAW compilation uses it
target_precompile_headers(ZenithDAW PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${ZENITH_PCH_HEADER}>"
)

message(STATUS "  PCH Header: ${ZENITH_PCH_HEADER}")
message(STATUS "  PCH: ENABLED for ZenithDAW target")

# ==============================================================================
# MSVC-specific PCH optimizations
# ==============================================================================
if(MSVC)
    # /Zm200 - Increase PCH memory allocation (default may be too small for JUCE)
    # This prevents "out of memory during PCH generation" errors 
    target_compile_options(ZenithDAW PRIVATE /Zm200)
    message(STATUS "  MSVC PCH memory: Increased (/Zm200)")
endif()

message(STATUS "============================================")
