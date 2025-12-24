/*
  ==============================================================================

    ResizablePanelContainer.cpp
    Created: 2025-12-12
    Author:  Zenith DAW Team

  ==============================================================================
*/

#include "ResizablePanelContainer.h"
#include "../../engine/ZenithLogger.h"
#include "GlassmorphicPanel.h"
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
// PanelHeader Implementation
//==============================================================================

PanelHeader::PanelHeader(const juce::String &title, bool collapsible)
    : title_(title), isCollapsible_(collapsible) {
  setSize(100, headerHeight);
}

void PanelHeader::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background gradient
  SkPoint gradientPoints[2] = {{0, 0}, {0, bounds.getHeight()}};
  SkColor gradientColors[2] = {design::colors::BG_DARK,
                               design::colors::BG_DARKEST};
  auto shader = SkGradientShader::MakeLinear(gradientPoints, gradientColors,
                                             nullptr, 2, SkTileMode::kClamp);

  SkPaint bgPaint;
  bgPaint.setShader(shader);
  bgPaint.setAntiAlias(true);
  canvas->drawRect(skBounds, bgPaint);

  // Bottom border
  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  canvas->drawLine(0, bounds.getHeight() - 0.5f, bounds.getWidth(),
                   bounds.getHeight() - 0.5f, borderPaint);

  // Collapse button (if collapsible)
  if (isCollapsible_) {
    auto collapseBtn = getCollapseButtonBounds();

    if (hoverOverCollapse_) {
      SkPaint hoverPaint;
      hoverPaint.setColor(SkColorSetA(design::colors::NEON_GREEN, 40));
      hoverPaint.setAntiAlias(true);
      canvas->drawRoundRect(
          SkRect::MakeXYWH(collapseBtn.getX(), collapseBtn.getY(),
                           collapseBtn.getWidth(), collapseBtn.getHeight()),
          4.0f, 4.0f, hoverPaint);
    }

    // Draw chevron icon
    SkPaint iconPaint;
    iconPaint.setColor(isHovered() ? design::colors::TEXT_PRIMARY
                                   : design::colors::TEXT_SECONDARY);
    iconPaint.setAntiAlias(true);
    iconPaint.setStyle(SkPaint::kStroke_Style);
    iconPaint.setStrokeWidth(1.5f);
    iconPaint.setStrokeCap(SkPaint::kRound_Cap);

    float cx = collapseBtn.getCentreX();
    float cy = collapseBtn.getCentreY();
    float size = 4.0f;

    SkPath chevron;
    if (isCollapsed_) {
      // Right-pointing chevron
      chevron.moveTo(cx - size, cy - size);
      chevron.lineTo(cx + size, cy);
      chevron.lineTo(cx - size, cy + size);
    } else {
      // Down-pointing chevron
      chevron.moveTo(cx - size, cy - size);
      chevron.lineTo(cx, cy + size);
      chevron.lineTo(cx + size, cy - size);
    }
    canvas->drawPath(chevron, iconPaint);
  }

  // Title text
  SkFont font = design::getSkFont(11.0f, design::FontWeight::Bold);
  font.setSize(11.0f);

  SkPaint textPaint;
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  float textX = isCollapsible_ ? 28.0f : 8.0f;
  float textY = bounds.getHeight() / 2.0f + 4.0f;

  canvas->drawString(title_.toRawUTF8(), textX, textY, font, textPaint);
}

void PanelHeader::mouseDown(const juce::MouseEvent &e) {
  if (isCollapsible_) {
    auto collapseBtn = getCollapseButtonBounds();
    if (collapseBtn.contains(e.position.toFloat())) {
      if (onCollapseClicked)
        onCollapseClicked();
      return;
    }
  }

  // Header drag for panel reorganization
  if (onHeaderDragStart)
    onHeaderDragStart();
}

void PanelHeader::mouseEnter(const juce::MouseEvent &e) {
  SkiaComponent::mouseEnter(e);
  hoverOverCollapse_ = getCollapseButtonBounds().contains(e.position.toFloat());
  repaint();
}

void PanelHeader::mouseExit(const juce::MouseEvent &e) {
  SkiaComponent::mouseExit(e);
  hoverOverCollapse_ = false;
  repaint();
}

void PanelHeader::setTitle(const juce::String &title) {
  title_ = title;
  // Keep accessibility title in sync with display title
  juce::Component::setTitle(title);
  repaint();
}

void PanelHeader::setCollapsed(bool collapsed) {
  isCollapsed_ = collapsed;
  repaint();
}

juce::Rectangle<float> PanelHeader::getCollapseButtonBounds() const {
  return juce::Rectangle<float>(4.0f, 4.0f, 20.0f, 20.0f);
}

std::unique_ptr<juce::AccessibilityHandler>
PanelHeader::createAccessibilityHandler() {
  // Role: Group with the panel's title for screen reader identification
  setTitle(title_);
  return std::make_unique<juce::AccessibilityHandler>(
      *this, juce::AccessibilityRole::group);
}

//==============================================================================
// PanelWrapper Implementation
//==============================================================================

PanelWrapper::PanelWrapper(const juce::String &panelId,
                           juce::Component *content,
                           const layout::PanelConfig &config)
    : panelId_(panelId), content_(content), config_(config) {

  header_ = std::make_unique<PanelHeader>(config.name, config.isCollapsible);
  header_->onCollapseClicked = [this]() { toggleCollapse(true); };
  addAndMakeVisible(header_.get());

  if (content_) {
    addAndMakeVisible(content_);
  }

  currentSize_ = config.initialSize > 0 ? config.initialSize : 200.0f;
  targetSize_ = currentSize_;
  preCollapseSize_ = currentSize_;
  isCollapsed_ = config.isCollapsed;

  if (isCollapsed_) {
    header_->setCollapsed(true);
    currentSize_ = static_cast<float>(collapsedHeight);
  }
}

PanelWrapper::~PanelWrapper() { stopTimer(); }

void PanelWrapper::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Panel background with glassmorphic effect
  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Elevated;
  opts.cornerRadius = 8.0f;
  GlassmorphicPanel::drawWithOptions(canvas, skBounds, opts);

  // Draw header
  if (header_ && header_->isVisible()) {
    canvas->save();
    canvas->translate(static_cast<float>(header_->getX()),
                      static_cast<float>(header_->getY()));
    canvas->clipRect(SkRect::MakeWH(static_cast<float>(header_->getWidth()),
                                    static_cast<float>(header_->getHeight())));
    header_->drawSkia(canvas);
    canvas->restore();
  }

  // Draw content
  if (content_ && content_->isVisible()) {
    if (auto *sc = dynamic_cast<SkiaComponent *>(content_)) {
      canvas->save();
      // Translate to the content's position relative to this wrapper
      auto contentBounds = content_->getBounds();
      canvas->translate(static_cast<float>(contentBounds.getX()),
                        static_cast<float>(contentBounds.getY()));

      // Clip to content bounds to prevent bleeding
      canvas->clipRect(
          SkRect::MakeWH(static_cast<float>(contentBounds.getWidth()),
                         static_cast<float>(contentBounds.getHeight())));

      sc->drawSkia(canvas);
      canvas->restore();
    }
  }
}

void PanelWrapper::resized() {
  auto bounds = getLocalBounds();

  // Position header
  if (header_) {
    header_->setBounds(bounds.removeFromTop(PanelHeader::headerHeight));
  }

  // Position content
  if (content_ && !isCollapsed_) {
    content_->setBounds(bounds);
    content_->setVisible(true);
  } else if (content_) {
    content_->setVisible(false);
  }
}

void PanelWrapper::setCurrentSize(float size) {
  size = juce::jlimit(config_.minSize,
                      config_.maxSize > 0 ? config_.maxSize : 10000.0f, size);
  if (!juce::approximatelyEqual(currentSize_, size)) {
    currentSize_ = size;
    if (onSizeChanged)
      onSizeChanged(this);
  }
}

void PanelWrapper::setCollapsed(bool collapsed, bool animate) {
  if (isCollapsed_ == collapsed)
    return;

  isCollapsed_ = collapsed;
  header_->setCollapsed(collapsed);

  if (collapsed) {
    preCollapseSize_ = currentSize_;
    targetSize_ = static_cast<float>(collapsedHeight);
  } else {
    targetSize_ = preCollapseSize_;
  }

  if (animate) {
    animationStartTime_ = juce::Time::getMillisecondCounter();
    animationProgress_ = 0.0f;
    startTimer(16); // ~60fps animation
  } else {
    currentSize_ = targetSize_;
    animationProgress_ = 1.0f;
  }

  if (onCollapseStateChanged)
    onCollapseStateChanged(this);
}

void PanelWrapper::toggleCollapse(bool animate) {
  setCollapsed(!isCollapsed_, animate);
}

void PanelWrapper::timerCallback() {
  // Time-based animation for consistent duration regardless of frame rate
  juce::uint32 elapsed =
      juce::Time::getMillisecondCounter() - animationStartTime_;
  animationProgress_ =
      std::min(1.0f, static_cast<float>(elapsed) / animationDurationMs);

  if (animationProgress_ >= 1.0f) {
    animationProgress_ = 1.0f;
    currentSize_ = targetSize_;
    stopTimer();
  } else {
    // Smooth easing (ease out cubic)
    float eased = 1.0f - std::pow(1.0f - animationProgress_, 3.0f);
    float startSize =
        isCollapsed_ ? preCollapseSize_ : static_cast<float>(collapsedHeight);
    currentSize_ = startSize + (targetSize_ - startSize) * eased;
  }

  if (onSizeChanged)
    onSizeChanged(this);
  repaint();
}

//==============================================================================
// PanelDivider Implementation
//==============================================================================

PanelDivider::PanelDivider(bool isHorizontal) : isHorizontal_(isHorizontal) {
  setSize(dividerSize, dividerSize);
  setMouseCursor(isHorizontal ? juce::MouseCursor::UpDownResizeCursor
                              : juce::MouseCursor::LeftRightResizeCursor);
}

void PanelDivider::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(isDragging_  ? design::colors::NEON_GREEN
                   : isHovered_ ? design::colors::BG_LIGHT
                                : design::colors::BG_DARK);
  bgPaint.setAntiAlias(true);
  canvas->drawRect(skBounds, bgPaint);

  // Center line indicator
  SkPaint linePaint;
  linePaint.setColor(isDragging_  ? design::colors::NEON_GREEN
                     : isHovered_ ? design::colors::TEXT_SECONDARY
                                  : design::colors::BORDER_SUBTLE);
  linePaint.setStrokeWidth(2.0f);
  linePaint.setStrokeCap(SkPaint::kRound_Cap);
  linePaint.setAntiAlias(true);

  float cx = bounds.getWidth() / 2.0f;
  float cy = bounds.getHeight() / 2.0f;

  if (isHorizontal_) {
    // Horizontal divider - draw dots in a row
    float dotSpacing = 8.0f;
    int numDots = 3;
    float startX = cx - (numDots - 1) * dotSpacing / 2.0f;
    for (int i = 0; i < numDots; ++i) {
      canvas->drawCircle(startX + i * dotSpacing, cy, 1.5f, linePaint);
    }
  } else {
    // Vertical divider - draw dots in a column
    float dotSpacing = 8.0f;
    int numDots = 3;
    float startY = cy - (numDots - 1) * dotSpacing / 2.0f;
    for (int i = 0; i < numDots; ++i) {
      canvas->drawCircle(cx, startY + i * dotSpacing, 1.5f, linePaint);
    }
  }
}

void PanelDivider::mouseDown(const juce::MouseEvent &e) {
  isDragging_ = true;
  dragStartPos_ = isHorizontal_ ? e.getPosition().y : e.getPosition().x;

  if (onDragStart)
    onDragStart();
  repaint();
}

void PanelDivider::mouseDrag(const juce::MouseEvent &e) {
  if (!isDragging_)
    return;

  int currentPos = isHorizontal_ ? e.getPosition().y : e.getPosition().x;
  float delta = static_cast<float>(currentPos - dragStartPos_);

  if (onDrag)
    onDrag(delta);

  dragStartPos_ = currentPos;
}

void PanelDivider::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  isDragging_ = false;

  if (onDragEnd)
    onDragEnd();
  repaint();
}

void PanelDivider::mouseEnter(const juce::MouseEvent &e) {
  SkiaComponent::mouseEnter(e);
  isHovered_ = true;
  repaint();
}

void PanelDivider::mouseExit(const juce::MouseEvent &e) {
  SkiaComponent::mouseExit(e);
  isHovered_ = false;
  repaint();
}

void PanelDivider::setPositionRatio(float ratio) {
  positionRatio_ = juce::jlimit(minPositionRatio_, maxPositionRatio_, ratio);
}

void PanelDivider::setPositionConstraints(float minRatio, float maxRatio) {
  minPositionRatio_ = juce::jlimit(0.0f, 1.0f, minRatio);
  maxPositionRatio_ = juce::jlimit(minPositionRatio_, 1.0f, maxRatio);
  positionRatio_ =
      juce::jlimit(minPositionRatio_, maxPositionRatio_, positionRatio_);
}

std::unique_ptr<juce::AccessibilityHandler>
PanelDivider::createAccessibilityHandler() {
  // Role: Splitter (using unspecified since JUCE lacks splitter role)
  setHelpText("Drag to resize");
  setDescription(isHorizontal_ ? "Horizontal splitter" : "Vertical splitter");
  return std::make_unique<juce::AccessibilityHandler>(
      *this, juce::AccessibilityRole::unspecified);
}

//==============================================================================
// TabGroup Implementation
//==============================================================================

TabGroup::TabGroup(const juce::String &accessibilityTitle)
    : accessibilityTitle_(accessibilityTitle) {
  setSize(100, tabBarHeight + 100);
}

TabGroup::~TabGroup() = default;

void TabGroup::setAccessibilityTitle(const juce::String &title) {
  accessibilityTitle_ = title;
  // Update accessibility info if handler already exists
  juce::Component::setTitle(title);
}

void TabGroup::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Tab bar background
  SkRect tabBarRect =
      SkRect::MakeWH(bounds.getWidth(), static_cast<float>(tabBarHeight));

  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_DARK);
  bgPaint.setAntiAlias(true);
  canvas->drawRect(tabBarRect, bgPaint);

  // Draw tabs
  for (size_t i = 0; i < tabs_.size(); ++i) {
    const auto &tab = tabs_[i];
    bool isActive = (static_cast<int>(i) == activeTabIndex_);

    // Tab background
    SkPaint tabPaint;
    if (isActive) {
      tabPaint.setColor(design::colors::BG_LIGHT);
    } else if (tab.isHovered) {
      tabPaint.setColor(SkColorSetA(design::colors::BG_LIGHT, 128));
    } else {
      tabPaint.setColor(SK_ColorTRANSPARENT);
    }
    tabPaint.setAntiAlias(true);

    SkRect tabRect =
        SkRect::MakeXYWH(tab.bounds.getX(), tab.bounds.getY(),
                         tab.bounds.getWidth(), tab.bounds.getHeight());
    canvas->drawRoundRect(tabRect, 4.0f, 4.0f, tabPaint);

    // Active indicator
    if (isActive) {
      SkPaint indicatorPaint;
      indicatorPaint.setColor(design::colors::NEON_GREEN);
      indicatorPaint.setAntiAlias(true);
      canvas->drawRect(SkRect::MakeXYWH(tab.bounds.getX() + 4.0f,
                                        tab.bounds.getBottom() - 2.0f,
                                        tab.bounds.getWidth() - 8.0f, 2.0f),
                       indicatorPaint);
    }

    // Tab title
    SkFont font = design::getSkFont(11.0f, design::FontWeight::Regular);
    font.setSize(11.0f);

    SkPaint textPaint;
    textPaint.setColor(isActive ? design::colors::TEXT_PRIMARY
                                : design::colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);

    float textX = tab.bounds.getX() + 8.0f;
    float textY = tab.bounds.getCentreY() + 4.0f;
    canvas->drawString(tab.title.toRawUTF8(), textX, textY, font, textPaint);
  }

  // Bottom border
  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawLine(0, static_cast<float>(tabBarHeight) - 0.5f,
                   bounds.getWidth(), static_cast<float>(tabBarHeight) - 0.5f,
                   borderPaint);
}

void TabGroup::resized() {
  updateTabBounds();

  // Position content below tab bar
  auto contentArea = getLocalBounds().withTrimmedTop(tabBarHeight);

  for (size_t i = 0; i < tabs_.size(); ++i) {
    if (tabs_[i].content) {
      tabs_[i].content->setBounds(contentArea);
      tabs_[i].content->setVisible(static_cast<int>(i) == activeTabIndex_);
    }
  }
}

void TabGroup::mouseDown(const juce::MouseEvent &e) {
  int clickedTab = getTabIndexAtPosition(e.getPosition());
  if (clickedTab >= 0 && clickedTab != activeTabIndex_) {
    setActiveTab(tabs_[static_cast<size_t>(clickedTab)].id, true);
  }

  if (dragReorderEnabled_ && clickedTab >= 0) {
    draggedTabIndex_ = clickedTab;
    dragStartPos_ = e.getPosition();
  }
}

void TabGroup::mouseDrag(const juce::MouseEvent &e) {
  if (draggedTabIndex_ < 0)
    return;

  // Check for tab reorder
  int targetTab = getTabIndexAtPosition(e.getPosition());
  if (targetTab >= 0 && targetTab != draggedTabIndex_) {
    // Swap tabs
    std::swap(tabs_[static_cast<size_t>(draggedTabIndex_)],
              tabs_[static_cast<size_t>(targetTab)]);

    if (activeTabIndex_ == draggedTabIndex_) {
      activeTabIndex_ = targetTab;
    } else if (activeTabIndex_ == targetTab) {
      activeTabIndex_ = draggedTabIndex_;
    }

    draggedTabIndex_ = targetTab;
    updateTabBounds();

    if (onTabReordered)
      onTabReordered(tabs_[static_cast<size_t>(targetTab)].id, targetTab);
  }

  // Check for drag-out
  if (e.getPosition().y > tabBarHeight + 20 || e.getPosition().y < -20) {
    if (onTabDraggedOut && draggedTabIndex_ >= 0)
      onTabDraggedOut(tabs_[static_cast<size_t>(draggedTabIndex_)].id,
                      e.getPosition());
  }
}

void TabGroup::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  draggedTabIndex_ = -1;
}

void TabGroup::addTab(const juce::String &tabId, const juce::String &title,
                      juce::Component *content) {
  TabInfo tab;
  tab.id = tabId;
  tab.title = title;
  tab.content = content;
  tabs_.push_back(tab);

  if (content) {
    addAndMakeVisible(content);
    content->setVisible(tabs_.size() == 1);
  }

  if (activeTabIndex_ < 0)
    activeTabIndex_ = 0;

  updateTabBounds();
  resized();
}

void TabGroup::removeTab(const juce::String &tabId) {
  for (size_t i = 0; i < tabs_.size(); ++i) {
    if (tabs_[i].id == tabId) {
      if (tabs_[i].content)
        removeChildComponent(tabs_[i].content);
      tabs_.erase(tabs_.begin() + static_cast<std::ptrdiff_t>(i));

      if (activeTabIndex_ >= static_cast<int>(tabs_.size())) {
        activeTabIndex_ = static_cast<int>(tabs_.size()) - 1;
      }
      break;
    }
  }

  updateTabBounds();
  resized();
}

void TabGroup::setActiveTab(const juce::String &tabId, bool animate) {
  juce::ignoreUnused(animate);

  for (size_t i = 0; i < tabs_.size(); ++i) {
    if (tabs_[i].id == tabId) {
      activeTabIndex_ = static_cast<int>(i);

      // Update visibility
      for (size_t j = 0; j < tabs_.size(); ++j) {
        if (tabs_[j].content)
          tabs_[j].content->setVisible(j == i);
      }

      if (onTabChanged)
        onTabChanged(tabId);
      repaint();
      break;
    }
  }
}

juce::String TabGroup::getActiveTabId() const {
  if (activeTabIndex_ >= 0 &&
      static_cast<size_t>(activeTabIndex_) < tabs_.size()) {
    return tabs_[static_cast<size_t>(activeTabIndex_)].id;
  }
  return {};
}

int TabGroup::getTabCount() const { return static_cast<int>(tabs_.size()); }

void TabGroup::updateTabBounds() {
  float x = 4.0f;
  float tabHeight = static_cast<float>(tabBarHeight) - 8.0f;

  for (auto &tab : tabs_) {
    // Measure tab width based on title
    float width =
        std::max(60.0f, static_cast<float>(tab.title.length()) * 8.0f + 16.0f);
    tab.bounds = juce::Rectangle<float>(x, 4.0f, width, tabHeight);
    x += width + 2.0f;
  }
}

int TabGroup::getTabIndexAtPosition(const juce::Point<int> &pos) const {
  auto posFloat = pos.toFloat();
  for (size_t i = 0; i < tabs_.size(); ++i) {
    if (tabs_[i].bounds.contains(posFloat)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void TabGroup::animateTabSwitch() {
  // Future: Add tab switch animation
}

std::unique_ptr<juce::AccessibilityHandler>
TabGroup::createAccessibilityHandler() {
  // Role: List (closest to TabList in JUCE's accessibility roles)
  // Use configurable title to distinguish multiple TabGroup instances
  setTitle(accessibilityTitle_);
  setDescription("Panel tab group");
  return std::make_unique<juce::AccessibilityHandler>(
      *this, juce::AccessibilityRole::list);
}

//==============================================================================
// ResizablePanelContainer Implementation
//==============================================================================

ResizablePanelContainer::ResizablePanelContainer() {
  layout::LayoutManager::getInstance().addChangeListener(this);
}

ResizablePanelContainer::~ResizablePanelContainer() {
  layout::LayoutManager::getInstance().removeChangeListener(this);
}

void ResizablePanelContainer::addPanel(std::unique_ptr<juce::Component> content,
                                       const layout::PanelConfig &config,
                                       SplitDirection direction) {
  insertPanel(std::move(content), config, static_cast<int>(panels_.size()),
              direction);
}

void ResizablePanelContainer::insertPanel(
    std::unique_ptr<juce::Component> content, const layout::PanelConfig &config,
    int insertIndex, SplitDirection direction) {

  splitDirection_ = direction;

  // Create wrapper
  PanelSlot slot;
  slot.ownedContent = std::move(content);
  slot.wrapper = std::make_unique<PanelWrapper>(
      config.id, slot.ownedContent.get(), config);

  // Connect callbacks
  slot.wrapper->onCollapseStateChanged = [this](PanelWrapper *) {
    recalculateLayout();
  };
  slot.wrapper->onSizeChanged = [this](PanelWrapper *) { recalculateLayout(); };

  addAndMakeVisible(slot.wrapper.get());

  // Calculate initial size ratio
  float totalFlex = 0.0f;
  for (const auto &p : panels_) {
    totalFlex += p.wrapper->getFlex();
  }
  totalFlex += config.flex;

  slot.sizeRatio = totalFlex > 0 ? config.flex / totalFlex : 1.0f;

  // Insert at position
  if (insertIndex >= static_cast<int>(panels_.size())) {
    panels_.push_back(std::move(slot));
  } else {
    panels_.insert(panels_.begin() + insertIndex, std::move(slot));
  }

  // Add divider if needed
  if (panels_.size() > 1) {
    auto divider =
        std::make_unique<PanelDivider>(direction == SplitDirection::Vertical);

    int dividerIdx = static_cast<int>(dividers_.size());
    divider->onDrag = [this, dividerIdx](float delta) {
      handleDividerDrag(dividerIdx, delta);
    };

    addAndMakeVisible(divider.get());
    dividers_.push_back(std::move(divider));
  }

  recalculateLayout();
}

void ResizablePanelContainer::removePanel(const juce::String &panelId) {
  for (size_t i = 0; i < panels_.size(); ++i) {
    if (panels_[i].wrapper->getPanelId() == panelId) {
      removeChildComponent(panels_[i].wrapper.get());
      panels_.erase(panels_.begin() + static_cast<std::ptrdiff_t>(i));

      // Remove corresponding divider
      if (i > 0 && !dividers_.empty()) {
        removeChildComponent(dividers_[i - 1].get());
        dividers_.erase(dividers_.begin() + static_cast<std::ptrdiff_t>(i - 1));
      } else if (!dividers_.empty()) {
        removeChildComponent(dividers_.back().get());
        dividers_.pop_back();
      }
      break;
    }
  }

  recalculateLayout();
}

PanelWrapper *
ResizablePanelContainer::getPanel(const juce::String &panelId) const {
  for (const auto &slot : panels_) {
    if (slot.wrapper->getPanelId() == panelId) {
      return slot.wrapper.get();
    }
  }
  return nullptr;
}

juce::StringArray ResizablePanelContainer::getPanelIds() const {
  juce::StringArray ids;
  for (const auto &slot : panels_) {
    ids.add(slot.wrapper->getPanelId());
  }
  return ids;
}

void ResizablePanelContainer::setSplitDirection(SplitDirection direction) {
  if (splitDirection_ != direction) {
    splitDirection_ = direction;

    // Update divider orientations
    for (auto &divider : dividers_) {
      divider =
          std::make_unique<PanelDivider>(direction == SplitDirection::Vertical);
      addAndMakeVisible(divider.get());
    }

    recalculateLayout();
  }
}

void ResizablePanelContainer::setDividerPosition(int dividerIndex, float ratio,
                                                 bool animate) {
  juce::ignoreUnused(animate);

  if (dividerIndex < 0 || static_cast<size_t>(dividerIndex) >= dividers_.size())
    return;

  dividers_[static_cast<size_t>(dividerIndex)]->setPositionRatio(ratio);
  recalculateLayout();
}

void ResizablePanelContainer::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_DARKEST);
  canvas->drawRect(skBounds, bgPaint);

  // Draw panels (via child component rendering)
  for (const auto &slot : panels_) {
    if (slot.wrapper && slot.wrapper->isVisible()) {
      canvas->save();
      auto panelBounds = slot.wrapper->getBounds();
      canvas->translate(static_cast<float>(panelBounds.getX()),
                        static_cast<float>(panelBounds.getY()));
      canvas->clipRect(
          SkRect::MakeWH(static_cast<float>(panelBounds.getWidth()),
                         static_cast<float>(panelBounds.getHeight())));
      slot.wrapper->drawSkia(canvas);
      canvas->restore();
    }
  }

  // Draw dividers
  for (const auto &divider : dividers_) {
    if (divider && divider->isVisible()) {
      canvas->save();
      auto divBounds = divider->getBounds();
      canvas->translate(static_cast<float>(divBounds.getX()),
                        static_cast<float>(divBounds.getY()));
      canvas->clipRect(
          SkRect::MakeWH(static_cast<float>(divBounds.getWidth()),
                         static_cast<float>(divBounds.getHeight())));
      divider->drawSkia(canvas);
      canvas->restore();
    }
  }
}

void ResizablePanelContainer::resized() { recalculateLayout(); }

void ResizablePanelContainer::recalculateLayout() {
  if (panels_.empty())
    return;

  auto bounds = getLocalBounds();
  bool isHorizontal = (splitDirection_ == SplitDirection::Horizontal);

  // Calculate total available space (minus dividers)
  float totalSpace =
      static_cast<float>(isHorizontal ? bounds.getWidth() : bounds.getHeight());
  float dividerSpace =
      static_cast<float>(dividers_.size() * PanelDivider::dividerSize);
  float availableSpace = totalSpace - dividerSpace;

  // Calculate total flex and fixed sizes
  float totalFlex = 0.0f;
  float fixedSpace = 0.0f;

  for (const auto &slot : panels_) {
    if (slot.wrapper->isCollapsed()) {
      fixedSpace += static_cast<float>(PanelHeader::headerHeight);
    } else if (slot.wrapper->getFlex() == 0.0f) {
      fixedSpace += slot.wrapper->getCurrentSize();
    } else {
      totalFlex += slot.wrapper->getFlex();
    }
  }

  float flexibleSpace = availableSpace - fixedSpace;

  // Position panels and dividers
  float currentPos = 0.0f;
  size_t dividerIdx = 0;

  for (size_t i = 0; i < panels_.size(); ++i) {
    auto &slot = panels_[i];

    // Calculate panel size
    float panelSize;
    if (slot.wrapper->isCollapsed()) {
      panelSize = static_cast<float>(PanelHeader::headerHeight);
    } else if (slot.wrapper->getFlex() == 0.0f) {
      panelSize = slot.wrapper->getCurrentSize();
    } else {
      panelSize = (slot.wrapper->getFlex() / totalFlex) * flexibleSpace;
    }

    // Enforce minimum sizes
    panelSize = std::max(panelSize, slot.wrapper->getMinSize());

    // Position panel
    if (isHorizontal) {
      slot.wrapper->setBounds(static_cast<int>(currentPos), 0,
                              static_cast<int>(panelSize), bounds.getHeight());
    } else {
      slot.wrapper->setBounds(0, static_cast<int>(currentPos),
                              bounds.getWidth(), static_cast<int>(panelSize));
    }

    currentPos += panelSize;

    // Position divider after panel (except last)
    if (i < panels_.size() - 1 && dividerIdx < dividers_.size()) {
      auto &divider = dividers_[dividerIdx];
      if (isHorizontal) {
        divider->setBounds(static_cast<int>(currentPos), 0,
                           PanelDivider::dividerSize, bounds.getHeight());
      } else {
        divider->setBounds(0, static_cast<int>(currentPos), bounds.getWidth(),
                           PanelDivider::dividerSize);
      }
      currentPos += PanelDivider::dividerSize;
      ++dividerIdx;
    }
  }

  // Debug Log Layout (limited to first 3 calls to avoid lag)
  static int layoutLogLimit = 0;
  if (layoutLogLimit < 3) {
    ZENITH_LOG_INFO(
        juce::String("ResizablePanelContainer::recalculateLayout (") +
        (isHorizontal ? "Horizontal" : "Vertical") + ")");
    ZENITH_LOG_INFO("  Bounds: " + bounds.toString());
    for (size_t i = 0; i < panels_.size(); ++i) {
      auto &slot = panels_[i];
      ZENITH_LOG_INFO("  Panel " + juce::String(i) + " (" +
                      slot.wrapper->getPanelId() +
                      "): " + slot.wrapper->getBounds().toString() +
                      (slot.wrapper->isVisible() ? " [Visible]" : " [Hidden]"));
    }
    layoutLogLimit++;
  }
}

void ResizablePanelContainer::handleDividerDrag(int dividerIndex,
                                                float deltaPixels) {
  if (dividerIndex < 0 || static_cast<size_t>(dividerIndex) >= dividers_.size())
    return;

  auto sz = static_cast<size_t>(dividerIndex);

  // Resize the two adjacent panels
  auto &panel1 = panels_[sz];
  auto &panel2 = panels_[sz + 1];

  float size1 = panel1.wrapper->getCurrentSize() + deltaPixels;
  float size2 = panel2.wrapper->getCurrentSize() - deltaPixels;

  // Enforce minimum sizes
  float min1 = panel1.wrapper->getMinSize();
  float min2 = panel2.wrapper->getMinSize();

  if (size1 < min1) {
    float adjustment = min1 - size1;
    size1 = min1;
    size2 -= adjustment;
  }
  if (size2 < min2) {
    float adjustment = min2 - size2;
    size2 = min2;
    size1 -= adjustment;
  }

  panel1.wrapper->setCurrentSize(size1);
  panel2.wrapper->setCurrentSize(size2);

  recalculateLayout();
}

void ResizablePanelContainer::constrainPanelSizes() {
  // Ensure all panels meet minimum size requirements
  for (auto &slot : panels_) {
    float currentSize = slot.wrapper->getCurrentSize();
    float minSize = slot.wrapper->getMinSize();
    if (currentSize < minSize) {
      slot.wrapper->setCurrentSize(minSize);
    }
  }
}

void ResizablePanelContainer::applyLayoutConfig(
    const layout::LayoutConfig &config) {
  // Clear existing panels
  for (auto &slot : panels_) {
    removeChildComponent(slot.wrapper.get());
  }
  panels_.clear();

  for (auto &divider : dividers_) {
    removeChildComponent(divider.get());
  }
  dividers_.clear();

  // Build from config
  // Simplified: just add panels in order
  auto &layoutMgr = layout::LayoutManager::getInstance();

  for (const auto &panelConfig : config.panels) {
    auto content = layoutMgr.createPanel(panelConfig.type);
    if (content) {
      addPanel(std::move(content), panelConfig, splitDirection_);
    }
  }

  // Apply divider positions
  for (size_t i = 0; i < config.dividers.size() && i < dividers_.size(); ++i) {
    dividers_[i]->setPositionRatio(
        config.dividers[static_cast<int>(i)].position);
  }

  recalculateLayout();
}

layout::LayoutConfig
ResizablePanelContainer::captureLayoutConfig(const juce::String &name) const {
  layout::LayoutConfig config;
  config.id = juce::Uuid().toString();
  config.name = name;
  config.lastModified = juce::Time::getCurrentTime();

  // Capture panel configs
  for (const auto &slot : panels_) {
    layout::PanelConfig panelConfig = slot.wrapper->getConfig();
    panelConfig.initialSize = slot.wrapper->getCurrentSize();
    panelConfig.isCollapsed = slot.wrapper->isCollapsed();
    config.panels.add(panelConfig);
  }

  // Capture divider positions
  for (const auto &divider : dividers_) {
    layout::DividerConfig divConfig;
    divConfig.id = juce::Uuid().toString();
    divConfig.position = divider->getPositionRatio();
    divConfig.isHorizontal = divider->isHorizontal();
    config.dividers.add(divConfig);
  }

  return config;
}

TabGroup *ResizablePanelContainer::createTabGroup(int panelIndex) {
  auto group = std::make_unique<TabGroup>();
  auto *groupPtr = group.get();
  addAndMakeVisible(groupPtr);
  tabGroups_.push_back(std::move(group));

  juce::ignoreUnused(panelIndex);
  // Future: Replace panel at index with tab group

  return groupPtr;
}

void ResizablePanelContainer::addPanelToTabGroup(const juce::String &panelId,
                                                 TabGroup *group) {
  auto *panel = getPanel(panelId);
  if (!panel || !group)
    return;

  group->addTab(panelId, panel->getConfig().name, panel->getContent());
}

void ResizablePanelContainer::changeListenerCallback(
    juce::ChangeBroadcaster *source) {
  juce::ignoreUnused(source);
  // Handle layout manager changes (e.g., animation settings)
  repaint();
}

} // namespace zenith
