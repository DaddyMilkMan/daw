# =============================================================================
# ZENITH MODULE DEFINITIONS
# =============================================================================

include_guard(GLOBAL)

# Include individual module definitions
include(ZenithEngineCore)
include(ZenithUIUnified)

# Additional module includes would go here
# include(ZenithDSP)
# include(ZenithAudioEngine)
# include(ZenithUIFramework)
# include(ZenithNetwork)
# include(ZenithAI)

# Main application sources
set(ZENITH_APP_SOURCES
    apps/desktop/Source/Main.cpp
    apps/desktop/Source/ui/common/MainWindow.cpp
)

# NOTE:
# This file is legacy and is intentionally limited to source-list definitions.
# The real build targets are declared in the top-level CMakeLists.txt.
