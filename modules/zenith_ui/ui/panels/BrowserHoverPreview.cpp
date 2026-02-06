/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "BrowserHoverPreview.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include "../framework/GlassmorphicPanel.h"
#include <effects/SkGradientShader.h>

namespace zenith {

BrowserHoverPreview::BrowserHoverPreview() {
  setInterceptsMouseClicks(false, false);
  setAlwaysOnTop(true);
}

BrowserHoverPreview::~BrowserHoverPreview() {
}

void BrowserHoverPreview::showForItem(std::shared_ptr<BrowserItem> item, juce::Point<int> pos) {
  if (currentItem_ != item) {
    currentItem_ = item;
    waveformPeaks_.clear();
  }

  // Calculate position (try to the right and center-aligned)
  int x = pos.x + 20;
  int y = pos.y - previewHeight_ / 2;
  
  // Basic screen boundary check
  auto screen = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
  if (x + previewWidth_ > screen.getRight()) x = pos.x - previewWidth_ - 20;
  if (y < screen.getY()) y = screen.getY() + 10;
  if (y + previewHeight_ > screen.getBottom()) y = screen.getBottom() - previewHeight_ - 10;

  setBounds(x, y, previewWidth_, previewHeight_);
  setVisible(true);
  repaint();
}

void BrowserHoverPreview::hide() {
  setVisible(false);
  currentItem_ = nullptr;
}

void BrowserHoverPreview::setWaveformData(const std::vector<float> &peaks) {
  waveformPeaks_ = peaks;
  repaint();
}

void BrowserHoverPreview::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  float w = (float)bounds.getWidth();
  float h = (float)bounds.getHeight();

  // Glow shadow
  SkPaint shadow;
  shadow.setColor(design::withAlpha(design::colors::BG_DARKEST, 0.4f));
  shadow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 10.0f));
  canvas->drawRoundRect(SkRect::MakeWH(w, h), 10, 10, shadow);

  // Background
  GlassmorphicPanel::fillBackground(canvas, SkRect::MakeWH(w, h));
  
  // Border accent
  SkPaint border;
  border.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.3f));
  border.setStyle(SkPaint::kStroke_Style);
  border.setStrokeWidth(1.5f);
  canvas->drawRoundRect(SkRect::MakeWH(w, h).makeInset(0.75f, 0.75f), 9.25f, 9.25f, border);

  if (!currentItem_) return;

  // Header: Name
  SkFont titleFont = design::getSkFont(14.0f, design::FontWeight::Bold);
  SkPaint titlePaint;
  titlePaint.setColor(design::colors::TEXT_PRIMARY);
  titlePaint.setAntiAlias(true);
  canvas->drawString(currentItem_->name.toStdString().c_str(), 12, 24, titleFont, titlePaint);

  // Metadata
  SkFont metaFont = design::getSkFont(10.0f, design::FontWeight::Regular);
  SkPaint metaPaint;
  metaPaint.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.7f));
  
  juce::String metaStr = BrowserItem::getEnumName(currentItem_->type);
  if (currentItem_->metadata.duration > 0) {
    int s = (int)currentItem_->metadata.duration;
    metaStr += " | " + juce::String::formatted("%02d:%02d", s/60, s%60);
  }
  if (currentItem_->metadata.sampleRate > 0) {
    metaStr += " | " + juce::String(currentItem_->metadata.sampleRate / 1000.0, 1) + "kHz";
  }
  canvas->drawString(metaStr.toStdString().c_str(), 12, 40, metaFont, metaPaint);

  // Waveform
  if (!waveformPeaks_.empty()) {
    SkRect waveRect = SkRect::MakeXYWH(12, 50, w - 24, h - 62);
    SkPaint bg; bg.setColor(design::withAlpha(design::colors::BG_DARKEST, 0.15f));
    canvas->drawRoundRect(waveRect, 4, 4, bg);

    SkPath path;
    float cy = waveRect.centerY();
    float mh = waveRect.height() * 0.45f;
    float pw = waveRect.width() / waveformPeaks_.size();
    
    for (size_t i = 0; i < waveformPeaks_.size(); ++i) {
        float x = waveRect.x() + i * pw;
        float height = waveformPeaks_[i] * mh;
        if (i == 0) path.moveTo(x, cy - height); else path.lineTo(x, cy - height);
    }
    for (int i = (int)waveformPeaks_.size() - 1; i >= 0; --i) {
        float x = waveRect.x() + i * pw;
        float height = waveformPeaks_[i] * mh;
        path.lineTo(x, cy + height);
    }
    path.close();

    SkPaint wp;
    wp.setColor(design::colors::ACCENT_PRIMARY);
    wp.setAntiAlias(true);
    canvas->drawPath(path, wp);
  } else if (currentItem_->type == BrowserItemType::AudioFile) {
    SkFont f = design::getSkFont(10.0f, design::FontWeight::Regular);
    SkPaint p; p.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.4f));
    canvas->drawString("Loading preview...", w/2 - 40, h/2 + 10, f, p);
  }
}

} // namespace zenith
