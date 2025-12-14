/*
  ==============================================================================

    ResizablePanelContainer.h
    Created: 2025-12-12
    Author:  Zenith DAW Team

    A flexible, VS Code-style resizable panel container system.
    Features:
    - Drag-to-resize dividers between panels
    - Collapsible panels with smooth animation
    - Tab groups (multiple views in same panel area)
    - Minimum panel sizes enforced
    - Smooth resize animations

  ==============================================================================
*/

#pragma once

#include "../ui/skia/SkiaComponent.h"
#include "../ui/skia/ZenithDesignSystem.h"
#include "LayoutManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace zenith {

// Forward declarations
class PanelDivider;
class TabGroup;

//==============================================================================
// Panel Header Component
//==============================================================================

/**
 * @brief Header bar for a panel with title and collapse button
 */
class PanelHeader : public SkiaComponent {
public:
  PanelHeader(const juce::String &title, bool collapsible = true);
  ~PanelHeader() override = default;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  void setTitle(const juce::String &title);
  void setCollapsed(bool collapsed);
  bool isCollapsed() const { return isCollapsed_; }

  // Callbacks
  std::function<void()> onCollapseClicked;
  std::function<void()> onHeaderDragStart;

  static constexpr int headerHeight = 28;

  // Accessibility
  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

private:
  juce::String title_;
  bool isCollapsible_ = true;
  bool isCollapsed_ = false;
  bool hoverOverCollapse_ = false;

  juce::Rectangle<float> getCollapseButtonBounds() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelHeader)
};

//==============================================================================
// Panel Wrapper
//==============================================================================

/**
 * @brief Wrapper around a content component with header and resize handling
 */
class PanelWrapper : public SkiaComponent {
public:
  PanelWrapper(const juce::String &panelId, juce::Component *content,
               const layout::PanelConfig &config);
  ~PanelWrapper() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // Panel management
  const juce::String &getPanelId() const { return panelId_; }
  juce::Component *getContent() const { return content_; }
  const layout::PanelConfig &getConfig() const { return config_; }

  // Size management
  float getMinSize() const { return config_.minSize; }
  float getMaxSize() const {
    return config_.maxSize > 0 ? config_.maxSize : 10000.0f;
  }
  float getFlex() const { return config_.flex; }
  float getCurrentSize() const { return currentSize_; }
  void setCurrentSize(float size);

  // Collapse support
  bool isCollapsible() const { return config_.isCollapsible; }
  bool isCollapsed() const { return isCollapsed_; }
  void setCollapsed(bool collapsed, bool animate = true);
  void toggleCollapse(bool animate = true);

  // Animation
  void timerCallback() override;

  // Callbacks
  std::function<void(PanelWrapper *)> onCollapseStateChanged;
  std::function<void(PanelWrapper *)> onSizeChanged;

private:
  juce::String panelId_;
  juce::Component *content_ = nullptr;
  layout::PanelConfig config_;

  std::unique_ptr<PanelHeader> header_;
  bool isCollapsed_ = false;
  float currentSize_ = 0.0f;
  float targetSize_ = 0.0f;
  float preCollapseSize_ = 0.0f;
  float animationProgress_ = 1.0f;
  juce::uint32 animationStartTime_ = 0; // For time-based animation

  static constexpr int collapsedHeight = PanelHeader::headerHeight;
  static constexpr float animationDurationMs =
      200.0f; // Animation duration in ms

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelWrapper)
};

//==============================================================================
// Panel Divider
//==============================================================================

/**
 * @brief Draggable divider between panels
 */
class PanelDivider : public SkiaComponent {
public:
  PanelDivider(bool isHorizontal);
  ~PanelDivider() override = default;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  bool isHorizontal() const { return isHorizontal_; }

  // Position constraints
  void setPositionRatio(float ratio);
  float getPositionRatio() const { return positionRatio_; }
  void setPositionConstraints(float minRatio, float maxRatio);

  // Callbacks
  std::function<void(float deltaPixels)> onDrag;
  std::function<void()> onDragStart;
  std::function<void()> onDragEnd;

  static constexpr int dividerSize = 6;

  // Accessibility
  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

private:
  bool isHorizontal_;
  bool isDragging_ = false;
  bool isHovered_ = false;
  float positionRatio_ = 0.5f;
  float minPositionRatio_ = 0.1f;
  float maxPositionRatio_ = 0.9f;
  int dragStartPos_ = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelDivider)
};

//==============================================================================
// Tab Group
//==============================================================================

/**
 * @brief Tab bar for grouping multiple panels in the same area
 */
class TabGroup : public SkiaComponent {
public:
  /**
   * @brief Constructor with optional accessibility title
   * @param accessibilityTitle Title for screen readers to distinguish multiple
   * TabGroups
   */
  explicit TabGroup(const juce::String &accessibilityTitle = "Tab List");
  ~TabGroup() override;

  /** @brief Set the accessibility title for this TabGroup */
  void setAccessibilityTitle(const juce::String &title);

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

  // Tab management
  void addTab(const juce::String &tabId, const juce::String &title,
              juce::Component *content);
  void removeTab(const juce::String &tabId);
  void setActiveTab(const juce::String &tabId, bool animate = true);
  juce::String getActiveTabId() const;
  int getTabCount() const;

  // Drag reordering
  void setDragReorderEnabled(bool enabled) { dragReorderEnabled_ = enabled; }

  // Callbacks
  std::function<void(const juce::String &tabId)> onTabChanged;
  std::function<void(const juce::String &tabId, int newIndex)> onTabReordered;
  std::function<void(const juce::String &tabId)> onTabClosed;
  std::function<void(const juce::String &tabId, const juce::Point<int> &pos)>
      onTabDraggedOut;

  static constexpr int tabBarHeight = 32;

  // Accessibility
  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

private:
  struct TabInfo {
    juce::String id;
    juce::String title;
    juce::Component *content = nullptr;
    juce::Rectangle<float> bounds;
    bool isHovered = false;
    bool isBeingDragged = false;
  };

  std::vector<TabInfo> tabs_;
  int activeTabIndex_ = -1;
  bool dragReorderEnabled_ = true;
  juce::String
      accessibilityTitle_; // Unique title for screen reader identification
  int draggedTabIndex_ = -1;
  juce::Point<int> dragStartPos_;

  void updateTabBounds();
  int getTabIndexAtPosition(const juce::Point<int> &pos) const;
  void animateTabSwitch();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabGroup)
};

//==============================================================================
// Resizable Panel Container
//==============================================================================

/**
 * @brief Main container that manages a hierarchy of resizable panels
 *
 * Supports:
 * - Horizontal/Vertical splits
 * - Nested layouts
 * - Drag-to-resize
 * - Collapsible panels
 * - Tab groups
 * - Layout persistence
 */
class ResizablePanelContainer : public SkiaComponent,
                                public juce::ChangeListener {
public:
  enum class SplitDirection { Horizontal, Vertical };

  ResizablePanelContainer();
  ~ResizablePanelContainer() override;

  //============================================================================
  // Panel Management
  //============================================================================

  /**
   * @brief Add a panel to the container
   */
  void addPanel(std::unique_ptr<juce::Component> content,
                const layout::PanelConfig &config,
                SplitDirection direction = SplitDirection::Horizontal);

  /**
   * @brief Add a panel to a specific position
   */
  void insertPanel(std::unique_ptr<juce::Component> content,
                   const layout::PanelConfig &config, int insertIndex,
                   SplitDirection direction = SplitDirection::Horizontal);

  /**
   * @brief Remove a panel by ID
   */
  void removePanel(const juce::String &panelId);

  /**
   * @brief Get a panel by ID
   */
  PanelWrapper *getPanel(const juce::String &panelId) const;

  /**
   * @brief Get all panel IDs
   */
  juce::StringArray getPanelIds() const;

  //============================================================================
  // Layout Control
  //============================================================================

  void setSplitDirection(SplitDirection direction);
  SplitDirection getSplitDirection() const { return splitDirection_; }

  /**
   * @brief Set divider position by ratio (0.0 - 1.0)
   */
  void setDividerPosition(int dividerIndex, float ratio, bool animate = true);

  /**
   * @brief Apply a layout configuration
   */
  void applyLayoutConfig(const layout::LayoutConfig &config);

  /**
   * @brief Capture current layout as configuration
   */
  layout::LayoutConfig
  captureLayoutConfig(const juce::String &name = "Custom") const;

  //============================================================================
  // Tab Groups
  //============================================================================

  /**
   * @brief Create a tab group at a specific panel position
   */
  TabGroup *createTabGroup(int panelIndex);

  /**
   * @brief Add an existing panel to a tab group
   */
  void addPanelToTabGroup(const juce::String &panelId, TabGroup *group);

  //============================================================================
  // Component Interface
  //============================================================================

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  //============================================================================
  // ChangeListener
  //============================================================================

  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

private:
  struct PanelSlot {
    std::unique_ptr<PanelWrapper> wrapper;
    std::unique_ptr<juce::Component> ownedContent;
    float sizeRatio = 1.0f;
  };

  std::vector<PanelSlot> panels_;
  std::vector<std::unique_ptr<PanelDivider>> dividers_;
  std::vector<std::unique_ptr<TabGroup>> tabGroups_;

  SplitDirection splitDirection_ = SplitDirection::Horizontal;
  bool isAnimating_ = false;

  // Layout calculation
  void recalculateLayout();
  void updateDividerPositions();
  void handleDividerDrag(int dividerIndex, float deltaPixels);
  void constrainPanelSizes();

  // Animation
  void animatePanelSizes();

  // Nested layout support
  void buildLayoutFromConfig(const juce::var &layoutNode, int depth);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizablePanelContainer)
};

} // namespace zenith
