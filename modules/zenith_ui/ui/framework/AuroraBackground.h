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

    AuroraBackground.h
    Created: 2025-12-13
    Author:  Zenith DAW Team

    "Living" Mesh Gradient Background Renderer.
    Replaces the "screensaver from 2005" with a premium shifting fog/aurora.


  ==============================================================================
*/

#pragma once

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkShader.h>
#include <effects/SkGradientShader.h>
#include <effects/SkRuntimeEffect.h>
#endif

namespace zenith {

class AuroraBackground {
public:
  AuroraBackground();
  ~AuroraBackground();

  /**
   * Render the Aurora Mesh Gradient.
   * @param canvas The Skia canvas to draw onto.
   * @param bounds The bounds of the area to fill.
   * @param time   Current animation time in seconds.
   */
#ifdef ZENITH_USE_SKIA
  void draw(SkCanvas *canvas, const SkRect &bounds, float time);
#endif

private:
#ifdef ZENITH_USE_SKIA
  // Runtime Effect for the "Smoke" displacement (if initialized)
  sk_sp<SkRuntimeEffect> noiseEffect_;
  bool hasRuntimeEffect_ = false;

  void initShaders();

  // Helpers for fallback rendering
  void drawMeshGradient(SkCanvas *canvas, const SkRect &bounds, float time);
  void drawVignette(SkCanvas *canvas, const SkRect &bounds);
#endif
};

} // namespace zenith
