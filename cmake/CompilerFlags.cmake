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

# Debug-only options (disabled for Release/Skia compatibility)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
else()
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
endif()

# Sanitizer configuration (Release builds only)
if(ENABLE_SANITIZERS AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
    message(STATUS "Sanitizers enabled for Release build")
endif()

# =============================================================================
# CODE COVERAGE
# =============================================================================

option(ZENITH_ENABLE_COVERAGE "Enable code coverage generation" OFF)

if(ZENITH_ENABLE_COVERAGE)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Code coverage enabled for non-Debug build. This may severely impact performance.")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(STATUS "Code coverage enabled")
        add_compile_options(-fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Code coverage is only supported for GNU and Clang compilers")
    endif()
endif()
