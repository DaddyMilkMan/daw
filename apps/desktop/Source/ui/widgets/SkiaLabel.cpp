/*
  ==============================================================================

    SkiaLabel.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based label implementation

  ==============================================================================
*/

#include "SkiaLabel.h"
#include "ZenithDesignSystem.h"

namespace zenith {

SkiaLabel::SkiaLabel(const juce::String &componentName,
                     const juce::String &labelText) {
  setName(componentName);
  text_ = labelText;

  // Set default appearance
  font_.setSize(design::typography::FONT_MD);
  textColour_ = design::colors::TEXT_PRIMARY;
  backgroundColour_ = SkColorSetARGB(0, 0, 0, 0); // Transparent by default
  borderColour_ = design::colors::BORDER_DEFAULT;
}

SkiaLabel::~SkiaLabel() {}

void SkiaLabel::setText(const juce::String &text,
                        juce::NotificationType notification) {
  if (text_ != text) {
    text_ = text;
    markDirty();

    if (notification != juce::dontSendNotification && onTextChange) {
      onTextChange();
    }
  }
}

void SkiaLabel::setFont(const SkFont &font) {
  font_ = font;
  markDirty();
}

void SkiaLabel::setFont(float fontSize, SkFontStyle::Weight weight) {
  font_.setSize(fontSize);
  // Note: SkFont doesn't directly support weight in the same way as JUCE
  // This would need to be handled by loading different font files for different
  // weights
  markDirty();
}

void SkiaLabel::setTextColour(SkColor colour) {
  textColour_ = colour;
  markDirty();
}

void SkiaLabel::setBackgroundColour(SkColor colour) {
  backgroundColour_ = colour;
  markDirty();
}

void SkiaLabel::setBorderColour(SkColor colour) {
  borderColour_ = colour;
  markDirty();
}

void SkiaLabel::setBorderWidth(float width) {
  borderWidth_ = width;
  markDirty();
}

void SkiaLabel::setJustification(Justification justification) {
  if (justification_ != justification) {
    justification_ = justification;
    markDirty();
  }
}

void SkiaLabel::setMinimumHorizontalScale(float scale) {
  minimumHorizontalScale_ = scale;
  markDirty();
}

void SkiaLabel::setBorderSize(int left, int top, int right, int bottom) {
  borderSize_.left = left;
  borderSize_.top = top;
  borderSize_.right = right;
  borderSize_.bottom = bottom;
  markDirty();
}

void SkiaLabel::setEditable(bool editable) { editable_ = editable; }

void SkiaLabel::setEditableOnSingleClick(bool editOnSingleClick) {
  editableOnSingleClick_ = editOnSingleClick;
}

void SkiaLabel::setEditableOnDoubleClick(bool editOnDoubleClick) {
  editableOnDoubleClick_ = editOnDoubleClick;
}

void SkiaLabel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background if not transparent
  if (SkColorGetA(backgroundColour_) > 0) {
    SkPaint bgPaint;
    bgPaint.setColor(backgroundColour_);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                     bgPaint);
  }

  // Draw border
  if (borderWidth_ > 0.0f) {
    SkPaint borderPaint;
    borderPaint.setColor(borderColour_);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(borderWidth_);
    borderPaint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                     borderPaint);
  }

  // Draw text
  if (!text_.isEmpty()) {
    SkRect skBounds = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                       bounds.getWidth(), bounds.getHeight());
    drawText(canvas, skBounds);
  }
}

void SkiaLabel::resized() {
  // Layout logic can be added here
}

void SkiaLabel::mouseDown(const juce::MouseEvent &e) {
  if (editable_ && editableOnSingleClick_) {
    handleEditRequest();
  }
}

void SkiaLabel::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (editable_ && editableOnDoubleClick_) {
    handleEditRequest();
  }
}

void SkiaLabel::drawText(SkCanvas *canvas, const SkRect &componentBounds) {
  // Calculate text bounds with padding
  SkRect textBounds = getTextBounds(componentBounds);

  // Set up text paint
  SkPaint textPaint;
  textPaint.setColor(textColour_);
  textPaint.setAntiAlias(true);

  // Handle text scaling if needed
  float scale = 1.0f;
  float textWidth = font_.measureText(text_.toRawUTF8(), text_.length(),
                                      SkTextEncoding::kUTF8);

  if (minimumHorizontalScale_ > 0.0f && textWidth > textBounds.width()) {
    scale = textBounds.width() / textWidth;
    if (scale < minimumHorizontalScale_) {
      scale = minimumHorizontalScale_;
    }
  }

  // Save canvas state for scaling
  canvas->save();
  canvas->scale(scale, 1.0f);

  // Calculate position based on justification
  float x = textBounds.left() / scale;
  float y = textBounds.top() + font_.getSize(); // Simple baseline calculation

  // Adjust position based on justification
  switch (justification_) {
  case Justification::Left:
  case Justification::TopLeft:
  case Justification::BottomLeft:
    // x is already correct
    break;

  case Justification::Center:
  case Justification::TopCenter:
  case Justification::BottomCenter:
    x = textBounds.centerX() / scale - textWidth * 0.5f;
    break;

  case Justification::Right:
  case Justification::TopRight:
  case Justification::BottomRight:
    x = textBounds.right() / scale - textWidth;
    break;
  }

  // Adjust y based on justification
  switch (justification_) {
  case Justification::TopLeft:
  case Justification::TopCenter:
  case Justification::TopRight:
    // y is already at top
    break;

  case Justification::Left:
  case Justification::Center:
  case Justification::Right:
    y = textBounds.centerY() + font_.getSize() * 0.3f;
    break;

  case Justification::BottomLeft:
  case Justification::BottomCenter:
  case Justification::BottomRight:
    y = textBounds.bottom() - font_.getSize() * 0.2f;
    break;
  }

  // Draw the text
  canvas->drawString(text_.toRawUTF8(), x, y, font_, textPaint);

  canvas->restore();
}

SkRect SkiaLabel::getTextBounds(const SkRect &componentBounds) const {
  // Apply border size padding
  return SkRect::MakeXYWH(
      componentBounds.left() + borderSize_.left,
      componentBounds.top() + borderSize_.top,
      componentBounds.width() - borderSize_.left - borderSize_.right,
      componentBounds.height() - borderSize_.top - borderSize_.bottom);
}

void SkiaLabel::handleEditRequest() {
  // For now, just notify that editing was requested
  // In a full implementation, this would show a text editor overlay
  if (onTextChange) {
    onTextChange();
  }
}

} // namespace zenith