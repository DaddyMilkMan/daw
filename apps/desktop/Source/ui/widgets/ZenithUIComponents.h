/*
  ==============================================================================

    ZenithUIComponents.h
    Created: 2025-11-27
    Updated: 2025-12-12
    Author:  Zenith DAW

    Umbrella header for all Zenith UI components.

    This file has been refactored from a 1,187-line monolith into
    individual component files for better maintainability.

  ==============================================================================
*/

#pragma once
#include "SkiaComponent.h"

// Base control class
#include "controls/ZenithControl.h"

// Core controls

// Specialized widgets
#include "controls/ZenithTooltipOverlay.h"
#include "visualization/ZenithModMatrix.h"
#include "visualization/ZenithVisualizer.h"

// Form controls
#include "controls/ZenithDropdown.h"
#include "controls/ZenithTextInput.h"
#include "controls/ZenithToggle.h"

// Specialized widgets
#include "controls/ZenithTooltipOverlay.h"
#include "visualization/ZenithModMatrix.h"
#include "visualization/ZenithVisualizer.h"

// Legacy aliases for compatibility
namespace zenith {

// Alias for compatibility with older code expecting SkiaCanvasComponent
using SkiaCanvasComponent = SkiaComponent;

// Alias for lightweight widget system
using SkiaWidget = SkiaComponent;

} // namespace zenith
