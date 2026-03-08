/*
  ==============================================================================

    BrowserFilterBar.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserFilterBar.h"
#include "../design-system/ZenithDesignSystem.h"

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

  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_02);
  canvas->drawRect(SkRect::MakeXYWH(0, y, w, h), bgPaint);

  auto activeFilter = model_.getActiveFilter();
  drawFilterTab(canvas, filterAllBounds_, "All", !model_.hasActiveFilter());
  drawFilterTab(canvas, filterInstrumentsBounds_, "Instruments",
                activeFilter == BrowserItemType::Instrument);
  drawFilterTab(canvas, filterSoundsBounds_, "Sounds",
                activeFilter == BrowserItemType::AudioFile);
  drawFilterTab(canvas, filterEffectsBounds_, "Effects",
                activeFilter == BrowserItemType::Plugin);
  drawFilterTab(canvas, filterMidiBounds_, "MIDI",
                activeFilter == BrowserItemType::MidiFile);
  drawFilterTab(canvas, filterPresetsBounds_, "Presets",
                activeFilter == BrowserItemType::Preset);
  drawFilterTab(canvas, filterProjectsBounds_, "Projects",
                activeFilter == BrowserItemType::Project);

  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  canvas->drawLine(0, y + h - 0.5f, w, y + h - 0.5f, borderPaint);
}

void BrowserFilterBar::drawFilterTab(SkCanvas *canvas,
                                 const juce::Rectangle<int> &bounds,
                                 const juce::String &label, bool active) {
  SkRect tabRect = SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                                    (float)bounds.getWidth(), (float)bounds.getHeight());

  if (active) {
    SkPaint tabPaint;
    tabPaint.setAntiAlias(true);
    tabPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.18f));
    canvas->drawRoundRect(tabRect, design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, tabPaint);

    SkPaint borderPaint;
    borderPaint.setColor(design::colors::ACCENT_PRIMARY);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(tabRect.makeInset(0.5f, 0.5f),
                          design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, borderPaint);

    SkPaint indicatorPaint;
    indicatorPaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRect(SkRect::MakeXYWH((float)bounds.getX() + 6,
                                      (float)bounds.getBottom() - 2,
                                      (float)bounds.getWidth() - 12, 2),
                     indicatorPaint);
  } else {
    SkPaint tabPaint;
    tabPaint.setColor(design::colors::BG_03);
    tabPaint.setAntiAlias(true);
    canvas->drawRoundRect(tabRect, design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, tabPaint);

    SkPaint borderPaint;
    borderPaint.setColor(design::colors::BORDER_SUBTLE);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(tabRect.makeInset(0.5f, 0.5f),
                          design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, borderPaint);
  }

  SkFont font = design::getSkFont(11.0f, design::FontWeight::Medium);
  SkPaint textPaint;
  textPaint.setColor(active ? design::colors::TEXT_PRIMARY
                            : design::colors::TEXT_SECONDARY);
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
  } else if (filterInstrumentsBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::Instrument);
    changed = true;
  } else if (filterSoundsBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::AudioFile);
    changed = true;
  } else if (filterEffectsBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::Plugin);
    changed = true;
  } else if (filterMidiBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::MidiFile);
    changed = true;
  } else if (filterPresetsBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::Preset);
    changed = true;
  } else if (filterProjectsBounds_.contains(e.getPosition())) {
    model_.setActiveFilter(BrowserItemType::Project);
    changed = true;
  }

  if (changed) {
    if (onFilterChanged) onFilterChanged();
    repaint();
  }
}

void BrowserFilterBar::resized() {
  auto bounds = getLocalBounds();
  int x = 8;
  int h = bounds.getHeight();
  const int tabGap = 6;
  const int tabH = h - 8;

  auto allocate = [&](const juce::String& label) {
    const int w = juce::jlimit(56, 132, 22 + label.length() * 8);
    juce::Rectangle<int> r(x, 4, w, tabH);
    x += w + tabGap;
    return r;
  };

  filterAllBounds_ = allocate("All");
  filterInstrumentsBounds_ = allocate("Instruments");
  filterSoundsBounds_ = allocate("Sounds");
  filterEffectsBounds_ = allocate("Effects");
  filterMidiBounds_ = allocate("MIDI");
  filterPresetsBounds_ = allocate("Presets");
  filterProjectsBounds_ = allocate("Projects");
}

} // namespace zenith
