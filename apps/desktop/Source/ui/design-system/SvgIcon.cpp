/*
  ==============================================================================

    SvgIcon.cpp
    Created: 2026-02-01
    Author:  Zenith DAW

  ==============================================================================
*/

#include "SvgIcon.h"
#include "ZenithIcons.h"

namespace zenith::svgicons {
namespace {

SkPath toPath(IconId id) {
  switch (id) {
  case IconId::Play:
    return icons::Play();
  case IconId::Stop:
    return icons::Stop();
  case IconId::Record:
    return icons::Record();
  case IconId::Settings:
    return icons::Settings();
  case IconId::Wing:
    return icons::Wingman();
  case IconId::ViewToggle:
    return icons::ViewToggle();
  case IconId::File:
    return icons::File();
  case IconId::Folder:
    return icons::Folder();
  case IconId::Edit:
    return icons::Edit();
  case IconId::Info:
    return icons::Info();
  }
  return icons::Info();
}

} // namespace

void preload(std::initializer_list<IconId> icons) { juce::ignoreUnused(icons); }

void drawIconCentered(SkCanvas *canvas, IconId id, const SkRect &bounds,
                      float size, const Style &style) {
  if (!canvas)
    return;

  icons::IconStyle iconStyle;
  iconStyle.color = style.color;
  iconStyle.filled = style.filled;
  iconStyle.glowRadius = style.glowRadius;
  iconStyle.glowColor = style.glowColor;
  iconStyle.strokeWidth = icons::STROKE_REGULAR;
  icons::drawIconCentered(canvas, toPath(id), bounds, size, iconStyle);
}

} // namespace zenith::svgicons
