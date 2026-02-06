# =============================================================================
# ZENITH COMMANDS MODULE
# =============================================================================

include_guard(GLOBAL)

# Command API sources
set(ZENITH_COMMANDS_SOURCES
    modules/zenith_commands/commands/CommandAPI.cpp
)

# Create commands library
add_library(zenith_commands STATIC
    ${ZENITH_COMMANDS_SOURCES}
)

# Target properties
target_include_directories(zenith_commands
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zenith_commands
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_commands
    PUBLIC
        zenith_core
        juce_core
)

# Installation
install(
    TARGETS zenith_commands
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
