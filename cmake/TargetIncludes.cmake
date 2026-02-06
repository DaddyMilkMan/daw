# =============================================================================
# TARGET INCLUDE DIRECTORIES - Centralized Interface Libraries
# =============================================================================
# Anti-Corner-Cutting Protocol:
# - NO include_directories() calls (pollutes global scope)
# - All includes via target_include_directories() with visibility
# - Interface libraries for shared include sets
#
# Benefits:
# - DRY: Define includes once, link to multiple targets
# - Clear visibility (PUBLIC/PRIVATE/INTERFACE)
# - Better dependency tracking for incremental builds
# - Reusable across ZenithDAW, ZenithDAWTests, ZenithCore, etc.
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Core Source Includes - Used by all Zenith targets
# -----------------------------------------------------------------------------
# Contains: engine, commands, instruments, network, dsp, ai, utils, rendering
# Does NOT include UI (for headless builds)
# -----------------------------------------------------------------------------
add_library(zenith_core_includes INTERFACE)
target_include_directories(zenith_core_includes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/ai_client
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/browser
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/instruments
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/effects
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/synth_engine
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_core/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_network
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_network/network
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_network/collaboration
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_network/cloud
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_network/mcp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_commands
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_commands/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/agents/cpp/ObservabilityAgent
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/agents/cpp/TransportProtocolAgent
)

# -----------------------------------------------------------------------------
# UI Includes - For targets that need UI components
# -----------------------------------------------------------------------------
# Contains: All UI subdirectories (framework, design-system, views, etc.)
# Automatically includes core includes via dependency
# -----------------------------------------------------------------------------
add_library(zenith_ui_includes INTERFACE)
target_include_directories(zenith_ui_includes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/framework
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/design-system
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/arranger
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/mixer
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/piano-roll
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/transport
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/common
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/controls
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/session
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/instruments
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/sample-editor
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/skia
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/views
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/dashboards
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/zenith_ui/ui/settings
)
# UI targets implicitly need core includes
target_link_libraries(zenith_ui_includes INTERFACE zenith_core_includes)

# -----------------------------------------------------------------------------
# JUCE Includes - For targets using JUCE modules
# -----------------------------------------------------------------------------
add_library(zenith_juce_includes INTERFACE)
target_include_directories(zenith_juce_includes SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules
)

# -----------------------------------------------------------------------------
# Full Application Includes - Combines all include sets
# -----------------------------------------------------------------------------
# Use this for ZenithDAW and ZenithDAWTests
# Provides: core + ui + juce
# -----------------------------------------------------------------------------
add_library(zenith_app_includes INTERFACE)
target_link_libraries(zenith_app_includes INTERFACE
    zenith_core_includes
    zenith_ui_includes
    zenith_juce_includes
)

# -----------------------------------------------------------------------------
# Platform-specific includes (added conditionally)
# -----------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    target_include_directories(zenith_core_includes INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/platform/linux
    )
elseif(APPLE)
    target_include_directories(zenith_core_includes INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/platform/mac
    )
elseif(WIN32)
    target_include_directories(zenith_core_includes INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/platform/windows
    )
endif()

message(STATUS "Zenith DAW: Include interface libraries configured")
message(STATUS "  zenith_core_includes: Core engine/processing includes")
message(STATUS "  zenith_ui_includes: UI framework and components")
message(STATUS "  zenith_juce_includes: JUCE module headers")
message(STATUS "  zenith_app_includes: Full application (all includes)")
