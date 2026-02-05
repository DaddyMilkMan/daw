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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    BrowserRecentSidebar.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/


#include "BrowserRecentSidebar.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include "../framework/GlassmorphicPanel.h"

namespace zenith {

BrowserRecentSidebar::BrowserRecentSidebar(BrowserModel &model) : model_(model) {
}

BrowserRecentSidebar::~BrowserRecentSidebar() {
}

void BrowserRecentSidebar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  float w = (float)bounds.getWidth();
  float h = (float)bounds.getHeight();

  // Sidebar background
  SkPaint bg;
  bg.setColor(design::withAlpha(design::colors::BG_DARKEST, 0.45f));
  canvas->drawRect(SkRect::MakeWH(w, h), bg);

  // Header
  SkFont headerFont = design::getSkFont(11.0f, design::FontWeight::Bold);
  SkPaint headerPaint;
  headerPaint.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.6f));
  canvas->drawString("RECENT", 10, 20, headerFont, headerPaint);

  auto recent = model_.getRecentItems();
  int y = 35;
  for (size_t i = 0; i < recent.size() && y < bounds.getHeight(); ++i) {
    auto item = recent[i];
    
    if ((int)i == hoverIndex_) {
      SkPaint hp;
      hp.setColor(design::withAlpha(SK_ColorWHITE, 0.08f));
      canvas->drawRect(SkRect::MakeXYWH(2, (float)y, w - 4, (float)itemHeight_ - 2), hp);
    }

    SkFont font = design::getSkFont(11.0f, design::FontWeight::Regular);
    SkPaint tp;
    tp.setColor(design::colors::TEXT_PRIMARY);
    tp.setAntiAlias(true);
    
    // Draw icon (simplified)
    SkPaint ip; ip.setColor(design::colors::ACCENT_PRIMARY); ip.setAntiAlias(true);
    canvas->drawCircle(20, (float)y + itemHeight_ / 2.0f, 3, ip);

    // Draw name with truncation
    std::string name = item->name.toStdString();
    if (name.length() > 20) name = name.substr(0, 17) + "...";
    canvas->drawString(name.c_str(), 35, (float)y + itemHeight_ / 2.0f + 4, font, tp);

    y += itemHeight_;
  }
}

void BrowserRecentSidebar::mouseDown(const juce::MouseEvent &e) {
  int idx = (e.y - 35) / itemHeight_;
  auto recent = model_.getRecentItems();
  if (idx >= 0 && idx < (int)recent.size()) {
    if (onItemSelected) onItemSelected(recent[idx]);
  }
}

void BrowserRecentSidebar::mouseMove(const juce::MouseEvent &e) {
  int newHover = (e.y - 35) / itemHeight_;
  auto recent = model_.getRecentItems();
  if (newHover >= 0 && newHover < (int)recent.size()) {
    if (newHover != hoverIndex_) {
      hoverIndex_ = newHover;
      repaint();
    }
  } else if (hoverIndex_ != -1) {
    hoverIndex_ = -1;
    repaint();
  }
}

void BrowserRecentSidebar::mouseExit(const juce::MouseEvent &e) {
  hoverIndex_ = -1;
  repaint();
}

} // namespace zenith
