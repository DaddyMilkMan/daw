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

    SvgIcon.h
    Created: 2026-02-01
    Author:  Zenith DAW

    Skia SVG icon loader + renderer (SkSVGDOM).
    Used to render transport + menu icons from SVG assets.


  ==============================================================================
*/

#pragma once

#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkRect.h>
#include <juce_core/juce_core.h>
#include <initializer_list>

namespace zenith::svgicons {

enum class IconId {
  Play,
  Stop,
  Record,
  Settings,
  Wing,
  ViewToggle,
  File,
  Edit,
  Info
};

struct Style {
  SkColor color = SK_ColorWHITE;
  float glowRadius = 0.0f;
  SkColor glowColor = 0x00000000;
  bool filled = false;
};

void preload(std::initializer_list<IconId> icons);
void drawIconCentered(SkCanvas *canvas, IconId id, const SkRect &bounds,
                      float size, const Style &style);

} // namespace zenith::svgicons
