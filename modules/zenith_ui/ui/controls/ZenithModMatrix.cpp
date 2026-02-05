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

    ZenithModMatrix.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the modulation matrix widget.

  ==============================================================================

*/

#include "ZenithModMatrix.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#endif

namespace zenith {

ZenithModMatrix::ZenithModMatrix(ZenithPolySynthProcessor &processor)
    : processor_(processor) {}

void ZenithModMatrix::mouseMove(const juce::MouseEvent &e) { updateHover(e); }

void ZenithModMatrix::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hoverRow_ = -1;
  hoverCol_ = -1;
  repaint();
}

void ZenithModMatrix::mouseDown(const juce::MouseEvent &e) {
  updateHover(e);

  if (hoverRow_ >= 0 && hoverCol_ >= 0) {
    isDragging_ = true;
    lastMouseY_ = static_cast<float>(e.position.y);

    ModulationSource src = static_cast<ModulationSource>(hoverRow_ + 1);
    ModulationDestination dst =
        static_cast<ModulationDestination>(hoverCol_ + 1);
    startVal_ = processor_.getModulationMatrix(src, dst);
  }
}

void ZenithModMatrix::mouseDrag(const juce::MouseEvent &e) {
  if (isDragging_ && hoverRow_ >= 0 && hoverCol_ >= 0) {
    float diff = (lastMouseY_ - static_cast<float>(e.position.y)) * 0.01f;
    float newVal = juce::jlimit(-1.0f, 1.0f, startVal_ + diff);

    ModulationSource src = static_cast<ModulationSource>(hoverRow_ + 1);
    ModulationDestination dst =
        static_cast<ModulationDestination>(hoverCol_ + 1);
    processor_.setModulationMatrix(src, dst, newVal);
    repaint();
  }
}

void ZenithModMatrix::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isDragging_ = false;
}

void ZenithModMatrix::updateHover(const juce::MouseEvent &e) {
  auto bounds = getLocalBounds();
  int numRows = static_cast<int>(ModulationSource::NumSources) - 1;
  int numCols = static_cast<int>(ModulationDestination::NumDestinations) - 1;

  if (numRows <= 0 || numCols <= 0) {
    hoverRow_ = -1;
    hoverCol_ = -1;
    return;
  }

  float cellWidth =
      static_cast<float>(bounds.getWidth()) / static_cast<float>(numCols);
  float cellHeight =
      static_cast<float>(bounds.getHeight()) / static_cast<float>(numRows);

  int r = static_cast<int>(e.position.y / cellHeight);
  int c = static_cast<int>(e.position.x / cellWidth);

  if (r >= 0 && r < numRows && c >= 0 && c < numCols) {
    if (hoverRow_ != r || hoverCol_ != c) {
      hoverRow_ = r;
      hoverCol_ = c;
      repaint();
    }
  } else {
    hoverRow_ = -1;
    hoverCol_ = -1;
    repaint();
  }
}

void ZenithModMatrix::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  auto bounds = getLocalBounds().toFloat();

  SkPaint paint;
  paint.setAntiAlias(true);

  // Background
  paint.setColor(SkColorSetARGB(30, 0, 0, 0));
  canvas->drawRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                    bounds.getWidth(), bounds.getHeight()),
                   paint);

  drawGrid(canvas);
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

void ZenithModMatrix::drawGrid(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  int numRows = static_cast<int>(ModulationSource::NumSources) - 1;
  int numCols = static_cast<int>(ModulationDestination::NumDestinations) - 1;

  if (numRows <= 0 || numCols <= 0) {
    // Draw empty state
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));

    SkFont font;
    font.setSize(14.0f);
    canvas->drawString("No modulation sources configured", 10.0f,
                       bounds.getCentreY(), font, paint);
    return;
  }

  float cellWidth = bounds.getWidth() / static_cast<float>(numCols);
  float cellHeight = bounds.getHeight() / static_cast<float>(numRows);

  for (int r = 0; r < numRows; ++r) {
    for (int c = 0; c < numCols; ++c) {
      ModulationSource src = static_cast<ModulationSource>(r + 1);
      ModulationDestination dst = static_cast<ModulationDestination>(c + 1);

      float val = processor_.getModulationMatrix(src, dst);
      float x = c * cellWidth;
      float y = r * cellHeight;

      drawCell(canvas, r, c, x, y, cellWidth, cellHeight, val);
    }
  }
}

void ZenithModMatrix::drawCell(SkCanvas *canvas, int row, int col, float x,
                               float y, float cellWidth, float cellHeight,
                               float value) {
  float cx = x + cellWidth * 0.5f;
  float cy = y + cellHeight * 0.5f;
  float radius = std::min(cellWidth, cellHeight) * 0.3f;

  SkPaint paint;
  paint.setAntiAlias(true);

  // Empty dot
  paint.setColor(SkColorSetARGB(50, 255, 255, 255));
  canvas->drawCircle(cx, cy, 2.0f, paint);

  // Active value indicator
  if (std::abs(value) > 0.01f) {
    SkColor color = value > 0 ? positiveColor_ : negativeColor_;
    paint.setColor(color);

    float amount = std::abs(value);
    canvas->drawCircle(cx, cy, radius * amount + 2.0f, paint);

    // Glow
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 5.0f));

    canvas->drawCircle(cx, cy, radius * amount + 2.0f, paint);
    paint.setMaskFilter(nullptr);
  }

  // Hover highlight
  if (hoverRow_ == row && hoverCol_ == col) {
    paint.setColor(hoverColor_);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    canvas->drawRect(SkRect::MakeXYWH(x, y, cellWidth, cellHeight), paint);
  }
}

void ZenithModMatrix::drawHeaders(SkCanvas *canvas) {
  // Headers could be drawn if needed
  juce::ignoreUnused(canvas);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
