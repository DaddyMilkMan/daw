# =============================================================================
# ZENITH NETWORK MODULE
# =============================================================================

include_guard(GLOBAL)

# Network system modules
set(ZENITH_NETWORK_SOURCES
    modules/zenith_network/mcp/MCPServer.cpp
    modules/zenith_network/network/SecureKeyStore.cpp
)

# Create network library
add_library(zenith_network STATIC
    ${ZENITH_NETWORK_SOURCES}
)

# Target properties
target_include_directories(zenith_network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_network>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(zenith_network
    PUBLIC
        zenith_core
        juce_core
        juce_events
)
