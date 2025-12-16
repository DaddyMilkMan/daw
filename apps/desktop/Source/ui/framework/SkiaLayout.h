/*
  ==============================================================================

    SkiaLayout.h
    Created: 2025-12-07
    Author:  AI Assistant

    Unified layout management system for Skia components

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {

namespace layout {

// ============================================================================
// Layout Constraints (similar to CSS Flexbox)
// ============================================================================

enum class Alignment {
  Start,  // Left/Top
  Center, // Middle
  End,    // Right/Bottom
  Stretch // Fill available space
};

enum class Direction { Horizontal, Vertical };

struct Spacing {
  float top = 0;
  float right = 0;
  float bottom = 0;
  float left = 0;

  Spacing() = default;
  Spacing(float all) : top(all), right(all), bottom(all), left(all) {}
  Spacing(float vertical, float horizontal)
      : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
  Spacing(float top, float right, float bottom, float left)
      : top(top), right(right), bottom(bottom), left(left) {}
};

struct LayoutParams {
  Alignment alignment = Alignment::Stretch;
  float flex = 0.0f;          // 0 = fixed size, >0 = flexible size
  float minSize = 0.0f;       // Minimum size in pixels
  float maxSize = FLT_MAX;    // Maximum size in pixels
  float preferredSize = 0.0f; // Preferred size in pixels
  Spacing margin;             // Outer spacing
  Spacing padding;            // Inner spacing

  LayoutParams() = default;
  LayoutParams(float flex) : flex(flex) {}
  LayoutParams(float flex, Alignment align) : flex(flex), alignment(align) {}
};

// ============================================================================
// Base Layout Container
// ============================================================================

class SkiaLayoutContainer : public SkiaComponent {
public:
  SkiaLayoutContainer();
  ~SkiaLayoutContainer() override;

  // Child management
  void addChild(SkiaComponent *child,
                const LayoutParams &params = LayoutParams());
  void addChild(SkiaComponent *child, float flex,
                Alignment alignment = Alignment::Stretch);
  void removeChild(SkiaComponent *child);
  void removeAllChildren();

  // Layout properties
  void setSpacing(float spacing);
  void setSpacing(float horizontal, float vertical);
  void setPadding(const Spacing &padding);
  void setDirection(Direction direction);

  // Alignment
  void setAlignment(Alignment alignment);
  void setChildAlignment(SkiaComponent *child, Alignment alignment);

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDrag(const juce::MouseEvent &e) override;

protected:
  struct ChildInfo {
    SkiaComponent *component;
    LayoutParams params;
  };

  juce::Array<ChildInfo> children_;
  Direction direction_ = Direction::Vertical;
  Alignment alignment_ = Alignment::Start;
  Spacing spacing_ = Spacing(design::spacing::SM);
  Spacing padding_ = Spacing(design::spacing::MD);

  // Layout calculation
  virtual void calculateLayout();
  juce::Rectangle<float>
  getChildBounds(const ChildInfo &child,
                 const juce::Rectangle<float> &availableBounds, float flexTotal,
                 float availableSpace);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaLayoutContainer)
};

// ============================================================================
// Horizontal Layout
// ============================================================================

class SkiaHorizontalLayout : public SkiaLayoutContainer {
public:
  SkiaHorizontalLayout();
  ~SkiaHorizontalLayout() override;

  // Convenience methods
  void addChild(SkiaComponent *child, float flex = 0.0f,
                Alignment verticalAlignment = Alignment::Center);
};

// ============================================================================
// Vertical Layout
// ============================================================================

class SkiaVerticalLayout : public SkiaLayoutContainer {
public:
  SkiaVerticalLayout();
  ~SkiaVerticalLayout() override;

  // Convenience methods
  void addChild(SkiaComponent *child, float flex = 0.0f,
                Alignment horizontalAlignment = Alignment::Center);
};

// ============================================================================
// Grid Layout
// ============================================================================

class SkiaGridLayout : public SkiaLayoutContainer {
public:
  SkiaGridLayout(int rows, int columns);
  ~SkiaGridLayout() override;

  // Grid configuration
  void setRowSpacing(float spacing);
  void setColumnSpacing(float spacing);
  void setRowFlex(int row, float flex);
  void setColumnFlex(int column, float flex);
  void setCellPadding(const Spacing &padding);

  // Add child at specific grid position
  void addChildAt(SkiaComponent *child, int row, int column, int rowSpan = 1,
                  int columnSpan = 1,
                  const LayoutParams &params = LayoutParams());

private:
  int rows_;
  int columns_;
  juce::Array<float> rowFlex_;
  juce::Array<float> columnFlex_;
  float rowSpacing_ = design::spacing::SM;
  float columnSpacing_ = design::spacing::SM;
  Spacing cellPadding_ = Spacing(design::spacing::XS);

  struct GridCell {
    SkiaComponent *component = nullptr;
    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
    LayoutParams params;
  };

  juce::Array<GridCell> gridChildren_;

  void calculateLayout() override;
  juce::Rectangle<float> getCellBounds(int row, int column, int rowSpan,
                                       int columnSpan,
                                       const juce::Rectangle<float> &gridBounds,
                                       const juce::Array<float> &rowHeights,
                                       const juce::Array<float> &columnWidths);
};

// ============================================================================
// Stack Layout (Overlay)
// ============================================================================

class SkiaStackLayout : public SkiaLayoutContainer {
public:
  SkiaStackLayout();
  ~SkiaStackLayout() override;

  // Add child that fills the entire container
  void addChild(SkiaComponent *child,
                Alignment horizontalAlignment = Alignment::Stretch,
                Alignment verticalAlignment = Alignment::Stretch);

private:
  void calculateLayout() override;
};

// ============================================================================
// Scroll Layout
// ============================================================================

class SkiaScrollLayout : public SkiaLayoutContainer {
public:
  SkiaScrollLayout(Direction scrollDirection = Direction::Vertical);
  ~SkiaScrollLayout() override;

  // Scroll properties
  void setScrollEnabled(bool enabled);
  void scrollTo(float position);
  void scrollToComponent(SkiaComponent *component);

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

private:
  Direction scrollDirection_;
  bool scrollEnabled_ = true;
  float scrollPosition_ = 0.0f;
  float contentSize_ = 0.0f;
  float dragStartPosition_ = 0.0f;

  void calculateLayout() override;
  void updateScrollbars();
};

// ============================================================================
// Layout Utilities
// ============================================================================

class LayoutUtils {
public:
  // Calculate flex distribution
  static juce::Array<float> calculateFlexDistribution(
      const juce::Array<float> &flexValues, float availableSpace,
      const juce::Array<float> &minSizes, const juce::Array<float> &maxSizes,
      const juce::Array<float> &preferredSizes);

  // Align items within available space
  static float alignItem(float itemSize, float availableSize,
                         Alignment alignment);

  // Apply padding and margin
  static juce::Rectangle<float>
  applyPadding(const juce::Rectangle<float> &bounds, const Spacing &padding);
  static juce::Rectangle<float>
  applyMargin(const juce::Rectangle<float> &bounds, const Spacing &margin);

  // Measure text size
  static juce::Rectangle<float> measureText(const SkFont &font,
                                            const juce::String &text);

  // Calculate total flex
  static float calculateTotalFlex(const juce::Array<float> &flexValues);
};

} // namespace layout
} // namespace zenith