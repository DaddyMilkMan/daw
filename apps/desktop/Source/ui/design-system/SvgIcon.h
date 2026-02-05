/*
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
