/*
  ==============================================================================

    SkiaButton.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based button component to replace juce::TextButton

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {

class SkiaButton : public SkiaComponent {
public:
  enum class Style {
    Primary,   // Accent color
    Secondary, // Panel color
    Danger,    // Red
    Warning,   // Amber
    Success,   // Green
    Ghost      // Transparent with border
  };

  enum class Size {
    Small,  // 24px height
    Medium, // 32px height (default)
    Large   // 40px height
  };

  SkiaButton(const juce::String &buttonText = {});
  ~SkiaButton() override;

  // Text and icon
  void setButtonText(const juce::String &text);
  juce::String getButtonText() const { return buttonText_; }

  void setIcon(const juce::String &iconText); // Unicode icons/emojis

  // Style and appearance
  void setButtonStyle(Style style);
  Style getButtonStyle() const { return style_; }

  void setButtonSize(Size size);
  Size getButtonSize() const { return size_; }

  // Toggle button support
  void setToggleable(bool toggleable);
  bool isToggleable() const { return toggleable_; }

  void setToggleState(bool state, bool sendNotification = true);
  bool getToggleState() const { return toggleState_; }

  // Enabled/disabled
  void setEnabled(bool enabled);

  // Event callbacks
  std::function<void()> onClick;
  std::function<void(bool)> onToggle; // Called when toggle state changes

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

private:
  juce::String buttonText_;
  juce::String iconText_;
  Style style_ = Style::Secondary;
  Size size_ = Size::Medium;

  bool toggleable_ = false;
  bool toggleState_ = false;
  bool isPressed_ = false;

  // Animation values
  float hoverProgress_ = 0.0f; // 0.0 to 1.0
  float pressProgress_ = 0.0f; // 0.0 to 1.0

  // Cached colors for current style
  SkColor bgColorNormal_;
  SkColor bgColorHover_;
  SkColor bgColorPressed_;
  SkColor textColor_;
  SkColor borderColor_;

  void updateColorsForStyle();
  SkRect getButtonRect() const;
  float getButtonHeight() const;
  float getCornerRadius() const;
  void animateHover(bool hover);
  void animatePress(bool press);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaButton)
};

} // namespace zenith