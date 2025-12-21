/*
  ==============================================================================

    ZenithLayout.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Layout engine wrapper for Zenith UI components.
    Provides FlexBox-based layout utilities with a fluent API.

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace zenith {

/**
 * @class ZenithLayout
 * @brief Wrapper around JUCE FlexBox for Zenith UI
 */
class ZenithLayout {
public:
  // ==============================================================================
  // Layout Containers
  // ==============================================================================

  /**
   * @brief Arranges components in a horizontal row
   */
  static void
  row(const juce::Rectangle<int> &bounds,
      const std::vector<juce::Component *> &items, float gap = 10.0f,
      juce::FlexBox::JustifyContent justify =
          juce::FlexBox::JustifyContent::flexStart,
      juce::FlexBox::AlignItems align = juce::FlexBox::AlignItems::center) {
    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::row;
    flex.justifyContent = justify;
    flex.alignItems = align;

    for (auto *item : items) {
      if (item) {
        flex.items.add(
            juce::FlexItem(*item).withFlex(1.0f).withMargin(gap / 2.0f));
      }
    }

    flex.performLayout(bounds);
  }

  /**
   * @brief Arranges components in a vertical column
   */
  static void
  column(const juce::Rectangle<int> &bounds,
         const std::vector<juce::Component *> &items, float gap = 10.0f,
         juce::FlexBox::JustifyContent justify =
             juce::FlexBox::JustifyContent::flexStart,
         juce::FlexBox::AlignItems align = juce::FlexBox::AlignItems::center) {
    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::column;
    flex.justifyContent = justify;
    flex.alignItems = align;

    for (auto *item : items) {
      if (item) {
        flex.items.add(
            juce::FlexItem(*item).withFlex(1.0f).withMargin(gap / 2.0f));
      }
    }

    flex.performLayout(bounds);
  }

  /**
   * @brief Arranges components in a grid
   */
  static void grid(const juce::Rectangle<int> &bounds,
                   const std::vector<juce::Component *> &items, int cols,
                   float gapX = 10.0f, float gapY = 10.0f) {
    if (cols <= 0)
      return;

    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::row;
    flex.flexWrap = juce::FlexBox::Wrap::wrap;
    flex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    flex.alignContent = juce::FlexBox::AlignContent::flexStart;

    float itemWidth = (float)bounds.getWidth() / cols;

    for (auto *item : items) {
      if (item) {
        auto margin = juce::FlexItem::Margin(gapY / 2.0f, gapX / 2.0f,
                                             gapY / 2.0f, gapX / 2.0f);
        flex.items.add(juce::FlexItem(*item)
                           .withWidth(itemWidth - gapX)
                           .withHeight(itemWidth - gapX)
                           .withMargin(margin));
      }
    }

    flex.performLayout(bounds);
  }

  // ==============================================================================
  // Fluent API Helper
  // ==============================================================================

  class Builder {
  public:
    Builder() = default;

    Builder &withBounds(const juce::Rectangle<int> &bounds) {
      bounds_ = bounds;
      return *this;
    }
    Builder &withGap(float gap) {
      gap_ = gap;
      return *this;
    }
    Builder &withJustify(juce::FlexBox::JustifyContent justify) {
      justify_ = justify;
      return *this;
    }
    Builder &withAlign(juce::FlexBox::AlignItems align) {
      align_ = align;
      return *this;
    }
    Builder &withItems(const std::vector<juce::Component *> &items) {
      items_ = items;
      return *this;
    }
    Builder &addItem(juce::Component *item) {
      items_.push_back(item);
      return *this;
    }

    // Apply row layout
    void applyRow() {
      ZenithLayout::row(bounds_, items_, gap_, justify_, align_);
    }

    // Apply column layout
    void applyColumn() {
      ZenithLayout::column(bounds_, items_, gap_, justify_, align_);
    }

  private:
    juce::Rectangle<int> bounds_;
    std::vector<juce::Component *> items_;
    float gap_ = 0.0f;
    juce::FlexBox::JustifyContent justify_ =
        juce::FlexBox::JustifyContent::flexStart;
    juce::FlexBox::AlignItems align_ = juce::FlexBox::AlignItems::center;
  };

  static Builder begin() { return Builder(); }
};

} // namespace zenith
