# ObservabilityAgent CMake Build Configuration

cmake_minimum_required(VERSION 3.25)

# Define the ObservabilityAgent library
add_library(ObservabilityAgent STATIC
    observability.cpp
)

# Set C++ standard to match project requirements
target_compile_features(ObservabilityAgent PUBLIC cxx_std_20)

# Include directories
target_include_directories(ObservabilityAgent PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler warnings and flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ObservabilityAgent PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(ObservabilityAgent PRIVATE
        /W4
        /WX
    )
endif()

# Optional: Link with common dependencies if needed in the future
# target_link_libraries(ObservabilityAgent PUBLIC
#     juce::juce_core
# )
