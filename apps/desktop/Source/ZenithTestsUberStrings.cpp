
/*
  ==============================================================================

    ZenithTestsUberStrings.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

    This file includes .cpp files directly to force their compilation into the test target.
    This is a workaround for CMake ignoring them in the source list.

  ==============================================================================
*/

// Include necessary headers to prevent duplicate definition errors if headers are included multiple times
// (although guards should handle it).

// We need to define ZENITH_DAW_TESTS so that if the cpp files verify it, they know.

#include "modules/zenith_core/instruments/ZenithPolySynthParameterManager.cpp"
#include "modules/zenith_ui/ui/controls/ZenithVisualizer.cpp"
