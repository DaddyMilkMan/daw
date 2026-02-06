/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// Base control class
#include "controls/ZenithControl.h"

// Core controls
#include "controls/ZenithButton.h"
#include "controls/ZenithKnob.h"
#include "controls/ZenithSlider.h"

// Form controls
#include "controls/ZenithDropdown.h"
#include "controls/ZenithTextInput.h"
#include "controls/ZenithToggle.h"

// Specialized widgets
#include "controls/ZenithModMatrix.h"
#include "controls/ZenithTooltipOverlay.h"
#include "controls/ZenithVisualizer.h"

// Legacy aliases for compatibility
namespace zenith {

// Alias for compatibility with older code expecting SkiaCanvasComponent
using SkiaCanvasComponent = SkiaComponent;

// Alias for lightweight widget system
using SkiaWidget = SkiaComponent;

// Legacy aliases for backward compatibility during refactor
using SkiaButton = ZenithButton;
using SkiaKnob = ZenithKnob;
using SkiaSlider = ZenithSlider;

} // namespace zenith
