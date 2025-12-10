/*
  ==============================================================================

    ModulationMatrixView.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Universal Modulation Matrix UI Component.
    Displays a grid of Sources (rows) and Destinations (columns).

  ==============================================================================
*/

#pragma once

#include "../../dsp/GlobalLFO.h"
#include "../../engine/MacroControl.h"
#include "../ZenithTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

class Engine;

//==============================================================================
/**
 * @brief A single cell in the modulation matrix
 *
 * Clicking sets/clears modulation, dragging adjusts amount
 */
class ModulationMatrixCell : public juce::Component {
public:
  ModulationMatrixCell() { setRepaintsOnMouseActivity(true); }

  void setAmount(float newAmount) {
    amount_ = juce::jlimit(-1.0f, 1.0f, newAmount);
    repaint();
  }

  float getAmount() const { return amount_; }

  bool isActive() const { return std::abs(amount_) > 0.001f; }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    // Background
    if (isMouseOver())
      g.setColour(juce::Colour(0xff3a3a3a));
    else
      g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Active indicator
    if (isActive()) {
      // Draw modulation amount bar
      float normalizedAmount = (amount_ + 1.0f) * 0.5f; // 0-1 range
      float barWidth = bounds.getWidth() * std::abs(amount_);

      if (amount_ > 0) {
        g.setColour(juce::Colour(0xff00cc88)); // Green for positive
      } else {
        g.setColour(juce::Colour(0xffcc4488)); // Pink for negative
      }

      float centerX = bounds.getCentreX();
      if (amount_ > 0) {
        g.fillRoundedRectangle(centerX, bounds.getY() + 2, barWidth * 0.5f,
                               bounds.getHeight() - 4, 2.0f);
      } else {
        g.fillRoundedRectangle(centerX - barWidth * 0.5f, bounds.getY() + 2,
                               barWidth * 0.5f, bounds.getHeight() - 4, 2.0f);
      }

      // Amount text
      g.setColour(juce::Colours::white);
      g.setFont(10.0f);
      g.drawText(juce::String(static_cast<int>(amount_ * 100)) + "%", bounds,
                 juce::Justification::centred);
    }

    // Border
    g.setColour(juce::Colour(0xff444444));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
  }

  void mouseDown(const juce::MouseEvent &e) override {
    if (e.mods.isRightButtonDown()) {
      // Clear modulation
      setAmount(0.0f);
      if (onAmountChanged)
        onAmountChanged(amount_);
    } else {
      // Toggle or prepare for drag
      dragStartY_ = e.y;
      dragStartAmount_ = amount_;

      if (!isActive()) {
        setAmount(1.0f); // Default to 100%
        if (onAmountChanged)
          onAmountChanged(amount_);
      }
    }
  }

  void mouseDrag(const juce::MouseEvent &e) override {
    float deltaY = (dragStartY_ - e.y) * 0.01f; // Invert Y for natural feel
    setAmount(dragStartAmount_ + deltaY);
    if (onAmountChanged)
      onAmountChanged(amount_);
  }

  std::function<void(float)> onAmountChanged;

private:
  float amount_ = 0.0f;
  float dragStartY_ = 0;
  float dragStartAmount_ = 0;
};

//==============================================================================
/**
 * @brief The Universal Modulation Matrix View
 *
 * Rows: Modulation Sources (LFOs, Macros, Track Envelopes)
 * Columns: Modulation Destinations (Plugin Parameters)
 */
class ModulationMatrixView : public juce::Component, public juce::Timer {
public:
  ModulationMatrixView();
  ~ModulationMatrixView() override;

  void setEngine(Engine *engine);

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  // Refresh the matrix when routing changes
  void refreshMatrix();

private:
  Engine *engine_ = nullptr;

  // Source/Destination labels
  juce::StringArray sourceLabels_;
  juce::StringArray destLabels_;

  // Matrix cells
  juce::OwnedArray<ModulationMatrixCell> cells_;

  // Layout constants
  static constexpr int kCellWidth = 60;
  static constexpr int kCellHeight = 24;
  static constexpr int kLabelWidth = 120;
  static constexpr int kHeaderHeight = 80;

  // Scroll viewport
  std::unique_ptr<juce::Viewport> viewport_;
  std::unique_ptr<juce::Component> matrixContent_;

  void buildSourceLabels();
  void buildDestLabels();
  void createCells();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixView)
};

} // namespace zenith
