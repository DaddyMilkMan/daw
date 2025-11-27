/**
 * @file SkiaButtonComponent_NEW.h
 * @brief NEW ARCHITECTURE - Button using SkiaComponent base class
 *
 * This demonstrates the CORRECT way to create Skia components:
 * 1. Inherit from SkiaComponent (not juce::Component)
 * 2. Override paintSkia() instead of paint()
 * 3. Don't create your own SkiaRenderer
 * 4. Content automatically appears on screen!
 *
 * Compare this to the old SkiaButtonComponent.h - much simpler!
 */

#pragma once

#include "SkiaComponent.h"
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

/**
 * @class SkiaButtonComponent_NEW
 * @brief Modern Skia button using correct architecture
 *
 * Features:
 * - GPU-accelerated rendering (when available)
 * - Automatic fallback to software rendering
 * - Proper integration with JUCE window system
 * - Shared GPU context across all components
 * - Spring physics animations
 */
class SkiaButtonComponent_NEW : public SkiaComponent, private juce::Timer {
public:
  enum class Style {
    Primary,   ///< Blue accent
    Secondary, ///< Gray
    Success,   ///< Green
    Danger,    ///< Red
    Warning    ///< Orange
  };

  /**
   * @brief Construct button
   */
  explicit SkiaButtonComponent_NEW(const juce::String &buttonText = {},
                                   Style style = Style::Primary);

  ~SkiaButtonComponent_NEW() override;

  //==========================================================================
  // Skia Rendering (overrides SkiaComponent)
  //==========================================================================

  /**
   * @brief Render button with Skia
   *
   * This is all you need! No manual blitting, no SkiaRenderer management.
   * Just draw, and it appears on screen.
   */
  /**
   * @brief Render button with Skia
   *
   * This is all you need! No manual blitting, no SkiaRenderer management.
   * Just draw, and it appears on screen.
   */
  void drawSkia(SkCanvas *canvas) override;

  //==========================================================================
  // Mouse Events
  //==========================================================================

  void mouseEnter(const juce::MouseEvent &event) override;
  void mouseExit(const juce::MouseEvent &event) override;
  void mouseDown(const juce::MouseEvent &event) override;
  void mouseUp(const juce::MouseEvent &event) override;

  //==========================================================================
  // Configuration
  //==========================================================================

  void setButtonText(const juce::String &text);
  juce::String getButtonText() const { return buttonText_; }

  void setStyle(Style style);
  Style getStyle() const { return style_; }

  std::function<void()> onClick;

private:
  // State
  juce::String buttonText_;
  Style style_;
  bool isHovered_ = false;
  bool isPressed_ = false;

  // Animation state (spring physics)
  float hoverProgress_ = 0.0f;
  float pressProgress_ = 0.0f;
  float hoverVelocity_ = 0.0f;
  float pressVelocity_ = 0.0f;

  // Methods
  void timerCallback() override;
  void updateAnimations();
  juce::Colour getStyleColour() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaButtonComponent_NEW)
};

} // namespace zenith
