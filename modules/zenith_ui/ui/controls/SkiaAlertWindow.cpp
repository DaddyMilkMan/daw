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

    SkiaAlertWindow.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based alert window implementation

  ==============================================================================

*/

#include "SkiaAlertWindow.h"
#include "ZenithDesignSystem.h"

namespace zenith {

bool SkiaAlertWindow::testModeEnabled_ = false;

SkiaAlertWindow::SkiaAlertWindow(const juce::String &title,
                                 const juce::String &message, IconType iconType)
    : iconType_(iconType) {
  setName(title);

  // Create title label
  titleLabel_ = std::make_unique<SkiaLabel>();
  titleLabel_->setText(title, juce::dontSendNotification);
  titleLabel_->setFont(18.0f);
  titleLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  titleLabel_->setJustification(SkiaLabel::Justification::Left);
  addAndMakeVisible(titleLabel_.get());

  // Create message label
  messageLabel_ = std::make_unique<SkiaLabel>();
  messageLabel_->setText(message, juce::dontSendNotification);
  messageLabel_->setFont(design::typography::FONT_MD);
  messageLabel_->setTextColour(design::colors::TEXT_SECONDARY);
  messageLabel_->setJustification(SkiaLabel::Justification::TopLeft);
  addAndMakeVisible(messageLabel_.get());

  // Create icon label if needed
  if (iconType != IconType::NoIcon) {
    iconLabel_ = std::make_unique<SkiaLabel>();

    const char *iconText = "";
    switch (iconType) {
    case IconType::QuestionIcon:
      iconText = "[?]";
      break;
    case IconType::WarningIcon:
      iconText = "[!]";
      break;
    case IconType::InfoIcon:
      iconText = "[i]";
      break;
    default:
      break;
    }

    iconLabel_->setText(iconText, juce::dontSendNotification);
    iconLabel_->setFont(24.0f);
    iconLabel_->setTextColour(design::colors::TEXT_PRIMARY);
    iconLabel_->setJustification(SkiaLabel::Justification::Center);
    addAndMakeVisible(iconLabel_.get());
  }

  // Set default appearance
  backgroundColour_ = design::colors::BG_DARKER;
  borderColour_ = design::colors::BORDER_DEFAULT;
  font_.setSize(design::typography::FONT_MD);

  // Set default size
  setSize(400, 200);
}

SkiaAlertWindow::~SkiaAlertWindow() {}

void SkiaAlertWindow::addButton(const juce::String &text, Result result,
                                SkiaButton::Style style) {
  ButtonInfo info;
  info.text = text;
  info.result = result;
  info.style = style;
  buttons_.add(info);

  layoutComponents();
}

void SkiaAlertWindow::addTextEditor(const juce::String &name,
                                    const juce::String &initialText,
                                    const juce::String &labelText,
                                    bool isPassword) {
  TextEditorInfo info;
  info.name = name;
  info.isPassword = isPassword;

  // Create label
  info.label = std::make_unique<SkiaLabel>();
  info.label->setText(labelText, juce::dontSendNotification);
  info.label->setFont(design::typography::FONT_SM);
  info.label->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(info.label.get());

  // Create text editor
  info.editor = std::make_unique<SkiaTextEditor>();
  info.editor->setText(initialText);
  info.editor->setMultiLine(false);
  if (isPassword) {
    // Note: Password masking would need to be implemented in SkiaTextEditor
  }
  addAndMakeVisible(info.editor.get());

  textEditors_.add(std::move(info));
  layoutComponents();
}

juce::String
SkiaAlertWindow::getTextEditorContents(const juce::String &name) const {
  for (const auto &editorInfo : textEditors_) {
    if (editorInfo.name == name && editorInfo.editor) {
      return editorInfo.editor->getText();
    }
  }
  return {};
}

void SkiaAlertWindow::showAsync(Callback callback) {
  callback_ = callback;
  
  if (testModeEnabled_) {
      DBG("SkiaAlertWindow: Test mode enabled, auto-dismissing '" + getName() + "'");
      handleButtonPressed(Result::Cancelled); // Default to Cancel/Close
      return;
  }
  
  // In a real implementation, this would show as a modal dialog
  setVisible(true);
  toFront(true);
  grabKeyboardFocus();
}

void SkiaAlertWindow::showMessageBoxAsync(IconType iconType,
                                          const juce::String &title,
                                          const juce::String &message,
                                          const juce::String &buttonText) {
  // Bug 23: Use shared_ptr to prevent double delete or leaks if callback never fires
  auto alert = std::shared_ptr<SkiaAlertWindow>(new SkiaAlertWindow(title, message, iconType));
  std::weak_ptr<SkiaAlertWindow> weakAlert = alert;

  alert->addButton(buttonText, Result::Button1);
  
  // Pass shared_ptr to keep it alive until callback finishes
  alert->showAsync([weakAlert, alert](Result result) {
    juce::ignoreUnused(result);
    // Alert will be destroyed when shared_ptr goes out of scope
  });
}

void SkiaAlertWindow::showAsync(IconType iconType, const juce::String &title,
                                const juce::String &message,
                                const juce::String &button1Text,
                                const juce::String &button2Text,
                                const juce::String &button3Text,
                                std::function<void(int)> callback) {
  auto *alert = new SkiaAlertWindow(title, message, iconType);

  alert->addButton(button1Text, Result::Button1);
  if (button2Text.isNotEmpty()) {
    alert->addButton(button2Text, Result::Button2,
                     SkiaButton::Style::Secondary);
  }
  if (button3Text.isNotEmpty()) {
    alert->addButton(button3Text, Result::Button3,
                     SkiaButton::Style::Secondary);
  }

  alert->showAsync([alert, callback](Result result) {
    if (callback) {
      callback(static_cast<int>(result));
    }
    delete alert;
  });
}

void SkiaAlertWindow::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(backgroundColour_);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  // Draw border
  SkPaint borderPaint;
  borderPaint.setColor(borderColour_);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   borderPaint);

  // Draw title bar background
  SkPaint titleBgPaint;
  titleBgPaint.setColor(design::colors::BG_DARK);
  canvas->drawRect(SkRect::MakeXYWH(0, 0, bounds.getWidth(), 40), titleBgPaint);

  // Draw child components (labels, buttons, etc.)
  drawChildren(canvas);
}

void SkiaAlertWindow::resized() { layoutComponents(); }

void SkiaAlertWindow::layoutComponents() {
  auto bounds = getLocalBounds();

  // Title bar
  auto titleBar = bounds.removeFromTop(40);
  if (iconLabel_) {
    iconLabel_->setBounds(titleBar.removeFromLeft(40).reduced(8));
  }
  titleLabel_->setBounds(titleBar.reduced(10, 5));

  // Message area
  auto messageArea = bounds.removeFromTop(60);
  messageLabel_->setBounds(messageArea.reduced(20, 10));

  // Text editors
  for (auto &editorInfo : textEditors_) {
    auto editorRow = bounds.removeFromTop(50);
    editorInfo.label->setBounds(editorRow.removeFromTop(20).reduced(20, 0));
    editorInfo.editor->setBounds(editorRow.reduced(20, 0));
  }

  // Buttons at bottom
  auto buttonArea = bounds.removeFromBottom(60);
  int buttonWidth = 100;
  int spacing = 10;
  int totalButtonWidth =
      buttons_.size() * buttonWidth + (buttons_.size() - 1) * spacing;
  int startX = (buttonArea.getWidth() - totalButtonWidth) / 2;

  // Clear existing button components
  buttonComponents_.clear();

  // Create and position buttons
  for (int i = 0; i < buttons_.size(); ++i) {
    const auto &buttonInfo = buttons_[i];

    auto *button = new SkiaButton();
    button->setText(buttonInfo.text);
    button->setStyle(buttonInfo.style);
    button->onClick = [this, result = buttonInfo.result]() {
      handleButtonPressed(result);
    };

    button->setBounds(startX + i * (buttonWidth + spacing),
                      buttonArea.getCentreY() - buttonHeight_ / 2, buttonWidth,
                      buttonHeight_);

    addAndMakeVisible(button);
    buttonComponents_.add(button);
  }
}

void SkiaAlertWindow::handleButtonPressed(Result result) {
  hideWindow();
  if (callback_) {
    callback_(result);
  }
}

void SkiaAlertWindow::hideWindow() {
  setVisible(false);
  if (getParentComponent()) {
    getParentComponent()->removeChildComponent(this);
  }
}

} // namespace zenith