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

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkImage.h>
#include <core/SkPath.h>
#include <core/SkTextBlob.h>
#endif

namespace zenith {

class ZenithButton : public SkiaComponent, public juce::TooltipClient {
public:
  // ----- Button Styles -----
  enum class Style {
    Primary,   // Accent color, high emphasis
    Secondary, // Panel color, medium emphasis
    Danger,    // Red, destructive actions
    Warning,   // Amber, caution
    Success,   // Green, positive actions
    Ghost      // Transparent with border
  };

  // ----- Button Sizes -----
  enum class Size {
    Small,  // 24px height
    Medium, // 32px height (default)
    Large   // 40px height
  };

  // ----- Icon Position -----
  enum class IconPosition {
    Left,
    Right,
    Only // Icon only, no text
  };

  // ----- Constructors -----
  ZenithButton();
  explicit ZenithButton(const juce::String &text,
                        std::function<void()> clickHandler = nullptr);
  ~ZenithButton() override;

  // ----- Text -----
  void setButtonText(const juce::String &text);
  void setText(const juce::String &text) { setButtonText(text); }
  juce::String getButtonText() const { return text_; }
  juce::String getText() const { return text_; }

  // ----- Style & Size -----
  void setButtonStyle(Style style);
  void setStyle(Style style) { setButtonStyle(style); }
  Style getButtonStyle() const { return style_; }
  Style getStyle() const { return style_; }

  void setButtonSize(Size size);
  void setSize(Size size) { setButtonSize(size); }
  Size getButtonSize() const { return size_; }
  Size getSize() const { return size_; }

  // ----- Icon -----
#ifdef ZENITH_USE_SKIA
  void setIcon(sk_sp<SkImage> icon);
  sk_sp<SkImage> getIcon() const { return icon_; }
  void setIconPath(const SkPath& path);
  SkPath getIconPath() const { return iconPath_; }
#endif
  void setIconText(const juce::String &iconText); // Unicode icons/emojis
  void setIconPosition(IconPosition pos);
  IconPosition getIconPosition() const { return iconPosition_; }

  // ----- Toggle Mode -----
  void setToggleable(bool toggleable);
  bool isToggleable() const { return toggleable_; }
  void setToggleState(bool state, bool sendNotification = true);
  bool getToggleState() const { return toggleState_; }

  // ----- Audio Reactive -----
  void setAudioReactive(bool reactive);
  bool isAudioReactive() const { return audioReactive_; }
  void setAudioLevel(float level); // 0.0 to 1.0

  // ----- State -----
  void setEnabled(bool enabled);
  bool isDown() const { return pressed_; }
  bool isHovered() const { return hovered_; }

  // ----- Callbacks -----
  std::function<void()> onClick;
  std::function<void(bool)> onToggle;

  // ----- TooltipClient -----
  juce::String getTooltip() override { return tooltip_; }
  void setTooltip(const juce::String &text) { tooltip_ = text; }

  // ----- Keyboard -----
  bool keyPressed(const juce::KeyPress &key) override;

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

protected:
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void focusGained(juce::Component::FocusChangeType cause) override;
  void focusLost(juce::Component::FocusChangeType cause) override;
  void resized() override;

private:
#ifdef ZENITH_USE_SKIA
  SkColor getBackgroundColor() const;
  SkColor getTextColor() const;
  SkColor getBorderColor() const;
  SkColor getGlowColor() const;

  void drawGlow(SkCanvas *canvas, const SkRRect &bounds);
  void drawBackground(SkCanvas *canvas, const SkRRect &bounds);
  void drawBorder(SkCanvas *canvas, const SkRRect &bounds);
  void drawIcon(SkCanvas *canvas, const SkRect &iconRect);
  void drawText(SkCanvas *canvas, const SkRect &textRect);

  void calculateLayout();
#endif

  // Content
  juce::String text_;
  juce::String tooltip_;
  juce::String iconText_;
#ifdef ZENITH_USE_SKIA
  sk_sp<SkImage> icon_;
  SkPath iconPath_;
  sk_sp<SkTextBlob> textBlob_;
#endif

  // Style
  Style style_ = Style::Secondary;
  Size size_ = Size::Medium;
  IconPosition iconPosition_ = IconPosition::Left;

  // State
  bool toggleable_ = false;
  bool toggleState_ = false;
  bool pressed_ = false;
  bool hovered_ = false;
  bool focused_ = false;

  // Audio reactive
  bool audioReactive_ = false;
  float audioLevel_ = 0.0f;

  // Animation
  float hoverProgress_ = 0.0f;
  float pressProgress_ = 0.0f;

  // Layout cache
  bool layoutDirty_ = true;
  bool textDirty_ = true;
#ifdef ZENITH_USE_SKIA
  SkRect iconRect_;
  SkRect textRect_;
#endif

  float getButtonHeight() const;
  float getCornerRadius() const;
  float getFontSize() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithButton)
};

// Legacy alias for backward compatibility
using SkiaButton = ZenithButton;

} // namespace zenith
