/*
  ==============================================================================

    ZenithModMatrix.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the modulation matrix widget.

  ==============================================================================
*/

#include "ZenithModMatrix.h"

#include "ZenithSkia.h"

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
}


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


} // namespace zenith
