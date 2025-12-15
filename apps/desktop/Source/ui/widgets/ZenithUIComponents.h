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

// Base control class
#include "widgets/ZenithControl.h"

// Core controls
#include "widgets/ZenithButton.h"
#include "widgets/ZenithKnob.h"
#include "widgets/ZenithSlider.h"

// Form controls
#include "widgets/ZenithDropdown.h"
#include "widgets/ZenithTextInput.h"
#include "widgets/ZenithToggle.h"

// Specialized widgets
#include "widgets/ZenithModMatrix.h"
#include "widgets/ZenithTooltipOverlay.h"
#include "widgets/ZenithVisualizer.h"

// Legacy aliases for compatibility
namespace zenith {

// Alias for compatibility with older code expecting SkiaCanvasComponent
using SkiaCanvasComponent = SkiaComponent;

// Alias for lightweight widget system
using SkiaWidget = SkiaComponent;

} // namespace zenith
