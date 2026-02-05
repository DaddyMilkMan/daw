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

    ProjectRecoveryModal.cpp
    Created: 2025-12-29
    Author:  Zenith DAW Team

    Premium Skia-based Project Recovery Modal implementation.

  ==============================================================================

*/

#include "ProjectRecoveryModal.h"
#include <core/SkTextBlob.h>
#include <core/SkBlurTypes.h>
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>
#include <cmath>

namespace zenith {

ProjectRecoveryModal::ProjectRecoveryModal(
    const std::vector<RecoveryInfo> &recoveries, RecoverCallback onRecover,
    DismissCallback onDiscard)
    : onRecover_(std::move(onRecover)), onDiscard_(std::move(onDiscard)) {

  setName("ProjectRecoveryModal");
  setWantsKeyboardFocus(true);

  // Initialize fonts using design system
  titleFont_ = design::typography::getSkFont(22.0f, design::FontWeight::Bold);
  subtitleFont_ = design::typography::getSkFont(14.0f, design::FontWeight::Regular);
  itemNameFont_ = design::typography::getSkFont(15.0f, design::FontWeight::SemiBold);
  itemDetailFont_ = design::typography::getSkFont(12.0f, design::FontWeight::Regular);
  buttonFont_ = design::typography::getSkFont(14.0f, design::FontWeight::SemiBold);

  // Build recovery items from the provided list
  for (const auto &info : recoveries) {
    RecoveryItem item;
    item.info = info;

    // Extract display name from file
    if (info.originalFile.existsAsFile()) {
      item.displayName = info.originalFile.getFileNameWithoutExtension();
    } else {
      item.displayName = info.recoveryFile.getFileNameWithoutExtension();
      if (item.displayName.startsWith("recovery_")) {
        item.displayName = "Untitled Project";
      }
    }

    item.timeAgo = formatTimeAgo(info.recoveryTimestamp);
    item.fileSize = formatFileSize(info.recoveryFile.getSize());
    item.scaleSpring = PhysicsSpring(1.0f);

    items_.push_back(std::move(item));
  }

  // Auto-select the most recent recovery (last in list)
  if (!items_.empty()) {
    selectedIndex_ = static_cast<int>(items_.size()) - 1;
    items_[selectedIndex_].isSelected = true;
  }

  // Start animation timer
  startTimerHz(60);
}

ProjectRecoveryModal::~ProjectRecoveryModal() { stopTimer(); }

void ProjectRecoveryModal::show() {
  isVisible_ = true;
  alpha_.setTarget(1.0f, 200);
  setVisible(true);
  grabKeyboardFocus();
}

void ProjectRecoveryModal::dismiss() {
  isVisible_ = false;
  alpha_.setTarget(0.0f, 150);
  // Modal will be hidden/reset in timerCallback when alpha is small
}

void ProjectRecoveryModal::timerCallback() {
  // Update animations
  animationTime_ += 0.016f;
  if (animationTime_ > 1000.0f)
    animationTime_ = 0.0f;

  // Rotate gradient border angle
  gradientAngle_ += 0.02f;
  if (gradientAngle_ > 6.28318f)
    gradientAngle_ -= 6.28318f;

  // Pulse icon glow
  iconPulse_ = 0.5f + 0.5f * std::sin(animationTime_ * 3.0f);

  // Update physics springs for items
  for (auto &item : items_) {
    float target = item.isHovered ? 1.02f : 1.0f;
    item.scaleSpring.setTarget(target);
    item.scaleSpring.update(0.016f);
  }

  // Check if we should hide after fade-out
  if (!isVisible_ && alpha_.get() < 0.01f) {
    setVisible(false);
    return;
  }

  markDirty();
}

void ProjectRecoveryModal::resized() { updateLayout(); }

void ProjectRecoveryModal::updateLayout() {
  auto bounds = getLocalBounds().toFloat();

  // Center the card
  float cardX = (bounds.getWidth() - CARD_WIDTH) / 2.0f;
  float cardY = (bounds.getHeight() - CARD_HEIGHT) / 2.0f;
  cardBounds_ = SkRect::MakeXYWH(cardX, cardY, CARD_WIDTH, CARD_HEIGHT);

  // Layout within card
  float innerLeft = cardBounds_.left() + PADDING;
  float innerRight = cardBounds_.right() - PADDING;
  float innerWidth = innerRight - innerLeft;
  float y = cardBounds_.top() + PADDING;

  // Close button (top-right corner)
  closeButtonBounds_ =
      SkRect::MakeXYWH(cardBounds_.right() - 40, cardBounds_.top() + 12, 28, 28);

  // Title area (with icon space)
  titleBounds_ = SkRect::MakeXYWH(innerLeft + 48, y, innerWidth - 48, 28);
  y += 32;

  // Subtitle
  subtitleBounds_ = SkRect::MakeXYWH(innerLeft, y, innerWidth, 20);
  y += 32;

  // Recovery list
  float listHeight = std::min(static_cast<float>(items_.size()) * ITEM_HEIGHT,
                              CARD_HEIGHT - 180.0f);
  listBounds_ = SkRect::MakeXYWH(innerLeft, y, innerWidth, listHeight);

  // Update item bounds within list
  float itemY = listBounds_.top();
  for (auto &item : items_) {
    item.bounds = SkRect::MakeXYWH(listBounds_.left(), itemY,
                                   listBounds_.width(), ITEM_HEIGHT - 4);
    itemY += ITEM_HEIGHT;
  }

  // Buttons at bottom
  float buttonY = cardBounds_.bottom() - PADDING - BUTTON_HEIGHT;
  float buttonWidth = (innerWidth - 12) / 2.0f;

  discardButtonBounds_ =
      SkRect::MakeXYWH(innerLeft, buttonY, buttonWidth, BUTTON_HEIGHT);
  recoverButtonBounds_ = SkRect::MakeXYWH(innerLeft + buttonWidth + 12, buttonY,
                                          buttonWidth, BUTTON_HEIGHT);
}

void ProjectRecoveryModal::drawSkia(SkCanvas *canvas) {
  if (!canvas)
    return;

  float alpha = alpha_.get();
  if (alpha < 0.01f)
    return;

  canvas->save();

  // Apply fade
  if (alpha < 1.0f) {
    SkPaint alphaPaint;
    alphaPaint.setAlphaf(alpha);
    canvas->saveLayerAlphaf(nullptr, alpha);
  }

  drawBackground(canvas);
  drawCard(canvas);
  drawAnimatedBorder(canvas);
  drawRecoveryIcon(canvas);
  drawTitle(canvas);
  drawRecoveryList(canvas);
  drawButtons(canvas);
  drawCloseButton(canvas);

  if (alpha < 1.0f) {
    canvas->restore();
  }

  canvas->restore();
}

void ProjectRecoveryModal::drawBackground(SkCanvas *canvas) {
  // Semi-transparent dark overlay behind modal
  SkPaint overlayPaint;
  overlayPaint.setColor(SkColorSetARGB(180, 0, 0, 0));
  canvas->drawRect(
      SkRect::MakeWH(static_cast<float>(getWidth()), static_cast<float>(getHeight())),
      overlayPaint);
}

void ProjectRecoveryModal::drawCard(SkCanvas *canvas) {
  // Use GlassmorphicPanel for the card
  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Floating;
  opts.cornerRadius = design::dimensions::RADIUS_LG;
  opts.drawShadow = true;
  opts.useBackdropBlur = true;

  GlassmorphicPanel::drawWithOptions(canvas, cardBounds_, opts);
}

void ProjectRecoveryModal::drawAnimatedBorder(SkCanvas *canvas) {
  using namespace design;

  float radius = dimensions::RADIUS_LG;
  SkRRect cardRRect = SkRRect::MakeRectXY(cardBounds_, radius, radius);

  // Animated gradient border (cyan ↔ magenta)
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(2.0f);

  // Create rotating gradient
  float cx = cardBounds_.centerX();
  float cy = cardBounds_.centerY();
  float gradRadius = std::max(cardBounds_.width(), cardBounds_.height()) * 0.7f;

  SkPoint pts[2] = {
      {cx + gradRadius * std::cos(gradientAngle_),
       cy + gradRadius * std::sin(gradientAngle_)},
      {cx + gradRadius * std::cos(gradientAngle_ + 3.14159f),
       cy + gradRadius * std::sin(gradientAngle_ + 3.14159f)}};

  SkColor gradColors[3] = {colors::CYAN, colors::MAGENTA, colors::CYAN};
  float positions[3] = {0.0f, 0.5f, 1.0f};

  borderPaint.setShader(SkGradientShader::MakeLinear(
      pts, gradColors, positions, 3, SkTileMode::kClamp));

  // Add glow effect
  borderPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 3.0f));

  SkRRect borderRRect = cardRRect;
  borderRRect.inset(1.0f, 1.0f);
  canvas->drawRRect(borderRRect, borderPaint);

  // Solid border on top for crispness
  SkPaint solidBorder;
  solidBorder.setAntiAlias(true);
  solidBorder.setStyle(SkPaint::kStroke_Style);
  solidBorder.setStrokeWidth(1.5f);
  solidBorder.setShader(SkGradientShader::MakeLinear(
      pts, gradColors, positions, 3, SkTileMode::kClamp));
  canvas->drawRRect(borderRRect, solidBorder);
}

void ProjectRecoveryModal::drawRecoveryIcon(SkCanvas *canvas) {
  using namespace design;

  // Icon position (left of title)
  float iconX = cardBounds_.left() + PADDING;
  float iconY = cardBounds_.top() + PADDING;
  float iconSize = 36.0f;

  // Glow behind icon
  SkPaint glowPaint;
  glowPaint.setAntiAlias(true);
  glowPaint.setColor(withAlpha(colors::CYAN, 0.3f * iconPulse_));
  glowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 8.0f));
  canvas->drawCircle(iconX + iconSize / 2, iconY + iconSize / 2, iconSize / 2,
                     glowPaint);

  // Draw folder icon with clock overlay
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);
  iconPaint.setColor(colors::CYAN);
  iconPaint.setStyle(SkPaint::kStroke_Style);
  iconPaint.setStrokeWidth(2.0f);
  iconPaint.setStrokeCap(SkPaint::kRound_Cap);
  iconPaint.setStrokeJoin(SkPaint::kRound_Join);

  // Folder shape
  SkPath folderPath;
  float fx = iconX + 4;
  float fy = iconY + 8;
  float fw = 28;
  float fh = 20;

  folderPath.moveTo(fx, fy + 4);
  folderPath.lineTo(fx, fy + fh);
  folderPath.lineTo(fx + fw, fy + fh);
  folderPath.lineTo(fx + fw, fy + 4);
  folderPath.lineTo(fx + fw * 0.6f, fy + 4);
  folderPath.lineTo(fx + fw * 0.5f, fy);
  folderPath.lineTo(fx, fy);
  folderPath.close();

  canvas->drawPath(folderPath, iconPaint);

  // Small clock in bottom-right of folder
  float clockX = iconX + 24;
  float clockY = iconY + 24;
  float clockR = 8;

  // Clock circle background
  SkPaint clockBg;
  clockBg.setAntiAlias(true);
  clockBg.setColor(colors::BG_02);
  canvas->drawCircle(clockX, clockY, clockR + 2, clockBg);

  // Clock circle
  iconPaint.setColor(colors::ORANGE);
  canvas->drawCircle(clockX, clockY, clockR, iconPaint);

  // Clock hands
  iconPaint.setStrokeWidth(1.5f);
  canvas->drawLine(clockX, clockY, clockX, clockY - 4, iconPaint);
  canvas->drawLine(clockX, clockY, clockX + 3, clockY + 1, iconPaint);
}

void ProjectRecoveryModal::drawTitle(SkCanvas *canvas) {
  using namespace design;

  // Title text
  SkPaint titlePaint;
  titlePaint.setAntiAlias(true);
  titlePaint.setColor(colors::TEXT_PRIMARY);

  juce::String titleText = "Recover Unsaved Work";
  auto titleBlob = SkTextBlob::MakeFromString(titleText.toRawUTF8(), titleFont_);
  if (titleBlob) {
    canvas->drawTextBlob(titleBlob, titleBounds_.left(),
                         titleBounds_.top() + 20, titlePaint);
  }

  // Subtitle
  SkPaint subtitlePaint;
  subtitlePaint.setAntiAlias(true);
  subtitlePaint.setColor(colors::TEXT_SECONDARY);

  juce::String subtitleText =
      "Zenith detected unsaved work from a previous session.";
  auto subtitleBlob =
      SkTextBlob::MakeFromString(subtitleText.toRawUTF8(), subtitleFont_);
  if (subtitleBlob) {
    canvas->drawTextBlob(subtitleBlob, subtitleBounds_.left(),
                         subtitleBounds_.top() + 14, subtitlePaint);
  }
}

void ProjectRecoveryModal::drawRecoveryList(SkCanvas *canvas) {
  // Draw each recovery item
  int index = 0;
  for (auto &item : items_) {
    drawRecoveryItem(canvas, item, index);
    index++;
  }
}

void ProjectRecoveryModal::drawRecoveryItem(SkCanvas *canvas,
                                            RecoveryItem &item, int index) {
  using namespace design;

  SkRect bounds = item.bounds;

  // Apply scale from spring
  float scale = item.scaleSpring.getCurrent();
  if (std::abs(scale - 1.0f) > 0.001f) {
    float cx = bounds.centerX();
    float cy = bounds.centerY();
    float w = bounds.width() * scale;
    float h = bounds.height() * scale;
    bounds = SkRect::MakeXYWH(cx - w / 2, cy - h / 2, w, h);
  }

  // Background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);

  if (item.isSelected) {
    // Selected: accent background with glow
    bgPaint.setColor(withAlpha(colors::CYAN, 0.15f));
    SkRRect bgRRect =
        SkRRect::MakeRectXY(bounds, dimensions::RADIUS_SM, dimensions::RADIUS_SM);
    canvas->drawRRect(bgRRect, bgPaint);

    // Selection border
    SkPaint selBorder;
    selBorder.setAntiAlias(true);
    selBorder.setStyle(SkPaint::kStroke_Style);
    selBorder.setStrokeWidth(1.5f);
    selBorder.setColor(withAlpha(colors::CYAN, 0.6f));
    canvas->drawRRect(bgRRect, selBorder);
  } else if (item.isHovered) {
    // Hover: subtle highlight
    bgPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.05f));
    SkRRect bgRRect =
        SkRRect::MakeRectXY(bounds, dimensions::RADIUS_SM, dimensions::RADIUS_SM);
    canvas->drawRRect(bgRRect, bgPaint);
  }

  // Project name
  SkPaint namePaint;
  namePaint.setAntiAlias(true);
  namePaint.setColor(colors::TEXT_PRIMARY);

  auto nameBlob =
      SkTextBlob::MakeFromString(item.displayName.toRawUTF8(), itemNameFont_);
  if (nameBlob) {
    canvas->drawTextBlob(nameBlob, bounds.left() + 12, bounds.top() + 24,
                         namePaint);
  }

  // Details: time ago and file size
  SkPaint detailPaint;
  detailPaint.setAntiAlias(true);
  detailPaint.setColor(colors::TEXT_SECONDARY);

  juce::String details = item.timeAgo + juce::String::fromUTF8(" • ") + item.fileSize;
  auto detailBlob =
      SkTextBlob::MakeFromString(details.toRawUTF8(), itemDetailFont_);
  if (detailBlob) {
    canvas->drawTextBlob(detailBlob, bounds.left() + 12, bounds.top() + 44,
                         detailPaint);
  }

  // Checkmark for selected item
  if (item.isSelected) {
    SkPaint checkPaint;
    checkPaint.setAntiAlias(true);
    checkPaint.setColor(colors::CYAN);
    checkPaint.setStyle(SkPaint::kStroke_Style);
    checkPaint.setStrokeWidth(2.0f);
    checkPaint.setStrokeCap(SkPaint::kRound_Cap);

    float cx = bounds.right() - 24;
    float cy = bounds.centerY();

    SkPath checkPath;
    checkPath.moveTo(cx - 6, cy);
    checkPath.lineTo(cx - 2, cy + 4);
    checkPath.lineTo(cx + 6, cy - 4);
    canvas->drawPath(checkPath, checkPaint);
  }
}

void ProjectRecoveryModal::drawButtons(SkCanvas *canvas) {
  using namespace design;

  // Discard button (secondary/muted)
  {
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    if (isDiscardHovered_) {
      bgPaint.setColor(withAlpha(colors::RED, 0.15f));
    } else {
      bgPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.05f));
    }

    SkRRect bgRRect = SkRRect::MakeRectXY(
        discardButtonBounds_, dimensions::RADIUS_SM, dimensions::RADIUS_SM);
    canvas->drawRRect(bgRRect, bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(
        isDiscardHovered_ ? withAlpha(colors::RED, 0.5f) : colors::BORDER_DEFAULT);
    canvas->drawRRect(bgRRect, borderPaint);

    // Text
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(isDiscardHovered_ ? colors::RED : colors::TEXT_SECONDARY);

    auto textBlob = SkTextBlob::MakeFromString("Discard All", buttonFont_);
    if (textBlob) {
      float textWidth = buttonFont_.measureText("Discard All", 11, SkTextEncoding::kUTF8);
      float tx = discardButtonBounds_.centerX() - textWidth / 2;
      float ty = discardButtonBounds_.centerY() + 5;
      canvas->drawTextBlob(textBlob, tx, ty, textPaint);
    }
  }

  // Recover button (primary with glow)
  {
    bool hasSelection = selectedIndex_ >= 0;
    SkColor btnColor = hasSelection ? colors::CYAN : colors::TEXT_TERTIARY;

    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    if (hasSelection) {
      // Gradient background
      SkPoint pts[2] = {{recoverButtonBounds_.left(), recoverButtonBounds_.top()},
                        {recoverButtonBounds_.right(), recoverButtonBounds_.bottom()}};
      SkColor gradColors[2] = {withAlpha(colors::CYAN, 0.3f),
                               withAlpha(colors::MAGENTA, 0.2f)};
      bgPaint.setShader(SkGradientShader::MakeLinear(pts, gradColors, nullptr, 2,
                                                     SkTileMode::kClamp));
    } else {
      bgPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.03f));
    }

    SkRRect bgRRect = SkRRect::MakeRectXY(
        recoverButtonBounds_, dimensions::RADIUS_SM, dimensions::RADIUS_SM);
    canvas->drawRRect(bgRRect, bgPaint);

    // Glow for hover
    if (isRecoverHovered_ && hasSelection) {
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setColor(withAlpha(colors::CYAN, 0.3f));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 6.0f));
      canvas->drawRRect(bgRRect, glowPaint);
    }

    // Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.5f);
    borderPaint.setColor(withAlpha(btnColor, hasSelection ? 0.8f : 0.3f));
    canvas->drawRRect(bgRRect, borderPaint);

    // Text
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(hasSelection ? colors::TEXT_PRIMARY : colors::TEXT_TERTIARY);

    const char *btnText = "Recover Selected";
    auto textBlob = SkTextBlob::MakeFromString(btnText, buttonFont_);
    if (textBlob) {
      float textWidth = buttonFont_.measureText(btnText, strlen(btnText), SkTextEncoding::kUTF8);
      float tx = recoverButtonBounds_.centerX() - textWidth / 2;
      float ty = recoverButtonBounds_.centerY() + 5;
      canvas->drawTextBlob(textBlob, tx, ty, textPaint);
    }
  }
}

void ProjectRecoveryModal::drawCloseButton(SkCanvas *canvas) {
  using namespace design;

  // Background on hover
  if (isCloseHovered_) {
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.1f));
    canvas->drawRoundRect(closeButtonBounds_, 6, 6, bgPaint);
  }

  // X icon
  SkPaint xPaint;
  xPaint.setAntiAlias(true);
  xPaint.setColor(isCloseHovered_ ? colors::TEXT_PRIMARY : colors::TEXT_SECONDARY);
  xPaint.setStyle(SkPaint::kStroke_Style);
  xPaint.setStrokeWidth(2.0f);
  xPaint.setStrokeCap(SkPaint::kRound_Cap);

  float cx = closeButtonBounds_.centerX();
  float cy = closeButtonBounds_.centerY();
  float s = 6;

  canvas->drawLine(cx - s, cy - s, cx + s, cy + s, xPaint);
  canvas->drawLine(cx + s, cy - s, cx - s, cy + s, xPaint);
}

void ProjectRecoveryModal::mouseMove(const juce::MouseEvent &e) {
  float mx = static_cast<float>(e.x);
  float my = static_cast<float>(e.y);

  // Update item hover states
  for (auto &item : items_) {
    item.isHovered = item.bounds.contains(mx, my);
  }

  // Update button hover states
  isRecoverHovered_ = recoverButtonBounds_.contains(mx, my);
  isDiscardHovered_ = discardButtonBounds_.contains(mx, my);
  isCloseHovered_ = closeButtonBounds_.contains(mx, my);

  repaint();
}

void ProjectRecoveryModal::mouseDown(const juce::MouseEvent &e) {
  float mx = static_cast<float>(e.x);
  float my = static_cast<float>(e.y);

  // Check item clicks
  for (size_t i = 0; i < items_.size(); ++i) {
    if (items_[i].bounds.contains(mx, my)) {
      selectItem(static_cast<int>(i));
      return;
    }
  }

  // Check button clicks
  if (recoverButtonBounds_.contains(mx, my) && selectedIndex_ >= 0) {
    if (onRecover_) {
      onRecover_(items_[selectedIndex_].info);
    }
    dismiss();
    return;
  }

  if (discardButtonBounds_.contains(mx, my)) {
    if (onDiscard_) {
      onDiscard_();
    }
    dismiss();
    return;
  }

  if (closeButtonBounds_.contains(mx, my)) {
    dismiss();
    return;
  }

  // Click outside card dismisses
  if (!cardBounds_.contains(mx, my)) {
    dismiss();
    return;
  }
}

void ProjectRecoveryModal::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ProjectRecoveryModal::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);

  for (auto &item : items_) {
    item.isHovered = false;
  }
  isRecoverHovered_ = false;
  isDiscardHovered_ = false;
  isCloseHovered_ = false;

  repaint();
}

bool ProjectRecoveryModal::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::escapeKey) {
    dismiss();
    return true;
  }

  if (key == juce::KeyPress::returnKey && selectedIndex_ >= 0) {
    if (onRecover_) {
      onRecover_(items_[selectedIndex_].info);
    }
    dismiss();
    return true;
  }

  if (key == juce::KeyPress::upKey && !items_.empty()) {
    int newIndex = selectedIndex_ > 0 ? selectedIndex_ - 1
                                      : static_cast<int>(items_.size()) - 1;
    selectItem(newIndex);
    return true;
  }

  if (key == juce::KeyPress::downKey && !items_.empty()) {
    int newIndex = selectedIndex_ < static_cast<int>(items_.size()) - 1
                       ? selectedIndex_ + 1
                       : 0;
    selectItem(newIndex);
    return true;
  }

  return false;
}

bool ProjectRecoveryModal::hitTest(int x, int y) {
  // Always capture all input when visible and NOT dismissing
  juce::ignoreUnused(x, y);
  return isVisible_ && alpha_.get() > 0.01f;
}

void ProjectRecoveryModal::selectItem(int index) {
  if (index < 0 || index >= static_cast<int>(items_.size()))
    return;

  // Deselect previous
  if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
    items_[selectedIndex_].isSelected = false;
  }

  selectedIndex_ = index;
  items_[selectedIndex_].isSelected = true;

  repaint();
}

juce::String ProjectRecoveryModal::formatTimeAgo(juce::int64 timestamp) const {
  juce::int64 now = juce::Time::currentTimeMillis() / 1000;
  juce::int64 diff = now - timestamp;

  if (diff < 60) {
    return "Just now";
  } else if (diff < 3600) {
    int mins = static_cast<int>(diff / 60);
    return juce::String(mins) + (mins == 1 ? " minute ago" : " minutes ago");
  } else if (diff < 86400) {
    int hours = static_cast<int>(diff / 3600);
    return juce::String(hours) + (hours == 1 ? " hour ago" : " hours ago");
  } else if (diff < 604800) {
    int days = static_cast<int>(diff / 86400);
    return juce::String(days) + (days == 1 ? " day ago" : " days ago");
  } else {
    juce::Time t(timestamp * 1000);
    return t.formatted("%b %d, %Y");
  }
}

juce::String ProjectRecoveryModal::formatFileSize(juce::int64 bytes) const {
  if (bytes < 1024) {
    return juce::String(bytes) + " B";
  } else if (bytes < 1024 * 1024) {
    return juce::String(bytes / 1024.0, 1) + " KB";
  } else if (bytes < 1024 * 1024 * 1024) {
    return juce::String(bytes / (1024.0 * 1024.0), 1) + " MB";
  } else {
    return juce::String(bytes / (1024.0 * 1024.0 * 1024.0), 2) + " GB";
  }
}

} // namespace zenith
