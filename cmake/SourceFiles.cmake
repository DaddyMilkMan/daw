# =============================================================================
# SOURCE FILE ORGANIZATION - MODULAR ARCHITECTURE
# =============================================================================

include_guard(GLOBAL)

# This file provides a modular organization of source files.
# Each major component is organized into its own CMake module file.

# Main application sources
set(ZENITH_APP_SOURCES
    apps/desktop/Source/Main.cpp
    apps/desktop/Source/ui/common/MainWindow.cpp
)

# Note: Individual modules are now defined in separate CMake files:
# - ZenithEngineCore.cmake
# - ZenithUIUnified.cmake
# - And other module files...

# Legacy sources (deprecated but kept for compatibility)
set(ZENITH_LEGACY_SOURCES
    # These files are maintained for backward compatibility
    # but should be refactored or removed when possible
    modules/zenith_core/engine/ClipSynchronizer.cpp
    modules/zenith_core/engine/TrackStateSynchronizer.cpp
    modules/zenith_core/engine/TrackAutomationSynchronizer.cpp
    modules/zenith_core/engine/TempoMapSynchronizer.cpp
)

# Test sources
set(ZENITH_TEST_SOURCES
    apps/desktop/Source/tests/
    apps/desktop/Source/tests/TrackManagerCreateTest.cpp
)

# Tools and utilities
set(ZENITH_TOOLS_SOURCES
    apps/desktop/Source/tools/
)
