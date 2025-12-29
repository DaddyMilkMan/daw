# =============================================================================
# COMPILER FLAGS CONFIGURATION
# =============================================================================

# Modern C++ standards and conformance
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build optimization options
option(ENABLE_IPO "Enable Interprocedural Optimization (LTO)" OFF)
option(ENABLE_SPECTRE "Enable Spectre Mitigations (MSVC)" OFF)
option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)

# Debug-only options (disabled for Release/Skia compatibility)
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_SANITIZERS OFF CACHE BOOL "Enable Address and UB Sanitizers" FORCE)
    set(ENABLE_HARDENING OFF CACHE BOOL "Enable Security Hardening Flags" FORCE)
endif()

# Sanitizer configuration (Debug builds only)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
    message(STATUS "Sanitizers enabled for Debug build")
endif()