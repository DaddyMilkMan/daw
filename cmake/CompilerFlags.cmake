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

# Sanitizer options - Default ON for Debug builds to catch memory leaks
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_SANITIZERS "Enable Address, Leak and UB Sanitizers" ON)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
else()
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
    option(ENABLE_HARDENING "Enable Security Hardening Flags" OFF)
endif()

# Sanitizer configuration
if(ENABLE_SANITIZERS)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        message(STATUS "Sanitizers enabled: Address")
    else()
        # Define sanitizer flags for reuse
        set(SANITIZER_FLAGS -fsanitize=address -fsanitize=leak -fsanitize=undefined)
        
        # Enable AddressSanitizer, LeakSanitizer (implicit with ASan), and UBSan
        add_compile_options(${SANITIZER_FLAGS})
        add_link_options(${SANITIZER_FLAGS})
        
        # Improved error reporting
        add_compile_options(-fno-omit-frame-pointer -g)
        
        message(STATUS "Sanitizers enabled: Address, Leak, UndefinedBehavior")
    endif()
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
