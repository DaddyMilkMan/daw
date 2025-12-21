/**
 * @file BrowserPanel.h
 * @brief Left-side browser panel with tabbed navigation
 *
 * Collapsible panel (240-260px width) containing:
 * - Tabs: Devices, Clips, Files, Macros
 * - Search bar at the top
 * - Content area for each tab
 */

#pragma once

#include "SkiaCanvasComponent.h"
#include "SkiaTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace zenith {

/**
 * @class BrowserPanel
 * @brief Modern DAW-style browser panel with tabbed navigation
 *
 * Provides access to various resources:
 * - Devices: VST3 plugins, built-in instruments
 * - Clips: Audio clips, MIDI patterns
 * - Files: Project files, samples
 * - Macros: Custom macros and shortcuts
 */
class BrowserPanel : public SkiaCanvasComponent {
public:
  //==========================================================================
  // Tab Types
  //==========================================================================

  enum class Tab { Devices, Clips, Files, Macros };

  //==========================================================================
  // Item Structure
  //==========================================================================

  struct BrowserItem {
    juce::String name;
    juce::String category;
    juce::String path;
    bool isFavorite = false;

    BrowserItem() = default;
    BrowserItem(const juce::String &n, const juce::String &cat,
                const juce::String &p = "")
        : name(n), category(cat), path(p) {}
  };

  //==========================================================================
  // Construction
  //==========================================================================

  BrowserPanel();
  ~BrowserPanel() override = default;

  //==========================================================================
  // State Management
  //==========================================================================

  void setCurrentTab(Tab tab);
  Tab getCurrentTab() const { return currentTab_; }

  void setCollapsed(bool collapsed);
  bool isCollapsed() const { return isCollapsed_; }

  void setSearchText(const juce::String &text);
  juce::String getSearchText() const { return searchText_; }

  //==========================================================================
  // Content Management
  //==========================================================================

  void addItem(Tab tab, const BrowserItem &item);
  void clearItems(Tab tab);
  std::vector<BrowserItem> getFilteredItems() const;

  //==========================================================================
  // Callbacks
  //==========================================================================

  std::function<void(Tab)> onTabChanged;
  std::function<void(const BrowserItem &)> onItemSelected;
  std::function<void(const BrowserItem &)> onItemDoubleClicked;
  std::function<void(const juce::String &)> onSearchChanged;
  std::function<void()> onCollapseToggled;

  //==========================================================================
  // Component Overrides
  //==========================================================================

  void resized() override;
  void mouseDown(const juce::MouseEvent &event) override;
  void mouseUp(const juce::MouseEvent &event) override;
  void mouseMove(const juce::MouseEvent &event) override;
  void mouseDoubleClick(const juce::MouseEvent &event) override;
  void mouseWheelMove(const juce::MouseEvent &event,
                      const juce::MouseWheelDetails &wheel) override;

protected:
  //==========================================================================
  // Skia Rendering
  //==========================================================================

#if ZENITH_ENABLE_SKIA
  void paintSkia(SkCanvas &canvas,
                 const juce::Rectangle<int> &bounds) override;
#endif

private:
  //==========================================================================
  // Internal Rendering Methods
  //==========================================================================

#if ZENITH_ENABLE_SKIA
  void drawBackground(SkCanvas &canvas, const SkRect &bounds);
  void drawTabBar(SkCanvas &canvas, const SkRect &bounds);
  void drawSearchBar(SkCanvas &canvas, const SkRect &bounds);
  void drawContentArea(SkCanvas &canvas, const SkRect &bounds);
  void drawCollapseButton(SkCanvas &canvas, const SkRect &bounds);

  void drawTab(SkCanvas &canvas, const SkRect &rect, const juce::String &label,
               bool isActive, bool isHovered);
  void drawBrowserItem(SkCanvas &canvas, const SkRect &rect,
                       const BrowserItem &item, bool isSelected,
                       bool isHovered);
#endif

  //==========================================================================
  // Hit Testing
  //==========================================================================

  enum class HitZone {
    None,
    TabDevices,
    TabClips,
    TabFiles,
    TabMacros,
    SearchBar,
    CollapseButton,
    ContentItem
  };

  HitZone hitTest(const juce::Point<int> &point) const;
  int getItemAtPoint(const juce::Point<int> &point) const;

  juce::Rectangle<int> getTabBarBounds() const;
  juce::Rectangle<int> getSearchBarBounds() const;
  juce::Rectangle<int> getContentAreaBounds() const;
  juce::Rectangle<int> getCollapseButtonBounds() const;
  juce::Rectangle<int> getTabBounds(Tab tab) const;

  //==========================================================================
  // State
  //==========================================================================

  Tab currentTab_ = Tab::Devices;
  bool isCollapsed_ = false;
  juce::String searchText_;

  // Content for each tab
  std::vector<BrowserItem> devicesItems_;
  std::vector<BrowserItem> clipsItems_;
  std::vector<BrowserItem> filesItems_;
  std::vector<BrowserItem> macrosItems_;

  // Interaction state
  HitZone hoveredZone_ = HitZone::None;
  HitZone activeZone_ = HitZone::None;
  int selectedItemIndex_ = -1;
  int hoveredItemIndex_ = -1;

  // Scroll state
  float scrollOffset_ = 0.0f;
  float maxScrollOffset_ = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

} // namespace zenith
