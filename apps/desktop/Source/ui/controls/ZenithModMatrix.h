/*
  ==============================================================================

    ZenithModMatrix.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Modulation matrix grid widget for ZenithPolySynth.
    Displays modulation routings as an interactive grid.

  ==============================================================================
*/

#pragma once

#include "../../instruments/ZenithPolySynth.h"
#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>


#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#endif

namespace zenith {

class ZenithModMatrix : public SkiaComponent {
public:
  explicit ZenithModMatrix(ZenithPolySynthProcessor &processor);
  ~ZenithModMatrix() override = default;

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

protected:
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  bool keyPressed(const juce::KeyPress &key) override;

private:
  void updateHover(const juce::MouseEvent &e);
  bool cellFromPosition(juce::Point<float> pos, int& outRow, int& outCol) const;
  void clearHoveredCell();
  juce::Rectangle<float> getGridBounds() const;
  bool rowMatchesFilter(int row) const;
  bool colMatchesFilter(int col) const;

#ifdef ZENITH_USE_SKIA
  void drawGrid(SkCanvas *canvas);
  void drawBackground(SkCanvas* canvas, const juce::Rectangle<float>& bounds);
  void drawGridLabels(SkCanvas* canvas, const juce::Rectangle<float>& gridBounds,
                      float cellWidth, float cellHeight,
                      int numRows, int numCols);
  void drawCell(SkCanvas *canvas, int row, int col, float x, float y,
                float cellWidth, float cellHeight, float value);
  void drawHoveredValueBadge(SkCanvas* canvas, const juce::Rectangle<float>& gridBounds,
                             float cellWidth, float cellHeight);
#endif

  ZenithPolySynthProcessor &processor_;

  int hoverRow_ = -1;
  int hoverCol_ = -1;
  bool isDragging_ = false;
  float startVal_ = 0.0f;
  float dragStartY_ = 0.0f;
  juce::Point<float> lastMousePos_;
  juce::String filterText_;
  bool filterTypingMode_ = false;

  static constexpr float kHeaderWidth_ = 102.0f;
  static constexpr float kHeaderHeight_ = 30.0f;
  static constexpr float kOuterPadding_ = 12.0f;
  static constexpr float kCellPadding_ = 3.0f;

  // Colors
  SkColor positiveColor_ = SkColorSetRGB(0, 230, 120);
  SkColor negativeColor_ = SkColorSetRGB(255, 90, 90);
  SkColor hoverColor_ = SkColorSetARGB(135, 255, 255, 255);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithModMatrix)
};

} // namespace zenith
