/**
 * @file RightSidePanel.h
 * @brief Right-side panel hosting Scratch Pads and Wingman Console
 *
 * Vertically split panel containing:
 * - Top: Scratch Pads area (placeholder for future arrangements)
 * - Bottom: Wingman Console panel
 * - Resizable splitter between them
 */

#pragma once

#include "SkiaCanvasComponent.h"
#include "SkiaTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declarations (global namespace)
class WingmanPanel;

namespace zenith {

/**
 * @class RightSidePanel
 * @brief Right panel with Scratch Pads and Wingman Console
 *
 * Provides:
 * - Scratch Pads area for quick ideas/arrangements
 * - Wingman AI command console
 * - Vertical resizing between sections
 */
class RightSidePanel : public juce::Component, public SkiaComponent {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  RightSidePanel();
  ~RightSidePanel() override = default;

  //==========================================================================
  // SkiaComponent Implementation
  //==========================================================================

  bool supportsSkiaRendering() const override { return true; }
  void paintToSkia(SkCanvas *canvas, SkRect bounds) override;

  //==========================================================================
  // Panel Access
  //==========================================================================

  void setWingmanPanel(WingmanPanel *panel);
  WingmanPanel *getWingmanPanel() const { return wingmanPanel_; }

  void setWingmanPanelHeight(int height);
  int getWingmanPanelHeight() const { return wingmanHeight_; }

  //==========================================================================
  // Component Overrides
  //==========================================================================

  void resized() override;
  void paint(juce::Graphics &g) override;

  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;
  void mouseMove(const juce::MouseEvent &event) override;

private:
  //==========================================================================
  // Scratch Pads Component (Placeholder)
  //==========================================================================

  class ScratchPadsPanel : public SkiaCanvasComponent {
  public:
    ScratchPadsPanel();
    ~ScratchPadsPanel() override = default;

  protected:
    void paintSkia(SkCanvas &canvas,
                   const juce::Rectangle<int> &bounds) override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPadsPanel)
  };

  //==========================================================================
  // Internal Methods
  //==========================================================================

  juce::Rectangle<int> getSplitterBounds() const;
  bool isSplitterHovered(const juce::Point<int> &point) const;

  //==========================================================================
  // State
  //==========================================================================

  std::unique_ptr<ScratchPadsPanel> scratchPadsPanel_;
  WingmanPanel *wingmanPanel_ = nullptr; // Not owned by this component

  int wingmanHeight_ = 300;
  static constexpr int MIN_WINGMAN_HEIGHT = 200;
  static constexpr int SPLITTER_HEIGHT = 6;

  bool isDraggingSplitter_ = false;
  bool isSplitterHovered_ = false;
  int dragStartY_ = 0;
  int dragStartHeight_ = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightSidePanel)
};

} // namespace zenith
