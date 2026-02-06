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

#include "BrowserListView.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../framework/GlassmorphicPanel.h"
#include "../framework/NeonGlow.h"
#include "../../browser/BrowserDragSource.h"
#include <algorithm>
#include <effects/SkGradientShader.h>

namespace zenith {

BrowserListView::BrowserListView(BrowserModel &model, BrowserWaveformLoader &loader)
    : model_(model), waveformLoader_(loader) {
  currentRoot_ = model_.getRoot();
  updateDisplayItems();
  
  hoverPreview_ = std::make_unique<BrowserHoverPreview>();
  addChildComponent(hoverPreview_.get());
}

BrowserListView::~BrowserListView() {
  stopTimer();
}

void BrowserListView::updateDisplayItems() {
  displayItems_.clear();
  if (searchText_.isNotEmpty()) {
    displayItems_ = model_.search(searchText_);
  } else if (currentRoot_) {
    displayItems_ = currentRoot_->children;
  }

  std::sort(displayItems_.begin(), displayItems_.end(),
            [](const std::shared_ptr<BrowserItem> &a, const std::shared_ptr<BrowserItem> &b) {
              if (a->isDirectory != b->isDirectory) return a->isDirectory > b->isDirectory;
              return a->name.compareNatural(b->name) < 0;
            });
}

void BrowserListView::setSearchText(const juce::String &text) {
  searchText_ = text;
  scrollOffset_ = 0;
  selectedIndex_ = -1;
  updateDisplayItems();
  repaint();
}

void BrowserListView::navigateTo(std::shared_ptr<BrowserItem> folder) {
  if (folder && folder->isDirectory) {
    currentRoot_ = folder;
    searchText_ = "";
    scrollOffset_ = 0;
    selectedIndex_ = -1;
    updateDisplayItems();
    repaint();
  }
}

void BrowserListView::navigateUp() {
  if (currentRoot_) {
    auto parent = currentRoot_->parent.lock();
    if (parent) {
      currentRoot_ = parent;
    } else {
      auto root = model_.getRoot();
      if (currentRoot_ != root) currentRoot_ = root;
    }

    searchText_ = "";
    scrollOffset_ = 0;
    selectedIndex_ = -1;
    updateDisplayItems();
    repaint();
  }
}

std::shared_ptr<BrowserItem> BrowserListView::getSelectedItem() const {
  if (selectedIndex_ >= 0 && selectedIndex_ < (int)displayItems_.size())
    return displayItems_[selectedIndex_];
  return nullptr;
}

void BrowserListView::timerCallback() {
  stopTimer();
  if (hoverIndex_ >= 0 && hoverIndex_ < (int)displayItems_.size()) {
    triggerHoverPreview(hoverIndex_);
  }
}

void BrowserListView::triggerHoverPreview(int index) {
  auto item = displayItems_[index];
  hoverPreviewIndex_ = index;
  
  int itemY = (index - scrollOffset_) * itemHeight_;
  juce::Point<int> screenPos = getScreenPosition() + juce::Point<int>(0, itemY + itemHeight_ / 2);
  
  hoverPreview_->showForItem(item, screenPos);
  
  if (item->type == BrowserItemType::AudioFile) {
    if (waveformLoader_.isCached(item->id)) {
      hoverPreview_->setWaveformData(waveformLoader_.getCached(item->id));
    } else {
      waveformLoader_.request(item->id, [this, index](const juce::String& path, const std::vector<float>& peaks) {
        if (hoverPreviewIndex_ == index) {
          hoverPreview_->setWaveformData(peaks);
        }
      });
    }
  }
}

void BrowserListView::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  canvas->save();
  canvas->clipRect(SkRect::MakeXYWH(0, 0, (float)bounds.getWidth(), (float)bounds.getHeight()));

  int maxVisible = bounds.getHeight() / itemHeight_;
  int visibleStart = scrollOffset_;
  int visibleEnd = std::min(visibleStart + maxVisible + 1, (int)displayItems_.size());

  for (int i = visibleStart; i < visibleEnd; ++i) {
    int itemY = (i - scrollOffset_) * itemHeight_;
    auto itemBounds = juce::Rectangle<int>(0, itemY, bounds.getWidth(), itemHeight_);
    drawBrowserItem(canvas, i, itemBounds);
  }

  if (displayItems_.size() > maxVisible) {
    float ratio = (float)maxVisible / (float)displayItems_.size();
    float scrollbarHeight = ratio * bounds.getHeight();
    float scrollbarY = ((float)scrollOffset_ / (float)displayItems_.size()) * bounds.getHeight();

    SkPaint scrollPaint;
    scrollPaint.setColor(design::withAlpha(design::colors::TEXT_PRIMARY, 0.24f));
    canvas->drawRoundRect(SkRect::MakeXYWH((float)bounds.getWidth() - 6, scrollbarY, 4, scrollbarHeight), 2, 2, scrollPaint);
  }

  canvas->restore();
}

void BrowserListView::drawBrowserItem(SkCanvas *canvas, int index, const juce::Rectangle<int> &bounds) {
  auto item = displayItems_[index];
  float x = (float)bounds.getX();
  float y = (float)bounds.getY();
  float w = (float)bounds.getWidth();
  float h = (float)bounds.getHeight();

  if (index == selectedIndex_) {
    GlassmorphicPanel::draw(canvas, SkRect::MakeXYWH(x, y, w, h), GlassmorphicPanel::Style::ActiveGlow);
    SkRect barRect = SkRect::MakeXYWH(x, y + 2, 3, h - 4);
    SkPaint barPaint;
    barPaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRect(barRect, barPaint);
    NeonGlow::drawGlow(canvas, barRect, barPaint.getColor(), NeonGlow::Intensity::Medium);
  } else if (index == hoverIndex_) {
    GlassmorphicPanel::draw(canvas, SkRect::MakeXYWH(x + 4, y + 1, w - 8, h - 2), GlassmorphicPanel::Style::Subtle);
  }

  if (item->type == BrowserItemType::AudioFile) {
    if (waveformLoader_.isCached(item->id)) {
      auto peaks = waveformLoader_.getCached(item->id);
      if (!peaks.empty()) {
        SkPaint wavePaint;
        wavePaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.15f));
        float midY = y + h * 0.5f;
        float waveH = h * 0.7f;
        float startX = x + 35;
        float availableW = w - 120 - 35;
        if (availableW > 50) {
          float step = availableW / (float)peaks.size();
          for (size_t i = 0; i < peaks.size(); ++i) {
            float barH = std::max(1.0f, peaks[i] * waveH);
            canvas->drawRect(SkRect::MakeXYWH(startX + i * step, midY - barH * 0.5f, std::max(1.0f, step - 0.5f), barH), wavePaint);
          }
        }
      }
    } else {
      waveformLoader_.request(item->id, [this](const juce::String&, const std::vector<float>&) { repaint(); });
    }
  }

  drawIcon(canvas, item->type, x + 18, (float)bounds.getCentreY(), 14);

  SkFont font = zenith::design::getSkFont(zenith::design::typography::FONT_MD);
  SkPaint textPaint;
  textPaint.setColor((index == selectedIndex_) ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
  textPaint.setAntiAlias(true);
  
  float nameX = x + 38;
  float nameY = (float)bounds.getCentreY() + 4;
  canvas->drawString(item->name.toStdString().c_str(), nameX, nameY, font, textPaint);

  // Star Ratings
  float nameWidth = font.measureText(item->name.toStdString().c_str(), item->name.length(), SkTextEncoding::kUTF8);
  float starX = nameX + nameWidth + 12;
  int rating = model_.getItemRating(item->id);
  
  for (int i = 0; i < 5; ++i) {
    SkPaint starPaint;
    starPaint.setAntiAlias(true);
    bool filled = (i < rating);
    starPaint.setColor(filled ? design::colors::AMBER : design::withAlpha(design::colors::TEXT_PRIMARY, 0.15f));
    
    // Simple star shape
    SkPath star;
    float cx = starX + i * 14;
    float cy = (float)bounds.getCentreY();
    for (int j = 0; j < 5; ++j) {
        float angle = j * 2.51327f - 1.5708f;
        float r1 = 5.0f; float r2 = 2.0f;
        if (j == 0) star.moveTo(cx + cos(angle)*r1, cy + sin(angle)*r1);
        else star.lineTo(cx + cos(angle)*r1, cy + sin(angle)*r1);
        angle += 0.6283f;
        star.lineTo(cx + cos(angle)*r2, cy + sin(angle)*r2);
    }
    star.close();
    canvas->drawPath(star, starPaint);
    if (filled) NeonGlow::drawGlow(canvas, star.getBounds(), starPaint.getColor(), NeonGlow::Intensity::Subtle);
  }

  if (!item->metadata.tags.empty()) {
    drawTags(canvas, item->metadata.tags, x + w - 10, (float)bounds.getCentreY());
  } else if (item->isDirectory) {
    // ... chevron drawing ...
    float arrowX = w - 20;
    float arrowY = (float)bounds.getCentreY();
    SkPath chevron;
    chevron.moveTo(arrowX - 2, arrowY - 5);
    chevron.lineTo(arrowX + 4, arrowY);
    chevron.lineTo(arrowX - 2, arrowY + 5);
    SkPaint cp;
    cp.setColor(index == hoverIndex_ ? design::withAlpha(design::colors::TEXT_PRIMARY, 0.7f) : design::withAlpha(design::colors::TEXT_PRIMARY, 0.3f));
    cp.setStyle(SkPaint::kStroke_Style);
    cp.setStrokeWidth(1.8f);
    cp.setStrokeCap(SkPaint::kRound_Cap);
    cp.setAntiAlias(true);
    canvas->drawPath(chevron, cp);
  }
}

void BrowserListView::drawIcon(SkCanvas *canvas, BrowserItemType type, float x, float y, float size) {
  SkPaint p; p.setAntiAlias(true);
  switch (type) {
    case BrowserItemType::Folder:
      p.setColor(design::colors::AMBER);
      { SkPath f; f.moveTo(x-6, y-4); f.lineTo(x-2, y-4); f.lineTo(x, y-6); f.lineTo(x+6, y-6); f.lineTo(x+6, y+4); f.lineTo(x-6, y+4); f.close(); canvas->drawPath(f, p); }
      break;
    case BrowserItemType::AudioFile:
      p.setColor(design::colors::SUCCESS);
      canvas->drawRect(SkRect::MakeXYWH(x-6, y-2, 3, 4), p);
      canvas->drawRect(SkRect::MakeXYWH(x-2, y-5, 3, 10), p);
      canvas->drawRect(SkRect::MakeXYWH(x+2, y-3, 3, 6), p);
      break;
    default:
      p.setColor(SK_ColorGRAY); canvas->drawCircle(x, y, size*0.3f, p); break;
  }
}

void BrowserListView::drawTags(SkCanvas *canvas, const std::vector<juce::String> &tags, float rightBound, float centerY) {
  SkFont font = design::getSkFont(10.0f, design::FontWeight::Bold);
  float x = rightBound;
  for (int i = (int)tags.size() - 1; i >= 0; --i) {
    const auto &tag = tags[i];
    SkRect b; font.measureText(tag.toStdString().c_str(), tag.length(), SkTextEncoding::kUTF8, &b);
    float width = b.width() + 12.0f;
    x -= width;
    if (x < 150) break; // Don't crowd the name/stars too much
    
    juce::String hexCol = model_.getTagColor(tag);
    SkColor tagCol = hexCol.isNotEmpty() ? design::toSkColor(juce::Colour::fromString(hexCol)) : design::withAlpha(design::colors::ACCENT_PRIMARY, 0.4f);
    
    SkPaint bp; bp.setAntiAlias(true);
    bp.setColor(design::withAlpha(tagCol, 0.25f)); // Subtle background
    canvas->drawRoundRect(SkRect::MakeXYWH(x, centerY-8, width, 16), 8, 8, bp);
    
    SkPaint tp; 
    tp.setColor(design::withAlpha(tagCol, 0.9f)); // Brighter text
    tp.setAntiAlias(true);
    canvas->drawString(tag.toStdString().c_str(), x+6, centerY+3.5f, font, tp);
    
    x -= 6;
  }
}

int BrowserListView::getItemIndexAt(int y) const {
  return (y / itemHeight_) + scrollOffset_;
}

void BrowserListView::mouseDown(const juce::MouseEvent &e) {
  hoverPreview_->hide();
  stopTimer();
  
  int clickedIndex = getItemIndexAt(e.y);
  if (clickedIndex >= 0 && clickedIndex < (int)displayItems_.size()) {
    selectedIndex_ = clickedIndex;
    auto item = displayItems_[clickedIndex];

    // Star hit testing (right side of name)
    int itemY = (clickedIndex - scrollOffset_) * itemHeight_;
    float starAreaX = 38.0f + design::getSkFont(design::typography::FONT_MD).measureText(item->name.toStdString().c_str(), item->name.length(), SkTextEncoding::kUTF8) + 10.0f;
    
    if (e.x >= starAreaX && e.x <= starAreaX + 80) {
      int rating = (int)((e.x - starAreaX) / 16.0f) + 1;
      model_.setItemRating(item, rating);
      repaint();
      return;
    }

    if (e.mods.isRightButtonDown()) {
      if (onItemRightClicked) onItemRightClicked(clickedIndex, e.getScreenPosition());
    } else {
      if (onItemSelected) onItemSelected(displayItems_[clickedIndex]);
    }
    repaint();
  }
}

void BrowserListView::mouseMove(const juce::MouseEvent &e) {
  int newHover = getItemIndexAt(e.y);
  if (newHover >= 0 && newHover < (int)displayItems_.size()) {
    if (newHover != hoverIndex_) {
      hoverIndex_ = newHover;
      hoverPreviewIndex_ = -1;
      hoverPreview_->hide();
      if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(hoverDelayMs_);
      repaint();
    }
  } else if (hoverIndex_ != -1) {
    hoverIndex_ = -1;
    hoverPreviewIndex_ = -1;
    hoverPreview_->hide();
    stopTimer();
    repaint();
  }
}

void BrowserListView::mouseExit(const juce::MouseEvent &e) {
  hoverIndex_ = -1;
  hoverPreviewIndex_ = -1;
  hoverPreview_->hide();
  stopTimer();
  repaint();
}

void BrowserListView::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
  hoverPreview_->hide();
  stopTimer();
  if (displayItems_.empty()) return;
  int delta = (wheel.deltaY > 0) ? -3 : 3;
  int maxScroll = std::max(0, (int)displayItems_.size() - (getHeight() / itemHeight_));
  scrollOffset_ = juce::jlimit(0, maxScroll, scrollOffset_ + delta);
  repaint();
}

void BrowserListView::mouseDoubleClick(const juce::MouseEvent &e) {
  int idx = getItemIndexAt(e.y);
  if (idx >= 0 && idx < (int)displayItems_.size()) {
    auto item = displayItems_[idx];
    if (item->isDirectory) navigateTo(item);
    else if (onItemDoubleClicked) onItemDoubleClicked(item);
  }
}

void BrowserListView::mouseDrag(const juce::MouseEvent &e) {
  if (e.getDistanceFromDragStart() > 5 && selectedIndex_ >= 0) {
    auto item = displayItems_[selectedIndex_];
    if (!item->isDirectory) BrowserDragSource::startDrag(this, item, juce::Image());
  }
}

bool BrowserListView::keyPressed(const juce::KeyPress &key) {
  if (key.isKeyCode(juce::KeyPress::upKey)) {
    if (selectedIndex_ > 0) { selectedIndex_--; if (selectedIndex_ < scrollOffset_) scrollOffset_ = selectedIndex_; repaint(); }
    return true;
  } else if (key.isKeyCode(juce::KeyPress::downKey)) {
    if (selectedIndex_ < (int)displayItems_.size() - 1) { selectedIndex_++; int max = getHeight() / itemHeight_; if (selectedIndex_ >= scrollOffset_ + max) scrollOffset_ = selectedIndex_ - max + 1; repaint(); }
    return true;
  } else if (key.isKeyCode(juce::KeyPress::returnKey) && selectedIndex_ >= 0) {
    auto item = displayItems_[selectedIndex_];
    if (item->isDirectory) navigateTo(item);
    return true;
  }
  return false;
}

void BrowserListView::resized() {}

} // namespace zenith
