/*
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
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithModMatrix::ZenithModMatrix(ZenithPolySynthProcessor &processor)
    : processor_(processor) {
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
}

void ZenithModMatrix::mouseMove(const juce::MouseEvent &e) { updateHover(e); }

void ZenithModMatrix::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  clearHoveredCell();
}

void ZenithModMatrix::mouseDown(const juce::MouseEvent &e) {
  grabKeyboardFocus();
  lastMousePos_ = e.position;
  updateHover(e);

  if (hoverRow_ >= 0 && hoverCol_ >= 0) {
    isDragging_ = true;
    dragStartY_ = static_cast<float>(e.position.y);

    ModulationSource src = static_cast<ModulationSource>(hoverRow_ + 1);
    ModulationDestination dst =
        static_cast<ModulationDestination>(hoverCol_ + 1);
    startVal_ = processor_.getModulationMatrix(src, dst);

    if (e.mods.isRightButtonDown()) {
      processor_.setModulationMatrix(src, dst, 0.0f);
      isDragging_ = false;
      repaint();
    }
  }
}

void ZenithModMatrix::mouseDrag(const juce::MouseEvent &e) {
  lastMousePos_ = e.position;
  if (isDragging_ && hoverRow_ >= 0 && hoverCol_ >= 0) {
    float diff = (dragStartY_ - static_cast<float>(e.position.y)) * 0.0085f;
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

bool ZenithModMatrix::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress('f', juce::ModifierKeys::commandModifier, 0) ||
      key == juce::KeyPress('f', juce::ModifierKeys::ctrlModifier, 0)) {
    filterTypingMode_ = true;
    repaint();
    return true;
  }

  if (key.isKeyCode(juce::KeyPress::escapeKey)) {
    filterTypingMode_ = false;
    if (filterText_.isNotEmpty()) {
      filterText_.clear();
      repaint();
    }
    return true;
  }

  if (key.isKeyCode(juce::KeyPress::returnKey)) {
    filterTypingMode_ = false;
    repaint();
    return true;
  }

  if (key.isKeyCode(juce::KeyPress::backspaceKey)) {
    if (filterText_.isNotEmpty()) {
      filterText_ = filterText_.dropLastCharacters(1);
      repaint();
      return true;
    }
  }

  const juce::juce_wchar ch = key.getTextCharacter();
  if (ch >= 32 && ch < 127) {
    filterText_ += juce::String::charToString(ch);
    filterTypingMode_ = true;
    repaint();
    return true;
  }

  return false;
}

void ZenithModMatrix::clearHoveredCell() {
  hoverRow_ = -1;
  hoverCol_ = -1;
  repaint();
}

juce::Rectangle<float> ZenithModMatrix::getGridBounds() const {
  auto bounds = getLocalBounds().toFloat().reduced(kOuterPadding_);
  bounds.removeFromLeft(kHeaderWidth_);
  bounds.removeFromTop(kHeaderHeight_);
  return bounds;
}

bool ZenithModMatrix::rowMatchesFilter(int row) const {
  if (filterText_.trim().isEmpty()) {
    return true;
  }
  const juce::String query = filterText_.toLowerCase().trim();
  juce::String label;
  switch (static_cast<ModulationSource>(row + 1)) {
    case ModulationSource::LFO1: label = "LFO 1"; break;
    case ModulationSource::LFO2: label = "LFO 2"; break;
    case ModulationSource::Env1: label = "Env 1"; break;
    case ModulationSource::Env2: label = "Env 2"; break;
    case ModulationSource::Velocity: label = "Velocity"; break;
    case ModulationSource::ModWheel: label = "Mod Wheel"; break;
    case ModulationSource::Aftertouch: label = "Aftertouch"; break;
    case ModulationSource::Timbre: label = "Timbre"; break;
    default: label = "--"; break;
  }
  return label.toLowerCase().contains(query);
}

bool ZenithModMatrix::colMatchesFilter(int col) const {
  if (filterText_.trim().isEmpty()) {
    return true;
  }
  const juce::String query = filterText_.toLowerCase().trim();
  juce::String label;
  switch (static_cast<ModulationDestination>(col + 1)) {
    case ModulationDestination::Osc1Pitch: label = "O1 Pitch"; break;
    case ModulationDestination::Osc2Pitch: label = "O2 Pitch"; break;
    case ModulationDestination::Osc3Pitch: label = "O3 Pitch"; break;
    case ModulationDestination::Osc1Mix: label = "O1 Mix"; break;
    case ModulationDestination::Osc2Mix: label = "O2 Mix"; break;
    case ModulationDestination::Osc3Mix: label = "O3 Mix"; break;
    case ModulationDestination::FilterCutoff: label = "Cutoff"; break;
    case ModulationDestination::FilterResonance: label = "Res"; break;
    case ModulationDestination::AmpGain: label = "Amp"; break;
    case ModulationDestination::Osc1Shape: label = "O1 Shape"; break;
    case ModulationDestination::Osc2Shape: label = "O2 Shape"; break;
    case ModulationDestination::Osc3Shape: label = "O3 Shape"; break;
    case ModulationDestination::LFO1Rate: label = "LFO1 Rate"; break;
    case ModulationDestination::LFO2Rate: label = "LFO2 Rate"; break;
    default: label = "--"; break;
  }
  return label.toLowerCase().contains(query);
}

bool ZenithModMatrix::cellFromPosition(juce::Point<float> pos, int& outRow, int& outCol) const {
  int numRows = static_cast<int>(ModulationSource::NumSources) - 1;
  int numCols = static_cast<int>(ModulationDestination::NumDestinations) - 1;
  if (numRows <= 0 || numCols <= 0) {
    return false;
  }

  auto gridBounds = getGridBounds();
  if (!gridBounds.contains(pos)) {
    return false;
  }

  float cellWidth = gridBounds.getWidth() / static_cast<float>(numCols);
  float cellHeight = gridBounds.getHeight() / static_cast<float>(numRows);

  const int row = static_cast<int>((pos.y - gridBounds.getY()) / cellHeight);
  const int col = static_cast<int>((pos.x - gridBounds.getX()) / cellWidth);
  if (row < 0 || row >= numRows || col < 0 || col >= numCols) {
    return false;
  }

  outRow = row;
  outCol = col;
  return true;
}

void ZenithModMatrix::updateHover(const juce::MouseEvent &e) {
  lastMousePos_ = e.position;
  int r = -1;
  int c = -1;
  if (cellFromPosition(e.position, r, c)) {
    if (hoverRow_ != r || hoverCol_ != c) {
      hoverRow_ = r;
      hoverCol_ = c;
      repaint();
    }
  } else {
    clearHoveredCell();
  }
}

void ZenithModMatrix::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  auto bounds = getLocalBounds().toFloat();

  drawBackground(canvas, bounds);

  drawGrid(canvas);
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

namespace {
const char* sourceLabelForRow(int row) {
  switch (static_cast<ModulationSource>(row + 1)) {
    case ModulationSource::LFO1: return "LFO 1";
    case ModulationSource::LFO2: return "LFO 2";
    case ModulationSource::Env1: return "Env 1";
    case ModulationSource::Env2: return "Env 2";
    case ModulationSource::Velocity: return "Velocity";
    case ModulationSource::ModWheel: return "Mod Wheel";
    case ModulationSource::Aftertouch: return "Aftertouch";
    case ModulationSource::Timbre: return "Timbre";
    default: return "--";
  }
}

const char* destinationLabelForCol(int col) {
  switch (static_cast<ModulationDestination>(col + 1)) {
    case ModulationDestination::Osc1Pitch: return "O1 Pitch";
    case ModulationDestination::Osc2Pitch: return "O2 Pitch";
    case ModulationDestination::Osc3Pitch: return "O3 Pitch";
    case ModulationDestination::Osc1Mix: return "O1 Mix";
    case ModulationDestination::Osc2Mix: return "O2 Mix";
    case ModulationDestination::Osc3Mix: return "O3 Mix";
    case ModulationDestination::FilterCutoff: return "Cutoff";
    case ModulationDestination::FilterResonance: return "Res";
    case ModulationDestination::AmpGain: return "Amp";
    case ModulationDestination::Osc1Shape: return "O1 Shape";
    case ModulationDestination::Osc2Shape: return "O2 Shape";
    case ModulationDestination::Osc3Shape: return "O3 Shape";
    case ModulationDestination::LFO1Rate: return "LFO1 Rate";
    case ModulationDestination::LFO2Rate: return "LFO2 Rate";
    default: return "--";
  }
}
} // namespace

void ZenithModMatrix::drawBackground(SkCanvas* canvas, const juce::Rectangle<float>& bounds) {
  SkColor colors[] = {
      SkColorSetRGB(18, 24, 36),
      SkColorSetRGB(14, 18, 30),
  };
  SkPoint points[] = {
      {bounds.getX(), bounds.getY()},
      {bounds.getRight(), bounds.getBottom()},
  };

  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setShader(SkGradientShader::MakeLinear(points, colors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawRoundRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()),
                        10.0f, 10.0f, bgPaint);

  bgPaint.setShader(nullptr);
  bgPaint.setStyle(SkPaint::kStroke_Style);
  bgPaint.setStrokeWidth(1.0f);
  bgPaint.setColor(SkColorSetARGB(70, 220, 235, 255));
  canvas->drawRoundRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()),
                        10.0f, 10.0f, bgPaint);

  const juce::String filterDisplay = filterText_.trim().isEmpty()
                                         ? "Filter: Ctrl/Cmd+F"
                                         : "Filter: " + filterText_;
  SkFont filterFont;
  filterFont.setSize(10.5f);
  SkPaint filterPaint;
  filterPaint.setAntiAlias(true);
  filterPaint.setColor(filterTypingMode_ ? SkColorSetARGB(235, 210, 235, 255)
                                         : SkColorSetARGB(170, 180, 210, 238));
  const std::string filterStd = filterDisplay.toStdString();
  const float textW = filterFont.measureText(filterStd.c_str(), filterStd.size(),
                                             SkTextEncoding::kUTF8);
  canvas->drawString(filterStd.c_str(), bounds.getRight() - textW - 10.0f,
                     bounds.getY() + 16.0f, filterFont, filterPaint);
}

void ZenithModMatrix::drawGrid(SkCanvas *canvas) {
  auto gridBounds = getGridBounds();

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
                       gridBounds.getCentreY(), font, paint);
    return;
  }

  float cellWidth = gridBounds.getWidth() / static_cast<float>(numCols);
  float cellHeight = gridBounds.getHeight() / static_cast<float>(numRows);

  drawGridLabels(canvas, gridBounds, cellWidth, cellHeight, numRows, numCols);

  for (int r = 0; r < numRows; ++r) {
    for (int c = 0; c < numCols; ++c) {
      ModulationSource src = static_cast<ModulationSource>(r + 1);
      ModulationDestination dst = static_cast<ModulationDestination>(c + 1);

      float val = processor_.getModulationMatrix(src, dst);
      float x = gridBounds.getX() + c * cellWidth;
      float y = gridBounds.getY() + r * cellHeight;

      drawCell(canvas, r, c, x, y, cellWidth, cellHeight, val);
    }
  }

  drawHoveredValueBadge(canvas, gridBounds, cellWidth, cellHeight);
}

void ZenithModMatrix::drawGridLabels(SkCanvas* canvas,
                                     const juce::Rectangle<float>& gridBounds,
                                     float cellWidth,
                                     float cellHeight,
                                     int numRows,
                                     int numCols) {
  SkFont rowFont;
  rowFont.setSize(11.0f);
  SkFont colFont;
  colFont.setSize(10.0f);

  SkPaint labelPaint;
  labelPaint.setAntiAlias(true);
  labelPaint.setColor(SkColorSetARGB(185, 225, 235, 255));

  for (int r = 0; r < numRows; ++r) {
    const float y = gridBounds.getY() + r * cellHeight + cellHeight * 0.62f;
    const bool rowMatch = rowMatchesFilter(r);
    labelPaint.setColor(rowMatch ? SkColorSetARGB(185, 225, 235, 255)
                                 : SkColorSetARGB(85, 145, 155, 175));
    canvas->drawString(sourceLabelForRow(r), kOuterPadding_ + 4.0f, y, rowFont, labelPaint);
  }

  for (int c = 0; c < numCols; ++c) {
    const float x = gridBounds.getX() + c * cellWidth + 6.0f;
    const float y = kOuterPadding_ + 18.0f;
    const bool colMatch = colMatchesFilter(c);
    labelPaint.setColor(colMatch ? SkColorSetARGB(185, 225, 235, 255)
                                 : SkColorSetARGB(85, 145, 155, 175));
    canvas->drawString(destinationLabelForCol(c), x, y, colFont, labelPaint);
  }
}

void ZenithModMatrix::drawCell(SkCanvas *canvas, int row, int col, float x,
                               float y, float cellWidth, float cellHeight,
                               float value) {
  const bool rowMatch = rowMatchesFilter(row);
  const bool colMatch = colMatchesFilter(col);
  const bool showAsFilteredOut = !(rowMatch || colMatch);

  const float contentX = x + kCellPadding_;
  const float contentY = y + kCellPadding_;
  const float contentW = cellWidth - kCellPadding_ * 2.0f;
  const float contentH = cellHeight - kCellPadding_ * 2.0f;

  SkPaint gridPaint;
  gridPaint.setAntiAlias(true);
  gridPaint.setColor(showAsFilteredOut ? SkColorSetARGB(14, 90, 110, 130)
                                       : SkColorSetARGB(30, 150, 190, 235));
  gridPaint.setStyle(SkPaint::kStroke_Style);
  gridPaint.setStrokeWidth(1.0f);
  canvas->drawRoundRect(SkRect::MakeXYWH(contentX, contentY, contentW, contentH), 4.0f, 4.0f, gridPaint);

  float cx = x + cellWidth * 0.5f;
  float cy = y + cellHeight * 0.5f;
  float radius = std::min(cellWidth, cellHeight) * 0.3f;

  SkPaint paint;
  paint.setAntiAlias(true);

  // Empty dot
  paint.setColor(showAsFilteredOut ? SkColorSetARGB(28, 180, 190, 210)
                                   : SkColorSetARGB(50, 255, 255, 255));
  canvas->drawCircle(cx, cy, 2.0f, paint);

  // Active value indicator
  if (std::abs(value) > 0.01f) {
    SkColor color = value > 0 ? positiveColor_ : negativeColor_;
    if (showAsFilteredOut) {
      color = SkColorSetARGB(95, SkColorGetR(color), SkColorGetG(color),
                             SkColorGetB(color));
    }
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
    paint.setStrokeWidth(1.2f);
    canvas->drawRoundRect(SkRect::MakeXYWH(contentX, contentY, contentW, contentH), 4.0f, 4.0f, paint);
  }
}

void ZenithModMatrix::drawHoveredValueBadge(SkCanvas* canvas,
                                            const juce::Rectangle<float>& gridBounds,
                                            float cellWidth,
                                            float cellHeight) {
  if (hoverRow_ < 0 || hoverCol_ < 0) {
    return;
  }

  const auto src = static_cast<ModulationSource>(hoverRow_ + 1);
  const auto dst = static_cast<ModulationDestination>(hoverCol_ + 1);
  const float value = processor_.getModulationMatrix(src, dst);

  juce::String text = juce::String(sourceLabelForRow(hoverRow_)) + " -> " +
                      juce::String(destinationLabelForCol(hoverCol_)) + "  " +
                      juce::String(value, 2);

  SkFont font;
  font.setSize(11.5f);
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(SkColorSetRGB(230, 240, 255));

  std::string str = text.toStdString();
  const float textWidth = font.measureText(str.c_str(), str.size(), SkTextEncoding::kUTF8);

  const float badgeW = textWidth + 18.0f;
  const float badgeH = 22.0f;
  float badgeX = gridBounds.getX() + hoverCol_ * cellWidth + (cellWidth - badgeW) * 0.5f;
  float badgeY = gridBounds.getY() + hoverRow_ * cellHeight - badgeH - 6.0f;
  badgeX = juce::jlimit(gridBounds.getX(), gridBounds.getRight() - badgeW, badgeX);
  badgeY = juce::jmax(kOuterPadding_ + 2.0f, badgeY);

  SkPaint badgePaint;
  badgePaint.setAntiAlias(true);
  badgePaint.setColor(SkColorSetARGB(210, 9, 14, 24));
  canvas->drawRoundRect(SkRect::MakeXYWH(badgeX, badgeY, badgeW, badgeH), 6.0f, 6.0f, badgePaint);

  badgePaint.setStyle(SkPaint::kStroke_Style);
  badgePaint.setStrokeWidth(1.0f);
  badgePaint.setColor(SkColorSetARGB(120, 200, 225, 255));
  canvas->drawRoundRect(SkRect::MakeXYWH(badgeX, badgeY, badgeW, badgeH), 6.0f, 6.0f, badgePaint);

  canvas->drawString(str.c_str(), badgeX + 9.0f, badgeY + 15.0f, font, textPaint);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
