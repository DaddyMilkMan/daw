# =============================================================================
# DEPENDENCY VERSION MANIFEST - PINNED TO GIT HASHES
# =============================================================================
# Anti-Corner-Cutting Protocol: Never use branch names (master, main)
# Every external dependency must be pinned to a specific commit hash.
#
# This provides:
#   - Reproducible builds across all environments
#   - Protection against upstream force-pushes to tags
#   - Explicit upgrade process with changelog review
#
# To update a dependency:
#   1. Find the new commit hash (git log, GitHub releases)
#   2. Update the hash in this file
#   3. Run cmake --fresh to verify fetch
#   4. Run full test suite before committing
# =============================================================================

# -----------------------------------------------------------------------------
# JUCE Framework
# -----------------------------------------------------------------------------
# Version: 8.0.11
# Verified against local external/JUCE submodule
# Release notes: https://github.com/juce-framework/JUCE/releases/tag/8.0.11
set(ZENITH_JUCE_GIT_HASH "c352e2489000ffeb9e397051377d75eef2efe641"
    CACHE STRING "JUCE framework git commit hash")
set(ZENITH_JUCE_GIT_URL "https://github.com/juce-framework/JUCE.git"
    CACHE STRING "JUCE git repository URL")

# -----------------------------------------------------------------------------
# ONNX Runtime
# -----------------------------------------------------------------------------
# Version: 1.17.1 (Linux x64 pre-built binary)
# Release notes: https://github.com/microsoft/onnxruntime/releases/tag/v1.17.1
set(ZENITH_ONNX_VERSION "1.17.1"
    CACHE STRING "ONNX Runtime version")

# Platform-specific archives
if(UNIX AND NOT APPLE)
    set(ZENITH_ONNX_URL 
        "https://github.com/microsoft/onnxruntime/releases/download/v${ZENITH_ONNX_VERSION}/onnxruntime-linux-x64-${ZENITH_ONNX_VERSION}.tgz"
        CACHE STRING "ONNX Runtime archive URL")
    set(ZENITH_ONNX_SHA256 
        "89b153af88746665909c758a06797175ae366280cbf25502c41eb5955f9a555e"
        CACHE STRING "ONNX Runtime archive SHA256 hash")
elseif(APPLE)
    set(ZENITH_ONNX_URL 
        "https://github.com/microsoft/onnxruntime/releases/download/v${ZENITH_ONNX_VERSION}/onnxruntime-osx-universal2-${ZENITH_ONNX_VERSION}.tgz"
        CACHE STRING "ONNX Runtime archive URL")
    set(ZENITH_ONNX_SHA256 
        ""  # TODO: Add SHA256 when macOS support is implemented
        CACHE STRING "ONNX Runtime archive SHA256 hash")
elseif(WIN32)
    set(ZENITH_ONNX_URL 
        "https://github.com/microsoft/onnxruntime/releases/download/v${ZENITH_ONNX_VERSION}/onnxruntime-win-x64-${ZENITH_ONNX_VERSION}.zip"
        CACHE STRING "ONNX Runtime archive URL")
    set(ZENITH_ONNX_SHA256 
        ""  # TODO: Add SHA256 when Windows support is implemented
        CACHE STRING "ONNX Runtime archive SHA256 hash")
endif()

# -----------------------------------------------------------------------------
# OpenSSL (fallback when system OpenSSL not found)
# -----------------------------------------------------------------------------
# Version: 3.2.1
# Release notes: https://github.com/openssl/openssl/releases/tag/openssl-3.2.1
set(ZENITH_OPENSSL_VERSION "3.2.1"
    CACHE STRING "OpenSSL version for FetchContent fallback")
set(ZENITH_OPENSSL_URL 
    "https://github.com/openssl/openssl/releases/download/openssl-${ZENITH_OPENSSL_VERSION}/openssl-${ZENITH_OPENSSL_VERSION}.tar.gz"
    CACHE STRING "OpenSSL archive URL")
set(ZENITH_OPENSSL_SHA256 
    "4ef067139369528d25ca6b07e596766465f2425cf3697e8e50b91d2fd9d924d5"
    CACHE STRING "OpenSSL archive SHA256 hash")

# -----------------------------------------------------------------------------
# LuaBridge (Lua scripting bindings)
# -----------------------------------------------------------------------------
# Version: 2.8
# Commit hash corresponding to tag 2.8
# Repository: https://github.com/vinniefalco/LuaBridge
set(ZENITH_LUABRIDGE_GIT_HASH "e0a7f3ce67e83a0e80cd4f9cc4c1c1ea77604ac1"
    CACHE STRING "LuaBridge git commit hash for version 2.8")
set(ZENITH_LUABRIDGE_GIT_URL "https://github.com/vinniefalco/LuaBridge.git"
    CACHE STRING "LuaBridge git repository URL")

# =============================================================================
# Version Summary
# =============================================================================
message(STATUS "═══════════════════════════════════════════════════════════════")
message(STATUS "Zenith DAW: Dependency Version Manifest")
message(STATUS "═══════════════════════════════════════════════════════════════")
message(STATUS "  JUCE:         ${ZENITH_JUCE_GIT_HASH} (8.0.11)")
message(STATUS "  ONNX Runtime: v${ZENITH_ONNX_VERSION}")
message(STATUS "  OpenSSL:      v${ZENITH_OPENSSL_VERSION} (fallback)")
message(STATUS "  LuaBridge:    ${ZENITH_LUABRIDGE_GIT_HASH} (2.8)")
message(STATUS "═══════════════════════════════════════════════════════════════")
