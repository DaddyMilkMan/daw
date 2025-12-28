/*
  ==============================================================================

    BrowserFilterBar.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserFilterBar.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>

namespace zenith {

BrowserFilterBar::BrowserFilterBar(BrowserModel &model) : model_(model) {
}

BrowserFilterBar::~BrowserFilterBar() {
}

void BrowserFilterBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  float y = 0;
  float w = (float)bounds.getWidth();
  float h = (float)bounds.getHeight();

  // Premium gradient background
  SkPoint bgGradPoints[2] = {{0, y}, {0, y + h}};
  SkColor bgGradColors[2] = {
      design::unified::bg_02(),
      design::unified::bg_00()
  };
  auto bgGradient = SkGradientShader::MakeLinear(
      bgGradPoints, bgGradColors, nullptr, 2, SkTileMode::kClamp);
  SkPaint bgPaint;
  bgPaint.setShader(bgGradient);
  canvas->drawRect(SkRect::MakeXYWH(0, y, w, h), bgPaint);

  // Subtle top highlight
  SkPaint highlightPaint;
  highlightPaint.setColor(design::colors::GLASS_HIGHLIGHT);
  canvas->drawLine(0, y + 0.5f, w, y + 0.5f, highlightPaint);

  // Draw tabs
  auto activeFilter = model_.getActiveFilter();
  drawFilterTab(canvas, filterAllBounds_, "All", !model_.hasActiveFilter());
  drawFilterTab(canvas, filterAudioBounds_, "Audio",
                activeFilter == BrowserItemType::AudioFile);
  drawFilterTab(canvas, filterMidiBounds_, "MIDI",
                activeFilter == BrowserItemType::MidiFile);
  drawFilterTab(canvas, filterPluginBounds_, "Plugins",
                activeFilter == BrowserItemType::Plugin);

  // Bottom border with subtle accent
  SkPaint borderPaint;
  borderPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.2f));
  canvas->drawLine(0, y + h - 0.5f, w, y + h - 0.5f, borderPaint);
}

void BrowserFilterBar::drawFilterTab(SkCanvas *canvas,
                                 const juce::Rectangle<int> &bounds,
                                 const juce::String &label, bool active) {
  SkRect tabRect = SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                                    (float)bounds.getWidth(), (float)bounds.getHeight());

  if (active) {
    SkPaint glowPaint;
    glowPaint.setColor(design::withAlpha(design::colors::CYAN, 0.4f));
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
    glowPaint.setAntiAlias(true);
    canvas->drawRoundRect(tabRect, 6, 6, glowPaint);

    SkPoint tabGradPoints[2] = {{0, tabRect.fTop}, {0, tabRect.fBottom}};
    SkColor tabGradColors[2] = {
        design::withAlpha(design::colors::CYAN, 0.3f),
        design::withAlpha(design::colors::CYAN, 0.1f)
    };
    auto tabGradient = SkGradientShader::MakeLinear(
        tabGradPoints, tabGradColors, nullptr, 2, SkTileMode::kClamp);
    SkPaint tabPaint;
    tabPaint.setShader(tabGradient);
    tabPaint.setAntiAlias(true);
    canvas->drawRoundRect(tabRect, 5, 5, tabPaint);

    SkPaint borderPaint;
    borderPaint.setColor(design::colors::CYAN);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(tabRect.makeInset(0.5f, 0.5f), 4.5f, 4.5f, borderPaint);

    // Indicator line
    SkPaint indicatorPaint;
    indicatorPaint.setColor(design::colors::CYAN);
    canvas->drawRoundRect(SkRect::MakeXYWH((float)bounds.getX() + 4,
                                           (float)bounds.getBottom() - 2,
                                           (float)bounds.getWidth() - 8, 2),
                          1, 1, indicatorPaint);
  } else {
    SkPaint tabPaint;
    tabPaint.setColor(design::colors::GLASS_HIGHLIGHT);
    tabPaint.setAntiAlias(true);
    canvas->drawRoundRect(tabRect, 4, 4, tabPaint);
  }

  SkFont font = design::getSkFont(11.0f, design::FontWeight::Medium);
  SkPaint textPaint;
  textPaint.setColor(active ? design::colors::ACCENT_PRIMARY
                            : design::withAlpha(design::colors::TEXT_PRIMARY, 0.7f));
  textPaint.setAntiAlias(true);

  SkRect textBounds;
  font.measureText(label.toStdString().c_str(), label.length(), SkTextEncoding::kUTF8, &textBounds);
  float textX = (float)bounds.getCentreX() - textBounds.width() / 2;
  float textY = (float)bounds.getCentreY() + 4;

  canvas->drawString(label.toStdString().c_str(), textX, textY, font, textPaint);
}

void BrowserFilterBar::mouseDown(const juce::MouseEvent &e) {
  bool changed = false;
  if (filterAllBounds_.contains(e.getPosition())) {
    model_.clearFilter();
    changed = true;
  } else if (filterAudioBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::AudioFile);
    changed = true;
  } else if (filterMidiBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::MidiFile);
    changed = true;
  } else if (filterPluginBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::Plugin);
    changed = true;
  }

  if (changed) {
    if (onFilterChanged) onFilterChanged();
    repaint();
  }
}

void BrowserFilterBar::resized() {
  auto bounds = getLocalBounds();
  int tabWidth = (bounds.getWidth() - 16) / 4;
  int x = 8;
  int h = bounds.getHeight();

  filterAllBounds_ = juce::Rectangle<int>(x, 4, tabWidth - 4, h - 8);
  x += tabWidth;
  filterAudioBounds_ = juce::Rectangle<int>(x, 4, tabWidth - 4, h - 8);
  x += tabWidth;
  filterMidiBounds_ = juce::Rectangle<int>(x, 4, tabWidth - 4, h - 8);
  x += tabWidth;
  filterPluginBounds_ = juce::Rectangle<int>(x, 4, tabWidth - 4, h - 8);
}

} // namespace zenith
