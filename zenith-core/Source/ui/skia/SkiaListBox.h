/**
 * @file SkiaListBox.h
 * @brief Beautiful GPU-accelerated list box with native Skia rendering
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


#ifdef ZENITH_USE_SKIA
#include "SkiaComponent.h"
#include "SkiaTextRenderer.h"
#include "SkiaTheme.h"
#include <functional>
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <vector>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @class SkiaListBox
 * @brief GPU-accelerated list box with smooth scrolling and animations
 *
 * Features:
 * - Custom item rendering via callback
 * - Smooth spring-based scrolling
 * - Selection highlighting with animations
 * - Hover effects
 * - Double-click support
 */
class SkiaListBox : public SkiaComponent, private juce::Timer {
public:
  /**
   * @brief Callback for custom item rendering
   * @param canvas Skia canvas to draw on
   * @param bounds Bounds of the item
   * @param rowNumber Row index
   * @param isSelected Whether this row is selected
   * @param isHovered Whether mouse is over this row
   */
  using ItemPaintCallback =
      std::function<void(SkCanvas *canvas, SkRect bounds, int rowNumber,
                         bool isSelected, bool isHovered)>;

  SkiaListBox()
      : numRows_(0), selectedRow_(-1), hoveredRow_(-1), rowHeight_(24.0f),
        scrollOffset_(0.0f), scrollVelocity_(0.0f), selectionAlpha_(0.0f),
        selectionVelocity_(0.0f) {
    setOpaque(false);
    startTimer(16); // 60 FPS
  }

  ~SkiaListBox() override { stopTimer(); }

  //==========================================================================
  // Configuration
  //==========================================================================

  void setNumRows(int numRows) {
    numRows_ = numRows;
    repaint();
  }

  int getNumRows() const { return numRows_; }

  void setRowHeight(float height) {
    rowHeight_ = height;
    repaint();
  }

  float getRowHeight() const { return rowHeight_; }

  void setSelectedRow(int row, bool notify = true) {
    if (selectedRow_ != row) {
      selectedRow_ = row;

      // Scroll to make selected row visible
      if (row >= 0) {
        float rowTop = row * rowHeight_;
        float rowBottom = rowTop + rowHeight_;
        float viewHeight = getHeight();

        if (rowBottom > scrollOffset_ + viewHeight)
          scrollOffset_ = rowBottom - viewHeight;
        else if (rowTop < scrollOffset_)
          scrollOffset_ = rowTop;
      }

      if (notify && onRowSelected)
        onRowSelected(row);

      repaint();
    }
  }

  int getSelectedRow() const { return selectedRow_; }

  void setItemPaintCallback(ItemPaintCallback callback) {
    itemPaintCallback_ = std::move(callback);
    repaint();
  }

  //==========================================================================
  // Callbacks
  //==========================================================================

  std::function<void(int row)> onRowSelected;
  std::function<void(int row)> onRowDoubleClicked;

  //==========================================================================
  // Component overrides
  //==========================================================================

  void mouseDown(const juce::MouseEvent &e) override {
    int row = getRowAtPosition(e.position.y);
    if (row >= 0 && row < numRows_) {
      setSelectedRow(row, true);
    }
  }

  void mouseDoubleClick(const juce::MouseEvent &e) override {
    int row = getRowAtPosition(e.position.y);
    if (row >= 0 && row < numRows_ && onRowDoubleClicked) {
      onRowDoubleClicked(row);
    }
  }

  void mouseMove(const juce::MouseEvent &e) override {
    int row = getRowAtPosition(e.position.y);
    if (hoveredRow_ != row) {
      hoveredRow_ = row;
      repaint();
    }
  }

  void mouseExit(const juce::MouseEvent &) override {
    hoveredRow_ = -1;
    repaint();
  }

  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &wheel) override {
    // Scroll with wheel
    scrollVelocity_ -= wheel.deltaY * 100.0f;
  }

  //==========================================================================
  // SkiaComponent implementation
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override {
    SkRect bounds = SkRect::MakeWH(getWidth(), getHeight());
    const auto &theme = SkiaTheme::getInstance();
    const auto &colors = theme.getColors();

    // Background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(colors.surfaceDefault);
    canvas->drawRect(bounds, bgPaint);

    // Clip to bounds for scrolling
    canvas->save();
    canvas->clipRect(bounds);

    // Calculate visible range
    int firstVisibleRow = static_cast<int>(scrollOffset_ / rowHeight_);
    int lastVisibleRow =
        static_cast<int>((scrollOffset_ + bounds.height()) / rowHeight_) + 1;

    firstVisibleRow = juce::jmax(0, firstVisibleRow);
    lastVisibleRow = juce::jmin(numRows_, lastVisibleRow);

    // Draw visible items
    for (int row = firstVisibleRow; row < lastVisibleRow; ++row) {
      float y = bounds.y() + row * rowHeight_ - scrollOffset_;
      SkRect itemBounds =
          SkRect::MakeXYWH(bounds.x(), y, bounds.width(), rowHeight_);

      bool isSelected = (row == selectedRow_);
      bool isHovered = (row == hoveredRow_);

      // Selection/hover background
      if (isSelected || isHovered) {
        SkPaint highlightPaint;
        highlightPaint.setAntiAlias(true);

        if (isSelected) {
          highlightPaint.setColor(colors.primary);
          highlightPaint.setAlpha(static_cast<uint8_t>(selectionAlpha_ * 100));
        } else if (isHovered) {
          highlightPaint.setColor(colors.surfaceHover);
        }

        canvas->drawRect(itemBounds, highlightPaint);
      }

      // Custom item rendering
      if (itemPaintCallback_) {
        itemPaintCallback_(canvas, itemBounds, row, isSelected, isHovered);
      } else {
        // Default rendering: just show row number
        SkiaTextRenderer textRenderer;
        TextRenderOptions opts;
        opts.color = isSelected ? colors.textOnAccent : colors.textPrimary;
        opts.antiAlias = true;
        opts.effects = TextEffect::None;

        juce::String text = "Row " + juce::String(row);
        float textX = itemBounds.x() + 8.0f;
        float textY = itemBounds.centerY() + 4.0f;

        textRenderer.drawText(canvas, text, textX, textY, TextStyle::Regular,
                              opts);
      }

      // Divider line
      SkPaint dividerPaint;
      dividerPaint.setColor(colors.border);
      dividerPaint.setAlpha(30);
      canvas->drawLine(itemBounds.x(), itemBounds.bottom(), itemBounds.right(),
                       itemBounds.bottom(), dividerPaint);
    }

    canvas->restore();

    // Scrollbar (if needed)
    if (numRows_ * rowHeight_ > bounds.height()) {
      float scrollbarWidth = 6.0f;
      float contentHeight = numRows_ * rowHeight_;
      float visibleRatio = bounds.height() / contentHeight;
      float scrollbarHeight = bounds.height() * visibleRatio;
      float scrollbarY =
          bounds.y() + (scrollOffset_ / contentHeight) * bounds.height();

      SkRect scrollbarBounds =
          SkRect::MakeXYWH(bounds.right() - scrollbarWidth - 2, scrollbarY,
                           scrollbarWidth, scrollbarHeight);

      SkPaint scrollbarPaint;
      scrollbarPaint.setAntiAlias(true);
      scrollbarPaint.setColor(colors.border);
      scrollbarPaint.setAlpha(100);

      SkRRect scrollbarRRect = SkRRect::MakeRectXY(scrollbarBounds, 3.0f, 3.0f);
      canvas->drawRRect(scrollbarRRect, scrollbarPaint);
    }
  }

private:
  void timerCallback() override {
    const float dt = 0.016f;
    const float stiffness = 300.0f;
    const float damping = 20.0f;

    bool needsRepaint = false;

    // Selection animation
    float selectionTarget = (selectedRow_ >= 0) ? 1.0f : 0.0f;
    if (std::abs(selectionAlpha_ - selectionTarget) > 0.001f) {
      float force = -stiffness * (selectionAlpha_ - selectionTarget) -
                    damping * selectionVelocity_;
      selectionVelocity_ += force * dt;
      selectionAlpha_ += selectionVelocity_ * dt;
      selectionAlpha_ = juce::jlimit(0.0f, 1.0f, selectionAlpha_);
      needsRepaint = true;
    }

    // Scroll physics
    if (std::abs(scrollVelocity_) > 0.1f) {
      scrollOffset_ += scrollVelocity_ * dt;

      // Clamp scroll offset
      float maxScroll = juce::jmax(0.0f, numRows_ * rowHeight_ - getHeight());
      if (scrollOffset_ < 0) {
        scrollOffset_ = 0;
        scrollVelocity_ = 0;
      } else if (scrollOffset_ > maxScroll) {
        scrollOffset_ = maxScroll;
        scrollVelocity_ = 0;
      }

      // Apply friction
      scrollVelocity_ *= 0.92f;
      needsRepaint = true;
    } else {
      scrollVelocity_ = 0;
    }

    if (needsRepaint)
      repaint();
  }

  int getRowAtPosition(float y) const {
    float adjustedY = y + scrollOffset_;
    return static_cast<int>(adjustedY / rowHeight_);
  }

  int numRows_;
  int selectedRow_;
  int hoveredRow_;
  float rowHeight_;
  float scrollOffset_;
  float scrollVelocity_;
  float selectionAlpha_;
  float selectionVelocity_;

  ItemPaintCallback itemPaintCallback_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaListBox)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
