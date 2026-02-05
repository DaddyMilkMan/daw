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

    UnsavedChangesModal.cpp
    Created: 2025-12-30
    Author:  Zenith DAW

  ==============================================================================
*/


#include "UnsavedChangesModal.h"
#include "../design-system/ZenithTheme.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/GlassmorphicPanel.h"

namespace zenith {

UnsavedChangesModal::UnsavedChangesModal() {
  // Save & Quit (Primary)
  btnSave_ = std::make_unique<SkiaButton>("Save & Quit");
  btnSave_->setStyle(SkiaButton::Style::Primary);
  btnSave_->onClick = [this] { if (onSaveAndQuit) onSaveAndQuit(); };
  addAndMakeVisible(btnSave_.get());

  // Discard (Destructive/Secondary)
  btnDiscard_ = std::make_unique<SkiaButton>("Discard Changes");
  btnDiscard_->setStyle(SkiaButton::Style::Ghost); // Or error style if available? Ghost is fine.
  btnDiscard_->onClick = [this] { if (onDiscardAndQuit) onDiscardAndQuit(); };
  addAndMakeVisible(btnDiscard_.get());

  // Cancel
  btnCancel_ = std::make_unique<SkiaButton>("Cancel");
  btnCancel_->setStyle(SkiaButton::Style::Ghost);
  btnCancel_->onClick = [this] { if (onCancel) onCancel(); };
  addAndMakeVisible(btnCancel_.get());

  // Set default size (centered by MainWindow usually)
  setSize(500, 260);
}

UnsavedChangesModal::~UnsavedChangesModal() {}

void UnsavedChangesModal::resized() {
    auto area = getLocalBounds().reduced(40);
    
    // Bottom row for buttons
    auto buttonRow = area.removeFromBottom(40);
    
    // Layout: Cancel (Left) ... Discard (Right-ish) ... Save (Far Right)
    // Actually standard dialog: [Discard] ... [Cancel] [Save]
    
    btnDiscard_->setBounds(buttonRow.removeFromLeft(140));
    
    btnSave_->setBounds(buttonRow.removeFromRight(120));
    buttonRow.removeFromRight(20); // Gap
    btnCancel_->setBounds(buttonRow.removeFromRight(100));
}

void UnsavedChangesModal::drawSkia(SkCanvas* canvas) {
    // Glass Background (Floating)
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
    GlassmorphicPanel::draw(canvas, rect, GlassmorphicPanel::Style::Floating);
    
    // Title
    SkFont titleFont = design::getSkFont(24.0f, design::FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);
    
    canvas->drawString("Unsaved Changes", 40, 60, titleFont, textPaint);
    
    // Body Text
    SkFont bodyFont = design::getSkFont(16.0f, design::FontWeight::Regular);
    textPaint.setColor(SkColorSetA(SK_ColorWHITE, 200));
    
    const char* text1 = "You have unsaved changes in your project.";
    const char* text2 = "Do you want to save your progress before quitting?";
    
    canvas->drawString(text1, 40, 110, bodyFont, textPaint);
    canvas->drawString(text2, 40, 135, bodyFont, textPaint);
    
    // Warning Icon/Accent (Optional)
    SkPaint accentPaint;
    accentPaint.setColor(design::colors::WARNING); // Use WARNING (Amber)
    accentPaint.setStyle(SkPaint::kStroke_Style);
    accentPaint.setStrokeWidth(2.0f);
    // Could draw a warning triangle here if we wanted
}

} // namespace zenith
