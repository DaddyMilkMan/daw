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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ZenithTooltipOverlay.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Glass overlay for Learning Mode / tooltip display.
    Highlights a target control with glow and shows tooltip card.


  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "ZenithControl.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#endif

namespace zenith {

class ZenithTooltipOverlay : public SkiaComponent {
public:
  ZenithTooltipOverlay() = default;
  ~ZenithTooltipOverlay() override = default;

  // ----- Target -----
  void setTarget(ZenithControl *target);
  ZenithControl *getTarget() const { return target_; }

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

  // Pass through mouse events
  bool hitTest(int x, int y) override;

private:
#ifdef ZENITH_USE_SKIA
  void drawTooltipCard(SkCanvas *canvas, const SkRect &targetRect);
#endif

  ZenithControl *target_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTooltipOverlay)
};

} // namespace zenith
