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
      bounds_ = bounds.toFloat();
      return *this;
    }
    
    Builder &withFloatBounds(const juce::Rectangle<float> &bounds) {
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

    // Enhanced component-based add
    Builder &addItem(juce::Component *item, float flex = 1.0f) {
      if (item) {
         flexItems_.push_back(juce::FlexItem(*item).withFlex(flex));
         componentMap_.push_back(item); 
      }
      return *this;
    }

    Builder &addFixedItem(juce::Component *item, float width, float height) {
      if (item) {
         flexItems_.push_back(juce::FlexItem(*item).withWidth(width).withHeight(height));
         componentMap_.push_back(item);
      }
      return *this;
    }

    // Generic FlexItem add
    Builder &addFlexItem(juce::FlexItem item) {
      flexItems_.push_back(item);
      componentMap_.push_back(nullptr); // No associated component to auto-resize
      return *this;
    }

    // Apply row layout to components
    void applyRow() {
        performLayout(juce::FlexBox::Direction::row);
    }

    // Apply column layout to components
    void applyColumn() {
        performLayout(juce::FlexBox::Direction::column);
    }

    // Calculate and return bounds (robust mode)
    std::vector<juce::Rectangle<float>> layout(juce::FlexBox::Direction direction) {
        juce::FlexBox flex;
        flex.flexDirection = direction;
        flex.justifyContent = justify_;
        flex.alignItems = align_;

        // Apply gap if items don't have custom margins
        // Note: Logic here tries to respect previous simple "gap" param 
        // while allowing complex FlexItems. 
        for (auto& item : flexItems_) {
            if (gap_ > 0.0f && item.margin.left == 0 && item.margin.right == 0 && 
                item.margin.top == 0 && item.margin.bottom == 0) {
                 item.withMargin(gap_ / 2.0f);
            }
            flex.items.add(item);
        }

        flex.performLayout(bounds_);

        std::vector<juce::Rectangle<float>> results;
        for (const auto& item : flex.items) {
            results.push_back(item.currentBounds);
        }
        return results;
    }

  private:
    void performLayout(juce::FlexBox::Direction direction) {
        auto rects = layout(direction);
        for (size_t i = 0; i < rects.size() && i < componentMap_.size(); ++i) {
            if (auto* comp = componentMap_[i]) {
                comp->setBounds(rects[i].toNearestInt());
            }
        }
    }

    juce::Rectangle<float> bounds_;
    std::vector<juce::FlexItem> flexItems_;
    std::vector<juce::Component*> componentMap_;
    float gap_ = 0.0f;
    juce::FlexBox::JustifyContent justify_ =
        juce::FlexBox::JustifyContent::flexStart;
    juce::FlexBox::AlignItems align_ = juce::FlexBox::AlignItems::center;
  };

  static Builder begin() { return Builder(); }
};

} // namespace zenith
