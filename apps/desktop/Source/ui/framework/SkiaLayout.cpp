/*
  ==============================================================================

    SkiaLayout.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Unified layout management system implementation

  ==============================================================================
*/

#include "SkiaLayout.h"
#include "ZenithDesignSystem.h"

namespace zenith {
namespace layout {

// ============================================================================
// SkiaLayoutContainer Implementation
// ============================================================================

SkiaLayoutContainer::SkiaLayoutContainer() { setName("SkiaLayoutContainer"); }

SkiaLayoutContainer::~SkiaLayoutContainer() { removeAllChildren(); }

void SkiaLayoutContainer::addChild(SkiaComponent *child,
                                   const LayoutParams &params) {
  if (child && !children_.contains([child](const ChildInfo &info) {
        return info.component == child;
      })) {
    ChildInfo info;
    info.component = child;
    info.params = params;
    children_.add(info);
    SkiaComponent::addAndMakeVisible(child);
    markDirty();
  }
}

void SkiaLayoutContainer::addChild(SkiaComponent *child, float flex,
                                   Alignment alignment) {
  LayoutParams params(flex, alignment);
  addChild(child, params);
}

void SkiaLayoutContainer::removeChild(SkiaComponent *child) {
  for (int i = 0; i < children_.size(); ++i) {
    if (children_[i].component == child) {
      children_.remove(i);
      SkiaComponent::removeChildComponent(child);
      markDirty();
      break;
    }
  }
}

void SkiaLayoutContainer::removeAllChildren() {
  for (const auto &childInfo : children_) {
    SkiaComponent::removeChildComponent(childInfo.component);
  }
  children_.clear();
  markDirty();
}

void SkiaLayoutContainer::setSpacing(float spacing) {
  spacing_ = Spacing(spacing);
  markDirty();
}

void SkiaLayoutContainer::setSpacing(float horizontal, float vertical) {
  spacing_ = Spacing(vertical, horizontal);
  markDirty();
}

void SkiaLayoutContainer::setPadding(const Spacing &padding) {
  padding_ = padding;
  markDirty();
}

void SkiaLayoutContainer::setDirection(Direction direction) {
  if (direction_ != direction) {
    direction_ = direction;
    markDirty();
  }
}

void SkiaLayoutContainer::setAlignment(Alignment alignment) {
  if (alignment_ != alignment) {
    alignment_ = alignment;
    markDirty();
  }
}

void SkiaLayoutContainer::setChildAlignment(SkiaComponent *child,
                                            Alignment alignment) {
  for (auto &childInfo : children_) {
    if (childInfo.component == child) {
      childInfo.params.alignment = alignment;
      markDirty();
      break;
    }
  }
}

void SkiaLayoutContainer::drawSkia(SkCanvas *canvas) {
  // Layout containers don't draw anything themselves
  // They just position their children
  juce::ignoreUnused(canvas);
}

void SkiaLayoutContainer::resized() { calculateLayout(); }

void SkiaLayoutContainer::calculateLayout() {
  if (children_.isEmpty()) {
    return;
  }

  auto bounds = getLocalBounds().toFloat();

  // Apply padding
  auto contentBounds = LayoutUtils::applyPadding(bounds, padding_);

  // Calculate total flex and available space
  float totalFlex = 0.0f;
  float availableSpace = (direction_ == Direction::Horizontal)
                             ? contentBounds.getWidth()
                             : contentBounds.getHeight();

  // Subtract spacing between items
  if (children_.size() > 1) {
    float spacing = (direction_ == Direction::Horizontal)
                        ? spacing_.left + spacing_.right
                        : spacing_.top + spacing_.bottom;
    availableSpace -= spacing * (children_.size() - 1);
  }

  // Calculate flex values and total flex
  juce::Array<float> flexValues;
  juce::Array<float> minSizes;
  juce::Array<float> maxSizes;
  juce::Array<float> preferredSizes;

  for (const auto &childInfo : children_) {
    float flex = childInfo.params.flex;
    totalFlex += flex;
    flexValues.add(flex);

    float minSize = childInfo.params.minSize;
    float maxSize = childInfo.params.maxSize;
    float preferredSize = childInfo.params.preferredSize;

    // If no preferred size, use component's current size
    if (preferredSize <= 0) {
      preferredSize = (direction_ == Direction::Horizontal)
                          ? childInfo.component->getWidth()
                          : childInfo.component->getHeight();
    }

    minSizes.add(minSize);
    maxSizes.add(maxSize);
    preferredSizes.add(preferredSize);
  }

  // Calculate final sizes using flex distribution
  juce::Array<float> finalSizes;
  if (totalFlex > 0) {
    finalSizes = LayoutUtils::calculateFlexDistribution(
        flexValues, availableSpace, minSizes, maxSizes, preferredSizes);
  } else {
    // No flex - use preferred sizes
    for (float prefSize : preferredSizes) {
      finalSizes.add(prefSize);
    }
  }

  // Position children
  float currentPosition = (direction_ == Direction::Horizontal)
                              ? contentBounds.getX()
                              : contentBounds.getY();

  for (int i = 0; i < children_.size(); ++i) {
    const auto &childInfo = children_[i];
    float size = finalSizes[i];

    // Calculate child bounds based on direction and alignment
    juce::Rectangle<float> childBounds;

    if (direction_ == Direction::Horizontal) {
      float y = LayoutUtils::alignItem(childInfo.component->getHeight(),
                                       contentBounds.getHeight(),
                                       childInfo.params.alignment);

      childBounds =
          juce::Rectangle<float>(currentPosition, contentBounds.getY() + y,
                                 size, contentBounds.getHeight());

      currentPosition += size + spacing_.left + spacing_.right;
    } else {
      float x = LayoutUtils::alignItem(childInfo.component->getWidth(),
                                       contentBounds.getWidth(),
                                       childInfo.params.alignment);

      childBounds =
          juce::Rectangle<float>(contentBounds.getX() + x, currentPosition,
                                 contentBounds.getWidth(), size);

      currentPosition += size + spacing_.top + spacing_.bottom;
    }

    // Apply margin to child bounds
    childBounds =
        LayoutUtils::applyMargin(childBounds, childInfo.params.margin);

    // Set component bounds
    childInfo.component->setBounds(childBounds.toNearestInt());
  }
}

juce::Rectangle<float> SkiaLayoutContainer::getChildBounds(
    const ChildInfo &child, const juce::Rectangle<float> &availableBounds,
    float flexTotal, float availableSpace) {
  juce::ignoreUnused(child, availableBounds, flexTotal, availableSpace);
  return {};
}

// ============================================================================
// SkiaHorizontalLayout Implementation
// ============================================================================

SkiaHorizontalLayout::SkiaHorizontalLayout() {
  setName("SkiaHorizontalLayout");
  setDirection(Direction::Horizontal);
}

SkiaHorizontalLayout::~SkiaHorizontalLayout() {}

void SkiaHorizontalLayout::addChild(SkiaComponent *child, float flex,
                                    Alignment verticalAlignment) {
  LayoutParams params(flex, verticalAlignment);
  SkiaLayoutContainer::addChild(child, params);
}

// ============================================================================
// SkiaVerticalLayout Implementation
// ============================================================================

SkiaVerticalLayout::SkiaVerticalLayout() {
  setName("SkiaVerticalLayout");
  setDirection(Direction::Vertical);
}

SkiaVerticalLayout::~SkiaVerticalLayout() {}

void SkiaVerticalLayout::addChild(SkiaComponent *child, float flex,
                                  Alignment horizontalAlignment) {
  LayoutParams params(flex, horizontalAlignment);
  SkiaLayoutContainer::addChild(child, params);
}

// ============================================================================
// SkiaGridLayout Implementation
// ============================================================================

SkiaGridLayout::SkiaGridLayout(int rows, int columns)
    : rows_(rows), columns_(columns) {
  setName("SkiaGridLayout");
  rowFlex_.resize(rows);
  columnFlex_.resize(columns);

  // Initialize with default flex values
  for (int i = 0; i < rows; ++i) {
    rowFlex_.set(i, 0.0f);
  }
  for (int i = 0; i < columns; ++i) {
    columnFlex_.set(i, 0.0f);
  }
}

SkiaGridLayout::~SkiaGridLayout() {}

void SkiaGridLayout::setRowSpacing(float spacing) {
  rowSpacing_ = spacing;
  markDirty();
}

void SkiaGridLayout::setColumnSpacing(float spacing) {
  columnSpacing_ = spacing;
  markDirty();
}

void SkiaGridLayout::setRowFlex(int row, float flex) {
  if (row >= 0 && row < rows_) {
    rowFlex_.set(row, flex);
    markDirty();
  }
}

void SkiaGridLayout::setColumnFlex(int column, float flex) {
  if (column >= 0 && column < columns_) {
    columnFlex_.set(column, flex);
    markDirty();
  }
}

void SkiaGridLayout::setCellPadding(const Spacing &padding) {
  cellPadding_ = padding;
  markDirty();
}

void SkiaGridLayout::addChildAt(SkiaComponent *child, int row, int column,
                                int rowSpan, int columnSpan,
                                const LayoutParams &params) {
  if (child && row >= 0 && row < rows_ && column >= 0 && column < columns_ &&
      rowSpan > 0 && columnSpan > 0 && row + rowSpan <= rows_ &&
      column + columnSpan <= columns_) {

    GridCell cell;
    cell.component = child;
    cell.row = row;
    cell.column = column;
    cell.rowSpan = rowSpan;
    cell.columnSpan = columnSpan;
    cell.params = params;

    gridChildren_.add(cell);
    SkiaComponent::addAndMakeVisible(child);
    markDirty();
  }
}

void SkiaGridLayout::calculateLayout() {
  if (gridChildren_.isEmpty()) {
    return;
  }

  auto bounds = getLocalBounds().toFloat();
  auto contentBounds = LayoutUtils::applyPadding(bounds, padding_);

  // Calculate row and column sizes using flex distribution
  juce::Array<float> rowHeights;
  juce::Array<float> columnWidths;

  float totalRowFlex = 0.0f;
  float totalColumnFlex = 0.0f;

  for (float flex : rowFlex_) {
    totalRowFlex += flex;
  }
  for (float flex : columnFlex_) {
    totalColumnFlex += flex;
  }

  // Default to equal distribution if no flex specified
  if (totalRowFlex == 0.0f) {
    for (int i = 0; i < rows_; ++i) {
      rowFlex_.set(i, 1.0f);
      totalRowFlex += 1.0f;
    }
  }
  if (totalColumnFlex == 0.0f) {
    for (int i = 0; i < columns_; ++i) {
      columnFlex_.set(i, 1.0f);
      totalColumnFlex += 1.0f;
    }
  }

  // Calculate available space
  float availableHeight = contentBounds.getHeight() - (rows_ - 1) * rowSpacing_;
  float availableWidth =
      contentBounds.getWidth() - (columns_ - 1) * columnSpacing_;

  // Distribute space to rows and columns
  for (int i = 0; i < rows_; ++i) {
    float height = (availableHeight * rowFlex_[i]) / totalRowFlex;
    rowHeights.add(height);
  }
  for (int i = 0; i < columns_; ++i) {
    float width = (availableWidth * columnFlex_[i]) / totalColumnFlex;
    columnWidths.add(width);
  }

  // Position grid children
  for (const auto &cell : gridChildren_) {
    auto cellBounds = getCellBounds(cell.row, cell.column, cell.rowSpan,
                                    cell.columnSpan, contentBounds);

    // Apply cell padding
    cellBounds = LayoutUtils::applyPadding(cellBounds, cellPadding_);
    cellBounds = LayoutUtils::applyMargin(cellBounds, cell.params.margin);

    cell.component->setBounds(cellBounds.toNearestInt());
  }
}

juce::Rectangle<float>
SkiaGridLayout::getCellBounds(int row, int column, int rowSpan, int columnSpan,
                              const juce::Rectangle<float> &gridBounds) {
  float x = gridBounds.getX();
  float y = gridBounds.getY();

  // Calculate x position
  for (int i = 0; i < column; ++i) {
    x += columnWidths[i] + columnSpacing_;
  }

  // Calculate y position
  for (int i = 0; i < row; ++i) {
    y += rowHeights[i] + rowSpacing_;
  }

  // Calculate width and height
  float width = 0.0f;
  for (int i = column; i < column + columnSpan; ++i) {
    width += columnWidths[i];
    if (i > column) {
      width += columnSpacing_;
    }
  }

  float height = 0.0f;
  for (int i = row; i < row + rowSpan; ++i) {
    height += rowHeights[i];
    if (i > row) {
      height += rowSpacing_;
    }
  }

  return juce::Rectangle<float>(x, y, width, height);
}

// ============================================================================
// SkiaStackLayout Implementation
// ============================================================================

SkiaStackLayout::SkiaStackLayout() { setName("SkiaStackLayout"); }

SkiaStackLayout::~SkiaStackLayout() {}

void SkiaStackLayout::addChild(SkiaComponent *child,
                               Alignment horizontalAlignment,
                               Alignment verticalAlignment) {
  LayoutParams params;
  params.alignment = horizontalAlignment; // Store horizontal alignment
  // Vertical alignment would need to be stored separately in a more complex
  // implementation
  SkiaLayoutContainer::addChild(child, params);
}

void SkiaStackLayout::calculateLayout() {
  auto bounds = getLocalBounds().toFloat();
  auto contentBounds = LayoutUtils::applyPadding(bounds, padding_);

  // Stack all children to fill the container
  for (const auto &childInfo : children_) {
    juce::Rectangle<float> childBounds = contentBounds;

    // Apply alignment if not stretch
    if (childInfo.params.alignment != Alignment::Stretch) {
      float childWidth = childInfo.component->getWidth();
      float childHeight = childInfo.component->getHeight();

      float x = LayoutUtils::alignItem(childWidth, contentBounds.getWidth(),
                                       childInfo.params.alignment);
      float y = LayoutUtils::alignItem(childHeight, contentBounds.getHeight(),
                                       childInfo.params.alignment);

      childBounds = juce::Rectangle<float>(contentBounds.getX() + x,
                                           contentBounds.getY() + y, childWidth,
                                           childHeight);
    }

    childInfo.component->setBounds(childBounds.toNearestInt());
  }
}

// ============================================================================
// SkiaScrollLayout Implementation
// ============================================================================

SkiaScrollLayout::SkiaScrollLayout(Direction scrollDirection)
    : scrollDirection_(scrollDirection) {
  setName("SkiaScrollLayout");
}

SkiaScrollLayout::~SkiaScrollLayout() {}

void SkiaScrollLayout::setScrollEnabled(bool enabled) {
  scrollEnabled_ = enabled;
}

void SkiaScrollLayout::scrollTo(float position) {
  if (scrollEnabled_) {
    scrollPosition_ = juce::jlimit(0.0f, contentSize_ - getHeight(), position);
    markDirty();
  }
}

void SkiaScrollLayout::scrollToComponent(SkiaComponent *component) {
  if (!component || !scrollEnabled_) {
    return;
  }

  auto componentBounds = component->getBounds();
  float componentY = componentBounds.getY();
  float componentHeight = componentBounds.getHeight();

  // Scroll to make component visible
  if (componentY < scrollPosition_) {
    scrollTo(componentY);
  } else if (componentY + componentHeight > scrollPosition_ + getHeight()) {
    scrollTo(componentY + componentHeight - getHeight());
  }
}

void SkiaScrollLayout::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_DARKER);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  // Clip to visible area
  canvas->save();
  canvas->clipRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));

  // Draw children (they handle their own drawing)
  drawChildren(canvas);

  canvas->restore();
}
void SkiaScrollLayout::resized() { SkiaLayoutContainer::resized(); }
void SkiaScrollLayout::mouseDown(const juce::MouseEvent &e) {
  if (scrollEnabled_ && e.mods.isLeftButtonDown()) {
    dragStartPosition_ = (scrollDirection_ == Direction::Vertical)
                             ? e.position.y + scrollPosition_
                             : e.position.x + scrollPosition_;
  }
}

void SkiaLayoutContainer::mouseDrag(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Base implementation does nothing
}

void SkiaScrollLayout::mouseDrag(const juce::MouseEvent &e) {
  if (scrollEnabled_ && e.mods.isLeftButtonDown()) {
    float currentPosition =
        (scrollDirection_ == Direction::Vertical) ? e.position.y : e.position.x;
    float newScrollPosition = dragStartPosition_ - currentPosition;

    scrollTo(newScrollPosition);
  }
}

void SkiaScrollLayout::mouseWheelMove(const juce::MouseEvent &e,
                                      const juce::MouseWheelDetails &wheel) {
  juce::ignoreUnused(e);

  if (scrollEnabled_) {
    float scrollAmount = wheel.deltaY * 50.0f; // 50 pixels per wheel tick
    scrollTo(scrollPosition_ - scrollAmount);
  }
}

void SkiaScrollLayout::calculateLayout() {
  if (children_.isEmpty()) {
    return;
  }

  // Calculate total content size
  contentSize_ = 0.0f;
  float spacing = (scrollDirection_ == Direction::Vertical)
                      ? spacing_.top + spacing_.bottom
                      : spacing_.left + spacing_.right;

  for (const auto &childInfo : children_) {
    float childSize = (scrollDirection_ == Direction::Vertical)
                          ? childInfo.component->getHeight()
                          : childInfo.component->getWidth();
    contentSize_ += childSize + spacing;
  }

  if (!children_.isEmpty()) {
    contentSize_ -= spacing; // Remove last spacing
  }

  // Position children vertically with scroll offset
  float currentPosition = -scrollPosition_;

  for (const auto &childInfo : children_) {
    auto bounds = getLocalBounds().toFloat();

    if (scrollDirection_ == Direction::Vertical) {
      float childWidth = bounds.getWidth();
      float childHeight = childInfo.component->getHeight();

      childInfo.component->setBounds(
          juce::Rectangle<float>(0, currentPosition, childWidth, childHeight)
              .toNearestInt());

      currentPosition += childHeight + spacing_.top + spacing_.bottom;
    } else {
      float childWidth = childInfo.component->getWidth();
      float childHeight = bounds.getHeight();

      childInfo.component->setBounds(
          juce::Rectangle<float>(currentPosition, 0, childWidth, childHeight)
              .toNearestInt());

      currentPosition += childWidth + spacing_.left + spacing_.right;
    }
  }
}

void SkiaScrollLayout::updateScrollbars() {
  // Scrollbar implementation would go here
  // For now, just mark dirty to trigger redraw
  markDirty();
}

// ============================================================================
// LayoutUtils Implementation
// ============================================================================

juce::Array<float> LayoutUtils::calculateFlexDistribution(
    const juce::Array<float> &flexValues, float availableSpace,
    const juce::Array<float> &minSizes, const juce::Array<float> &maxSizes,
    const juce::Array<float> &preferredSizes) {

  juce::Array<float> result;
  int count = flexValues.size();

  if (count == 0) {
    return result;
  }

  result.resize(count);

  // First pass: try to satisfy preferred sizes
  float totalPreferred = 0.0f;
  for (float pref : preferredSizes) {
    totalPreferred += pref;
  }

  if (totalPreferred <= availableSpace) {
    // Enough space for preferred sizes
    for (int i = 0; i < count; ++i) {
      result.set(i, preferredSizes[i]);
    }
  } else {
    // Not enough space - use flex distribution
    float totalFlex = calculateTotalFlex(flexValues);

    if (totalFlex > 0.0f) {
      // Flex distribution
      for (int i = 0; i < count; ++i) {
        float size = (availableSpace * flexValues[i]) / totalFlex;

        // Apply min/max constraints
        size = juce::jlimit(minSizes[i], maxSizes[i], size);
        result.set(i, size);
      }
    } else {
      // Equal distribution
      float equalSize = availableSpace / count;
      for (int i = 0; i < count; ++i) {
        result.set(i, juce::jlimit(minSizes[i], maxSizes[i], equalSize));
      }
    }
  }

  return result;
}

float LayoutUtils::alignItem(float itemSize, float availableSize,
                             Alignment alignment) {
  switch (alignment) {
  case Alignment::Start:
    return 0.0f;

  case Alignment::Center:
    return (availableSize - itemSize) * 0.5f;

  case Alignment::End:
    return availableSize - itemSize;

  case Alignment::Stretch:
    return 0.0f; // Item will be stretched to fill available space

  default:
    return 0.0f;
  }
}

juce::Rectangle<float>
LayoutUtils::applyPadding(const juce::Rectangle<float> &bounds,
                          const Spacing &padding) {
  return juce::Rectangle<float>(
      bounds.getX() + padding.left, bounds.getY() + padding.top,
      bounds.getWidth() - padding.left - padding.right,
      bounds.getHeight() - padding.top - padding.bottom);
}

juce::Rectangle<float>
LayoutUtils::applyMargin(const juce::Rectangle<float> &bounds,
                         const Spacing &margin) {
  return juce::Rectangle<float>(
      bounds.getX() - margin.left, bounds.getY() - margin.top,
      bounds.getWidth() + margin.left + margin.right,
      bounds.getHeight() + margin.top + margin.bottom);
}

juce::Rectangle<float> LayoutUtils::measureText(const SkFont &font,
                                                const juce::String &text) {
  SkRect bounds;
  font.measureText(text.toRawUTF8(), text.length(), SkTextEncoding::kUTF8,
                   &bounds);

  return juce::Rectangle<float>(bounds.fLeft, bounds.fTop, bounds.width(),
                                bounds.height());
}

float LayoutUtils::calculateTotalFlex(const juce::Array<float> &flexValues) {
  float total = 0.0f;
  for (float flex : flexValues) {
    total += flex;
  }
  return total;
}

} // namespace layout
} // namespace zenith