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

private:
  void updateHover(const juce::MouseEvent &e);

#ifdef ZENITH_USE_SKIA
  void drawGrid(SkCanvas *canvas);
  void drawCell(SkCanvas *canvas, int row, int col, float x, float y,
                float cellWidth, float cellHeight, float value);
  void drawHeaders(SkCanvas *canvas);
#endif

  ZenithPolySynthProcessor &processor_;

  int hoverRow_ = -1;
  int hoverCol_ = -1;
  bool isDragging_ = false;
  float lastMouseY_ = 0.0f;
  float startVal_ = 0.0f;

  // Colors
  SkColor positiveColor_ = SkColorSetRGB(0, 255, 100);
  SkColor negativeColor_ = SkColorSetRGB(255, 50, 50);
  SkColor hoverColor_ = SkColorSetARGB(100, 255, 255, 255);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithModMatrix)
};

} // namespace zenith
