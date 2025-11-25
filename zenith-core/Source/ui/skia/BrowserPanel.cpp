/**
 * @file BrowserPanel.cpp
 * @brief Implementation of left-side browser panel
 */

// POLISH: spacing normalized to 8px grid (width 256px, heights 48/40/32,
// padding 16, radius 4px)

#include "BrowserPanel.h"
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

static constexpr float BROWSER_WIDTH = 256.0f; // 8px grid: 260→256
static constexpr float COLLAPSED_WIDTH = 48.0f;
static constexpr float TAB_BAR_HEIGHT = 48.0f;    // 8px grid: 44→48
static constexpr float SEARCH_BAR_HEIGHT = 40.0f; // 8px grid: 36→40
static constexpr float ITEM_HEIGHT = 32.0f;
static constexpr float SECTION_PADDING = 16.0f; // 8px grid: 12→16
static constexpr float CORNER_RADIUS = 4.0f;    // 8px grid: 6→4

//==============================================================================
// Construction
//==============================================================================

BrowserPanel::BrowserPanel() {
  setSize(static_cast<int>(BROWSER_WIDTH), 600);

  // Populate with some default items (will be replaced by real data)
  addItem(Tab::Devices, BrowserItem("Zenith PolySynth", "Instruments"));
  addItem(Tab::Devices, BrowserItem("Zenith Sampler", "Instruments"));
  addItem(Tab::Clips, BrowserItem("Drum Loop 01", "Drums"));
  addItem(Tab::Clips, BrowserItem("Bass Pattern", "Bass"));
  addItem(Tab::Files, BrowserItem("Project 1.zen", "Projects"));
  addItem(Tab::Macros, BrowserItem("Quick Export", "Utilities"));
}

//==============================================================================
// State Management
//==============================================================================

void BrowserPanel::setCurrentTab(Tab tab) {
  if (currentTab_ != tab) {
    currentTab_ = tab;
    scrollOffset_ = 0.0f;
    selectedItemIndex_ = -1;
    repaint();

    if (onTabChanged)
      onTabChanged(tab);
  }
}

void BrowserPanel::setCollapsed(bool collapsed) {
  if (isCollapsed_ != collapsed) {
    isCollapsed_ = collapsed;
    setSize(static_cast<int>(collapsed ? COLLAPSED_WIDTH : BROWSER_WIDTH),
            getHeight());
    repaint();

    if (onCollapseToggled)
      onCollapseToggled();
  }
}

void BrowserPanel::setSearchText(const juce::String &text) {
  if (searchText_ != text) {
    searchText_ = text;
    repaint();

    if (onSearchChanged)
      onSearchChanged(text);
  }
}

//==============================================================================
// Content Management
//==============================================================================

void BrowserPanel::addItem(Tab tab, const BrowserItem &item) {
  switch (tab) {
  case Tab::Devices:
    devicesItems_.push_back(item);
    break;
  case Tab::Clips:
    clipsItems_.push_back(item);
    break;
  case Tab::Files:
    filesItems_.push_back(item);
    break;
  case Tab::Macros:
    macrosItems_.push_back(item);
    break;
  }
  repaint();
}

void BrowserPanel::clearItems(Tab tab) {
  switch (tab) {
  case Tab::Devices:
    devicesItems_.clear();
    break;
  case Tab::Clips:
    clipsItems_.clear();
    break;
  case Tab::Files:
    filesItems_.clear();
    break;
  case Tab::Macros:
    macrosItems_.clear();
    break;
  }
  repaint();
}

std::vector<BrowserPanel::BrowserItem> BrowserPanel::getFilteredItems() const {
  // Get items for current tab
  const std::vector<BrowserItem> *items = nullptr;
  switch (currentTab_) {
  case Tab::Devices:
    items = &devicesItems_;
    break;
  case Tab::Clips:
    items = &clipsItems_;
    break;
  case Tab::Files:
    items = &filesItems_;
    break;
  case Tab::Macros:
    items = &macrosItems_;
    break;
  }

  if (!items)
    return {};

  // Filter by search text
  if (searchText_.isEmpty())
    return *items;

  std::vector<BrowserItem> filtered;
  for (const auto &item : *items) {
    if (item.name.containsIgnoreCase(searchText_) ||
        item.category.containsIgnoreCase(searchText_)) {
      filtered.push_back(item);
    }
  }

  return filtered;
}

//==============================================================================
// Component Overrides
//==============================================================================

void BrowserPanel::resized() { SkiaCanvasComponent::resized(); }

void BrowserPanel::mouseDown(const juce::MouseEvent &event) {
  activeZone_ = hitTest(event.getPosition());

  if (activeZone_ == HitZone::ContentItem) {
    int itemIndex = getItemAtPoint(event.getPosition());
    if (itemIndex >= 0) {
      selectedItemIndex_ = itemIndex;
      repaint();
    }
  }
}

void BrowserPanel::mouseUp(const juce::MouseEvent &event) {
  auto zone = hitTest(event.getPosition());

  if (zone == activeZone_) {
    switch (zone) {
    case HitZone::TabDevices:
      setCurrentTab(Tab::Devices);
      break;
    case HitZone::TabClips:
      setCurrentTab(Tab::Clips);
      break;
    case HitZone::TabFiles:
      setCurrentTab(Tab::Files);
      break;
    case HitZone::TabMacros:
      setCurrentTab(Tab::Macros);
      break;
    case HitZone::CollapseButton:
      setCollapsed(!isCollapsed_);
      break;
    default:
      break;
    }
  }

  activeZone_ = HitZone::None;
}

void BrowserPanel::mouseMove(const juce::MouseEvent &event) {
  auto newZone = hitTest(event.getPosition());
  int newItemIndex = getItemAtPoint(event.getPosition());

  if (newZone != hoveredZone_ || newItemIndex != hoveredItemIndex_) {
    hoveredZone_ = newZone;
    hoveredItemIndex_ = newItemIndex;
    repaint();
  }
}

void BrowserPanel::mouseDoubleClick(const juce::MouseEvent &event) {
  if (hitTest(event.getPosition()) == HitZone::ContentItem) {
    int itemIndex = getItemAtPoint(event.getPosition());
    if (itemIndex >= 0) {
      auto items = getFilteredItems();
      if (itemIndex < static_cast<int>(items.size())) {
        if (onItemDoubleClicked)
          onItemDoubleClicked(items[itemIndex]);
      }
    }
  }
}

void BrowserPanel::mouseWheelMove(const juce::MouseEvent &event,
                                  const juce::MouseWheelDetails &wheel) {
  if (isCollapsed_)
    return;

  scrollOffset_ -= wheel.deltaY * 40.0f; // Scroll speed

  // Clamp scroll offset
  scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_);

  repaint();
}

//==============================================================================
// Skia Rendering
//==============================================================================

void BrowserPanel::paintSkia(SkCanvas &canvas,
                             const juce::Rectangle<int> &bounds) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();

  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  drawBackground(canvas, skBounds);

  if (isCollapsed_) {
    drawCollapseButton(canvas, skBounds);
    return;
  }

  // Draw sections
  auto tabBarBounds = getTabBarBounds();
  auto searchBarBounds = getSearchBarBounds();
  auto contentAreaBounds = getContentAreaBounds();

  SkRect tabBarRect =
      SkRect::MakeXYWH(tabBarBounds.getX(), tabBarBounds.getY(),
                       tabBarBounds.getWidth(), tabBarBounds.getHeight());
  SkRect searchBarRect =
      SkRect::MakeXYWH(searchBarBounds.getX(), searchBarBounds.getY(),
                       searchBarBounds.getWidth(), searchBarBounds.getHeight());
  SkRect contentRect = SkRect::MakeXYWH(
      contentAreaBounds.getX(), contentAreaBounds.getY(),
      contentAreaBounds.getWidth(), contentAreaBounds.getHeight());

  drawTabBar(canvas, tabBarRect);
  drawSearchBar(canvas, searchBarRect);
  drawContentArea(canvas, contentRect);
  drawCollapseButton(canvas, skBounds);
}

void BrowserPanel::drawBackground(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Main background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg1);

  canvas.drawRect(bounds, bgPaint);

  // Right border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors.borderSubtle);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);

  canvas.drawLine(bounds.right() - 1, 0, bounds.right() - 1, bounds.bottom(),
                  borderPaint);
}

void BrowserPanel::drawTabBar(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg2);

  canvas.drawRect(bounds, bgPaint);

  // Bottom border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors.borderSubtle);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);

  canvas.drawLine(bounds.left(), bounds.bottom() - 1, bounds.right(),
                  bounds.bottom() - 1, borderPaint);

  // Draw tabs
  float tabWidth = bounds.width() / 4.0f;

  const char *tabLabels[] = {"Devices", "Clips", "Files", "Macros"};
  Tab tabs[] = {Tab::Devices, Tab::Clips, Tab::Files, Tab::Macros};
  HitZone hitZones[] = {HitZone::TabDevices, HitZone::TabClips,
                        HitZone::TabFiles, HitZone::TabMacros};

  for (int i = 0; i < 4; ++i) {
    SkRect tabRect = SkRect::MakeXYWH(bounds.left() + i * tabWidth,
                                      bounds.top(), tabWidth, bounds.height());

    bool isActive = (currentTab_ == tabs[i]);
    bool isHovered = (hoveredZone_ == hitZones[i]);

    drawTab(canvas, tabRect, tabLabels[i], isActive, isHovered);
  }
}

void BrowserPanel::drawSearchBar(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg3);

  SkRect searchRect = bounds;
  searchRect.inset(SECTION_PADDING,
                   (bounds.height() - 32) / 2); // 8px grid: 28→32

  SkRRect searchRRect =
      SkRRect::MakeRectXY(searchRect, CORNER_RADIUS, CORNER_RADIUS);
  canvas.drawRRect(searchRRect, bgPaint);

  // Search icon (magnifying glass)
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);
  iconPaint.setColor(colors.textSubtle);
  iconPaint.setStyle(SkPaint::kStroke_Style);
  iconPaint.setStrokeWidth(1.5f);

  float iconX = searchRect.left() + 12; // 8px grid: 10→12
  float iconY = searchRect.centerY();

  canvas.drawCircle(iconX, iconY, 6.0f, iconPaint);
  canvas.drawLine(iconX + 4, iconY + 4, iconX + 8, iconY + 8, iconPaint);

  // Search text
  auto &typo = SkiaTheme::getInstance().getTypography();
  if (!searchText_.isEmpty()) {
    SkFont font;
    font.setSize(typo.body.size);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors.textStrong);

    canvas.drawString(searchText_.toRawUTF8(),
                      searchRect.left() + 24, // 8px grid: 26→24
                      searchRect.centerY() + 4, font, textPaint);
  } else {
    // Placeholder
    SkFont font;
    font.setSize(typo.body.size);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors.textSubtle);

    canvas.drawString("Search...", searchRect.left() + 24, // 8px grid: 26→24
                      searchRect.centerY() + 4, font, textPaint);
  }
}

void BrowserPanel::drawContentArea(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Clip to content area
  canvas.save();
  canvas.clipRect(bounds);

  // Get filtered items
  auto items = getFilteredItems();

  // Calculate max scroll
  float totalHeight = items.size() * ITEM_HEIGHT;
  maxScrollOffset_ = std::max(0.0f, totalHeight - bounds.height());

  // Draw items
  float yPos = bounds.top() - scrollOffset_;

  for (size_t i = 0; i < items.size(); ++i) {
    SkRect itemRect =
        SkRect::MakeXYWH(bounds.left(), yPos, bounds.width(), ITEM_HEIGHT);

    // Only draw if visible
    if (itemRect.bottom() >= bounds.top() &&
        itemRect.top() <= bounds.bottom()) {
      bool isSelected = (static_cast<int>(i) == selectedItemIndex_);
      bool isHovered = (static_cast<int>(i) == hoveredItemIndex_);

      drawBrowserItem(canvas, itemRect, items[i], isSelected, isHovered);
    }

    yPos += ITEM_HEIGHT;
  }

  canvas.restore();

  // Draw scrollbar if needed
  if (maxScrollOffset_ > 0) {
    float scrollBarHeight = (bounds.height() / totalHeight) * bounds.height();
    float scrollBarY = bounds.top() + (scrollOffset_ / maxScrollOffset_) *
                                          (bounds.height() - scrollBarHeight);

    SkPaint scrollBarPaint;
    scrollBarPaint.setAntiAlias(true);
    scrollBarPaint.setColor(colors.borderStrong);

    SkRect scrollBarRect =
        SkRect::MakeXYWH(bounds.right() - 8, scrollBarY, 4, // 8px grid: -6→-8
                         scrollBarHeight);
    SkRRect scrollBarRRect = SkRRect::MakeRectXY(scrollBarRect, 2.0f, 2.0f);

    canvas.drawRRect(scrollBarRRect, scrollBarPaint);
  }
}

void BrowserPanel::drawCollapseButton(SkCanvas &canvas, const SkRect &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  auto buttonBounds = getCollapseButtonBounds();
  SkRect buttonRect =
      SkRect::MakeXYWH(buttonBounds.getX(), buttonBounds.getY(),
                       buttonBounds.getWidth(), buttonBounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(hoveredZone_ == HitZone::CollapseButton ? colors.bg3
                                                           : colors.bg2);

  SkRRect bgRRect = SkRRect::MakeRectXY(buttonRect, 4.0f, 4.0f);
  canvas.drawRRect(bgRRect, bgPaint);

  // Arrow icon
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);
  iconPaint.setColor(colors.textMuted);
  iconPaint.setStyle(SkPaint::kStroke_Style);
  iconPaint.setStrokeWidth(2.0f);

  float centerX = buttonRect.centerX();
  float centerY = buttonRect.centerY();

  SkPath arrowPath;
  if (isCollapsed_) {
    // Arrow pointing right (expand)
    arrowPath.moveTo(centerX - 4, centerY - 6);
    arrowPath.lineTo(centerX + 4, centerY);
    arrowPath.lineTo(centerX - 4, centerY + 6);
  } else {
    // Arrow pointing left (collapse)
    arrowPath.moveTo(centerX + 4, centerY - 6);
    arrowPath.lineTo(centerX - 4, centerY);
    arrowPath.lineTo(centerX + 4, centerY + 6);
  }

  canvas.drawPath(arrowPath, iconPaint);
}

void BrowserPanel::drawTab(SkCanvas &canvas, const SkRect &rect,
                           const juce::String &label, bool isActive,
                           bool isHovered) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &interaction = theme.getInteraction();

  // Background
  if (isActive) {
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(colors.bg1);
    canvas.drawRect(rect, bgPaint);
  } else if (isHovered) {
    // POLISH: unified hover using theme.getInteraction()
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(interaction.hoverOverlay);
    canvas.drawRect(rect, hoverPaint);
  }

  // Active indicator (bottom line)
  if (isActive) {
    SkPaint indicatorPaint;
    indicatorPaint.setAntiAlias(true);
    indicatorPaint.setColor(colors.accentMain);
    indicatorPaint.setStyle(SkPaint::kStroke_Style);
    indicatorPaint.setStrokeWidth(2.0f);

    canvas.drawLine(rect.left(), rect.bottom() - 1, rect.right(),
                    rect.bottom() - 1, indicatorPaint);
  }

  // Text
  auto &typo = SkiaTheme::getInstance().getTypography();
  SkFont font;
  font.setSize(typo.body.size);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(isActive ? colors.textStrong : colors.textMuted);

  // Center text
  SkRect textBounds;
  font.measureText(label.toRawUTF8(), label.length(), SkTextEncoding::kUTF8,
                   &textBounds);

  float textX = rect.centerX() - textBounds.width() / 2;
  float textY = rect.centerY() + 4; // 8px grid: 5→4

  canvas.drawString(label.toRawUTF8(), textX, textY, font, textPaint);
}

void BrowserPanel::drawBrowserItem(SkCanvas &canvas, const SkRect &rect,
                                   const BrowserItem &item, bool isSelected,
                                   bool isHovered) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();
  auto &interaction = theme.getInteraction();

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);

  if (isSelected) {
    bgPaint.setColor(colors.accentMain);
  } else {
    bgPaint.setColor(colors.bg1);
  }

  canvas.drawRect(rect, bgPaint);

  // POLISH: unified hover using theme.getInteraction()
  if (isHovered && !isSelected) {
    SkPaint hoverPaint;
    hoverPaint.setAntiAlias(true);
    hoverPaint.setColor(interaction.hoverOverlay);
    canvas.drawRect(rect, hoverPaint);
  }

  // Icon (color-coded by category)
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);

  if (item.category == "Instruments") {
    iconPaint.setColor(colors.clipLeads);
  } else if (item.category == "Drums") {
    iconPaint.setColor(colors.clipDrums);
  } else if (item.category == "Bass") {
    iconPaint.setColor(colors.clipBass);
  } else if (item.category == "Projects") {
    iconPaint.setColor(colors.accentAlt);
  } else {
    iconPaint.setColor(colors.textMuted);
  }

  float iconX = rect.left() + 16;
  float iconY = rect.centerY();

  // Draw waveform preview for audio items instead of simple circle
  if (item.category == "Drums" || item.category == "Bass" ||
      item.category == "Instruments") {
    // Define waveform rect
    SkRect waveRect =
        SkRect::MakeXYWH(rect.left() + 8, rect.centerY() - 8, 60, 16);

    // Draw "Fake" Waveform for visual polish (until real audio analysis is
    // linked)
    SkPath wavePath;
    wavePath.moveTo(waveRect.left(), waveRect.centerY());
    float x = waveRect.left();
    int seed =
        (int)(item.name.hashCode() % 100); // Deterministic random based on name
    while (x < waveRect.right()) {
      seed = (seed * 1103515245 + 12345) & 0x7fffffff; // Simple LCG
      float h = (float)((seed % 10) + 2);              // Height variation
      wavePath.lineTo(x, waveRect.centerY() - h);
      wavePath.lineTo(x + 2, waveRect.centerY() + h);
      x += 3;
    }

    SkPaint wavePaint;
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setColor(isSelected ? colors.bg0 : iconPaint.getColor());
    wavePaint.setStrokeWidth(1.5f);
    wavePaint.setAntiAlias(true);
    canvas.drawPath(wavePath, wavePaint);
  } else {
    // Fallback to circle for non-audio items
    canvas.drawCircle(iconX, iconY, 4.0f, iconPaint);
  }

  // Item name
  auto &typo = theme.getTypography();
  SkFont font;
  font.setSize(typo.body.size);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(isSelected ? colors.bg0 : colors.textStrong);

  canvas.drawString(item.name.toRawUTF8(), rect.left() + 76, rect.centerY() + 4,
                    font, textPaint);

  // Category (smaller, muted)
  font.setSize(typo.small.size);
  textPaint.setColor(isSelected ? colors.bg1 : colors.textSubtle);

  canvas.drawString(item.category.toRawUTF8(), rect.left() + 76,
                    rect.centerY() + 16, font, textPaint);
}

//==============================================================================
// Hit Testing
//==============================================================================

BrowserPanel::HitZone
BrowserPanel::hitTest(const juce::Point<int> &point) const {
  if (getCollapseButtonBounds().contains(point))
    return HitZone::CollapseButton;

  if (isCollapsed_)
    return HitZone::None;

  if (getTabBounds(Tab::Devices).contains(point))
    return HitZone::TabDevices;
  if (getTabBounds(Tab::Clips).contains(point))
    return HitZone::TabClips;
  if (getTabBounds(Tab::Files).contains(point))
    return HitZone::TabFiles;
  if (getTabBounds(Tab::Macros).contains(point))
    return HitZone::TabMacros;

  if (getSearchBarBounds().contains(point))
    return HitZone::SearchBar;

  if (getContentAreaBounds().contains(point))
    return HitZone::ContentItem;

  return HitZone::None;
}

int BrowserPanel::getItemAtPoint(const juce::Point<int> &point) const {
  auto contentBounds = getContentAreaBounds();

  if (!contentBounds.contains(point))
    return -1;

  float relativeY = point.y - contentBounds.getY() + scrollOffset_;
  int itemIndex = static_cast<int>(relativeY / ITEM_HEIGHT);

  auto items = getFilteredItems();
  if (itemIndex < 0 || itemIndex >= static_cast<int>(items.size()))
    return -1;

  return itemIndex;
}

juce::Rectangle<int> BrowserPanel::getTabBarBounds() const {
  return juce::Rectangle<int>(0, 0, getWidth(),
                              static_cast<int>(TAB_BAR_HEIGHT));
}

juce::Rectangle<int> BrowserPanel::getSearchBarBounds() const {
  return juce::Rectangle<int>(0, static_cast<int>(TAB_BAR_HEIGHT), getWidth(),
                              static_cast<int>(SEARCH_BAR_HEIGHT));
}

juce::Rectangle<int> BrowserPanel::getContentAreaBounds() const {
  int y = static_cast<int>(TAB_BAR_HEIGHT + SEARCH_BAR_HEIGHT);
  int height = getHeight() - y - 40; // Leave space for collapse button

  return juce::Rectangle<int>(0, y, getWidth(), height);
}

juce::Rectangle<int> BrowserPanel::getCollapseButtonBounds() const {
  int size = 24;
  int x = isCollapsed_ ? (getWidth() - size) / 2 : getWidth() - size - 8;
  int y = getHeight() - size - 8;

  return juce::Rectangle<int>(x, y, size, size);
}

juce::Rectangle<int> BrowserPanel::getTabBounds(Tab tab) const {
  auto tabBarBounds = getTabBarBounds();
  float tabWidth = tabBarBounds.getWidth() / 4.0f;

  int tabIndex = static_cast<int>(tab);
  int x = static_cast<int>(tabIndex * tabWidth);

  return juce::Rectangle<int>(x, 0, static_cast<int>(tabWidth),
                              tabBarBounds.getHeight());
}

} // namespace zenith
