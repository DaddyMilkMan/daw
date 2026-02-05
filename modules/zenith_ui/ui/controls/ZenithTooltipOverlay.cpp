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

    ZenithTooltipOverlay.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the tooltip overlay for Learning Mode.

  ==============================================================================

*/

#include "ZenithTooltipOverlay.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#endif

namespace zenith {

void ZenithTooltipOverlay::setTarget(ZenithControl *target) {
  if (target_ != target) {
    target_ = target;
    repaint();
  }
}

bool ZenithTooltipOverlay::hitTest(int x, int y) {
  juce::ignoreUnused(x, y);
  return false; // Pass through all mouse events
}

void ZenithTooltipOverlay::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr || target_ == nullptr)
    return;

  auto bounds = getLocalBounds().toFloat();

  SkPaint paint;
  paint.setAntiAlias(true);

  // 1. Dim background with cutout
  paint.setColor(SkColorSetARGB(200, 10, 10, 15));

  SkPath path;
  path.addRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));

  // Get target bounds relative to this component
  auto targetBounds = target_->getBounds().toFloat();
  SkRect targetRect =
      SkRect::MakeXYWH(targetBounds.getX(), targetBounds.getY(),
                       targetBounds.getWidth(), targetBounds.getHeight());

  // Add cutout (counter-clockwise to create hole)
  path.addRect(targetRect, SkPathDirection::kCCW);
  canvas->drawPath(path, paint);

  // 2. Glow around target
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(2.0f);
  paint.setColor(SkColorSetRGB(0, 255, 255));
  paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
  canvas->drawRect(targetRect, paint);

  // Sharp border
  paint.setMaskFilter(nullptr);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(150, 255, 255, 255));
  canvas->drawRect(targetRect, paint);

  // 3. Tooltip card
  drawTooltipCard(canvas, targetRect);
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

void ZenithTooltipOverlay::drawTooltipCard(SkCanvas *canvas,
                                           const SkRect &targetRect) {
  if (target_ == nullptr)
    return;

  SkPaint paint;
  paint.setAntiAlias(true);

  // Position card to the right of target (or left if no space)
  float cardX = targetRect.right() + 20.0f;
  float cardY = targetRect.top();
  float cardW = 200.0f;
  float cardH = 100.0f;

  if (cardX + cardW > getWidth()) {
    cardX = targetRect.left() - cardW - 20.0f;
  }

  // Clamp to visible area
  if (cardX < 10.0f)
    cardX = 10.0f;
  if (cardY + cardH > getHeight() - 10.0f) {
    cardY = getHeight() - cardH - 10.0f;
  }

  SkRect cardRect = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);
  SkRRect rrect = SkRRect::MakeRectXY(cardRect, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM);

  // Card background (glass)
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(SkColorSetARGB(220, 30, 30, 40));
  canvas->drawRRect(rrect, paint);

  // Card border
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(100, 255, 255, 255));
  canvas->drawRRect(rrect, paint);

  // Title (control name)
  SkFont titleFont;
  titleFont.setSize(14.0f);
  titleFont.setEmbolden(true);

  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(SkColorSetRGB(0, 255, 255));

  juce::String label = target_->getLabel();
  if (label.isEmpty())
    label = "Control";

  std::string titleStr = label.toStdString();
  canvas->drawSimpleText(titleStr.c_str(), titleStr.length(),
                         SkTextEncoding::kUTF8, cardX + 12.0f, cardY + 22.0f,
                         titleFont, paint);

  // Tooltip text
  SkFont bodyFont;
  bodyFont.setSize(12.0f);

  paint.setColor(SkColorSetARGB(200, 220, 220, 230));

  juce::String tooltip = target_->getTooltip();
  if (tooltip.isEmpty())
    tooltip = "No description available.";

  std::string bodyStr = tooltip.toStdString();

  // Simple word wrap (basic implementation)
  float textX = cardX + 12.0f;
  float textY = cardY + 42.0f;
  float maxWidth = cardW - 24.0f;
  float lineHeight = 16.0f;

  juce::StringArray words;
  words.addTokens(tooltip, " ", "");

  juce::String currentLine;
  for (const auto &word : words) {
    juce::String testLine =
        currentLine.isEmpty() ? word : currentLine + " " + word;
    std::string testStr = testLine.toStdString();
    float testWidth = bodyFont.measureText(testStr.c_str(), testStr.length(),
                                           SkTextEncoding::kUTF8);

    if (testWidth > maxWidth && currentLine.isNotEmpty()) {
      // Draw current line and start new one
      std::string lineStr = currentLine.toStdString();
      canvas->drawSimpleText(lineStr.c_str(), lineStr.length(),
                             SkTextEncoding::kUTF8, textX, textY, bodyFont,
                             paint);
      textY += lineHeight;
      currentLine = word;

      // Stop if we're running out of card space
      if (textY > cardY + cardH - 10.0f)
        break;
    } else {
      currentLine = testLine;
    }
  }

  // Draw remaining line
  if (currentLine.isNotEmpty() && textY <= cardY + cardH - 10.0f) {
    std::string lineStr = currentLine.toStdString();
    canvas->drawSimpleText(lineStr.c_str(), lineStr.length(),
                           SkTextEncoding::kUTF8, textX, textY, bodyFont,
                           paint);
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
