/**
 * @file RightSidePanel.cpp
 * @brief Implementation of right-side panel
 */

#include "RightSidePanel.h"
#include "../WingmanPanel.h"
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkDashPathEffect.h>

namespace zenith {

//==============================================================================
// ScratchPadsPanel Implementation
//==============================================================================

RightSidePanel::ScratchPadsPanel::ScratchPadsPanel() { setSize(400, 300); }

void RightSidePanel::ScratchPadsPanel::paintSkia(
    SkCanvas &canvas, const juce::Rectangle<int> &bounds) {
  auto &colors = SkiaTheme::getInstance().getColors();

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(colors.bg1);

  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  canvas.drawRect(skBounds, bgPaint);

  // Header bar
  SkPaint headerPaint;
  headerPaint.setAntiAlias(true);
  headerPaint.setColor(colors.bg2);

  SkRect headerRect = SkRect::MakeXYWH(0, 0, skBounds.width(), 40);
  canvas.drawRect(headerRect, headerPaint);

  // Bottom border on header
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(colors.borderSubtle);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);

  canvas.drawLine(0, 40, skBounds.width(), 40, borderPaint);

  // Title text
  SkFont titleFont;
  titleFont.setSize(14.0f);

  SkPaint titlePaint;
  titlePaint.setAntiAlias(true);
  titlePaint.setColor(colors.textStrong);

  canvas.drawString("Scratch Pads", 16, 26, titleFont, titlePaint);

  // Placeholder content
  SkFont bodyFont;
  bodyFont.setSize(12.0f);

  SkPaint bodyPaint;
  bodyPaint.setAntiAlias(true);
  bodyPaint.setColor(colors.textMuted);

  float centerX = skBounds.centerX();
  float centerY = skBounds.centerY();

  // Draw placeholder icon (dashed rectangle)
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);
  iconPaint.setColor(colors.textSubtle);
  iconPaint.setStyle(SkPaint::kStroke_Style);
  iconPaint.setStrokeWidth(2.0f);

  const float intervals[] = {8.0f, 4.0f};
  iconPaint.setPathEffect(SkDashPathEffect::Make(intervals, 2));

  SkRect placeholderRect =
      SkRect::MakeXYWH(centerX - 60, centerY - 40, 120, 80);
  SkRRect placeholderRRect = SkRRect::MakeRectXY(placeholderRect, 6.0f, 6.0f);

  canvas.drawRRect(placeholderRRect, iconPaint);

  // Placeholder text
  const char *line1 = "Scratch Pads";
  const char *line2 = "Coming Soon";

  SkRect textBounds1, textBounds2;
  bodyFont.measureText(line1, strlen(line1), SkTextEncoding::kUTF8,
                       &textBounds1);
  bodyFont.measureText(line2, strlen(line2), SkTextEncoding::kUTF8,
                       &textBounds2);

  canvas.drawString(line1, centerX - textBounds1.width() / 2, centerY + 60,
                    bodyFont, bodyPaint);

  bodyPaint.setColor(colors.textSubtle);
  canvas.drawString(line2, centerX - textBounds2.width() / 2, centerY + 76,
                    bodyFont, bodyPaint);
}

//==============================================================================
// RightSidePanel Construction
//==============================================================================

RightSidePanel::RightSidePanel() {
  // Create scratch pads panel
  scratchPadsPanel_ = std::make_unique<ScratchPadsPanel>();
  addAndMakeVisible(scratchPadsPanel_.get());

  setSize(400, 600);
}

//==============================================================================
// Panel Access
//==============================================================================

void RightSidePanel::setWingmanPanel(WingmanPanel *panel) {
  if (wingmanPanel_ != panel) {
    wingmanPanel_ = panel;
    if (wingmanPanel_) {
      addAndMakeVisible(wingmanPanel_);
      resized();
    }
  }
}

void RightSidePanel::setWingmanPanelHeight(int height) {
  wingmanHeight_ = juce::jmax(MIN_WINGMAN_HEIGHT, height);
  resized();
}

//==============================================================================
// Component Overrides
//==============================================================================

void RightSidePanel::resized() {
  auto bounds = getLocalBounds();

  // Ensure wingman height fits
  wingmanHeight_ = juce::jlimit(MIN_WINGMAN_HEIGHT, bounds.getHeight() - 100,
                                wingmanHeight_);

  // Bottom: Wingman panel
  if (wingmanPanel_) {
    auto wingmanBounds = bounds.removeFromBottom(wingmanHeight_);
    wingmanPanel_->setBounds(wingmanBounds);
  }

  // Splitter
  bounds.removeFromBottom(SPLITTER_HEIGHT);

  // Top: Scratch Pads
  scratchPadsPanel_->setBounds(bounds);
}

void RightSidePanel::paint(juce::Graphics &g) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();

  // Draw splitter
  auto splitterBounds = getSplitterBounds();

  juce::Colour splitterColor = juce::Colour(
      isSplitterHovered_ ? colors.borderStrong : colors.borderSubtle);

  g.setColour(splitterColor);
  g.fillRect(splitterBounds);

  // Draw drag handle in center
  if (isSplitterHovered_) {
    juce::Colour handleColor = juce::Colour(colors.textSubtle);
    g.setColour(handleColor);

    int handleWidth = 40;
    int handleHeight = 3;
    int handleX = splitterBounds.getCentreX() - handleWidth / 2;
    int handleY = splitterBounds.getCentreY() - handleHeight / 2;

    g.fillRoundedRectangle(handleX, handleY, handleWidth, handleHeight, 1.5f);
  }
}

void RightSidePanel::paintToSkia(SkCanvas *canvas, SkRect bounds) {
  auto &theme = SkiaTheme::getInstance();
  auto &colors = theme.getColors();

  // 1. Draw splitter
  auto splitterBounds = getSplitterBounds();
  SkRect skSplitterBounds =
      SkRect::MakeXYWH(bounds.left() + splitterBounds.getX(),
                       bounds.top() + splitterBounds.getY(),
                       splitterBounds.getWidth(), splitterBounds.getHeight());

  SkPaint splitterPaint;
  splitterPaint.setAntiAlias(true);
  splitterPaint.setColor(isSplitterHovered_ ? colors.borderStrong
                                            : colors.borderSubtle);
  canvas->drawRect(skSplitterBounds, splitterPaint);

  // Draw drag handle in center
  if (isSplitterHovered_) {
    SkPaint handlePaint;
    handlePaint.setAntiAlias(true);
    handlePaint.setColor(colors.textSubtle);

    float handleWidth = 40.0f;
    float handleHeight = 3.0f;
    float handleX = skSplitterBounds.centerX() - handleWidth / 2;
    float handleY = skSplitterBounds.centerY() - handleHeight / 2;

    SkRRect handleRRect = SkRRect::MakeRectXY(
        SkRect::MakeXYWH(handleX, handleY, handleWidth, handleHeight), 1.5f,
        1.5f);
    canvas->drawRRect(handleRRect, handlePaint);
  }

  // 2. Draw children
  for (auto *child : getChildren()) {
    if (!child->isVisible())
      continue;

    auto childBounds = child->getBounds();
    SkRect childSkBounds = SkRect::MakeXYWH(
        bounds.left() + childBounds.getX(), bounds.top() + childBounds.getY(),
        childBounds.getWidth(), childBounds.getHeight());

    auto *skiaChild = dynamic_cast<SkiaComponent *>(child);
    if (skiaChild && skiaChild->supportsSkiaRendering()) {
      // Native Skia rendering
      canvas->save();
      // Optional: clip to child bounds to prevent bleeding
      canvas->clipRect(childSkBounds);
      skiaChild->paintToSkia(canvas, childSkBounds);
      canvas->restore();
    } else {
      // JUCE Fallback rendering for non-Skia children (e.g. WingmanPanel)
      juce::Image componentImage(juce::Image::ARGB,
                                 juce::jmax(1, childBounds.getWidth()),
                                 juce::jmax(1, childBounds.getHeight()), true);

      juce::Graphics componentGraphics(componentImage);
      componentGraphics.setOrigin(-childBounds.getX(), -childBounds.getY());
      child->paint(componentGraphics);
      // Note: We don't recursively paint children of JUCE components here for
      // simplicity, assuming they handle their own painting or are leaf
      // components. For complex hierarchies, we might need paintEntireComponent
      // but that's heavier. child->paintEntireComponent(componentGraphics,
      // false); Actually, paint() is usually enough for simple components, but
      // for containers we need more. Let's stick to paint() for now as
      // WingmanPanel is likely self-contained or we accept this limitation.

      // Convert JUCE image to Skia
      juce::Image::BitmapData bitmapData(componentImage,
                                         juce::Image::BitmapData::readOnly);

      SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(
          componentImage.getWidth(), componentImage.getHeight());

      sk_sp<SkImage> skiaImage = SkImages::RasterFromPixmapCopy(
          SkPixmap(imageInfo, bitmapData.data, bitmapData.lineStride));

      if (skiaImage) {
        SkPaint paint;
        paint.setAntiAlias(true);
        canvas->drawImage(skiaImage, childSkBounds.left(), childSkBounds.top(),
                          SkSamplingOptions(SkFilterMode::kLinear), &paint);
      }
    }
  }
}

void RightSidePanel::mouseDown(const juce::MouseEvent &event) {
  if (isSplitterHovered(event.getPosition())) {
    isDraggingSplitter_ = true;
    dragStartY_ = event.getPosition().y;
    dragStartHeight_ = wingmanHeight_;
  }
}

void RightSidePanel::mouseDrag(const juce::MouseEvent &event) {
  if (isDraggingSplitter_) {
    int deltaY = event.getPosition().y - dragStartY_;
    int newHeight =
        dragStartHeight_ - deltaY; // Invert because we're sizing from bottom

    setWingmanPanelHeight(newHeight);
  }
}

void RightSidePanel::mouseMove(const juce::MouseEvent &event) {
  bool wasHovered = isSplitterHovered_;
  isSplitterHovered_ = isSplitterHovered(event.getPosition());

  if (wasHovered != isSplitterHovered_) {
    repaint(getSplitterBounds());

    // Update cursor
    setMouseCursor(isSplitterHovered_ ? juce::MouseCursor::UpDownResizeCursor
                                      : juce::MouseCursor::NormalCursor);
  }
}

//==============================================================================
// Internal Methods
//==============================================================================

juce::Rectangle<int> RightSidePanel::getSplitterBounds() const {
  int y = getHeight() - wingmanHeight_ - SPLITTER_HEIGHT;
  return juce::Rectangle<int>(0, y, getWidth(), SPLITTER_HEIGHT);
}

bool RightSidePanel::isSplitterHovered(const juce::Point<int> &point) const {
  return getSplitterBounds().expanded(0, 2).contains(point);
}

} // namespace zenith
