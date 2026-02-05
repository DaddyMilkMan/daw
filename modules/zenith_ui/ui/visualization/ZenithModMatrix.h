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
