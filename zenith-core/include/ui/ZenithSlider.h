/**
 * @file ZenithSlider.h
 * @brief Beautiful custom slider with gradients and animations
 *
 * Modern slider design with:
 * - Vertical and horizontal orientations
 * - Ableton-style gradient track
 * - Animated thumb with shadow and glow
 * - Hover effects with scaling
 * - Value tooltip on hover/drag
 * - Smooth 60 Hz animations
 * - NO JUCE default Slider - completely custom drawn
 */

#pragma once

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
 * @class ZenithSlider
 * @brief Beautiful custom slider component (vertical or horizontal)
 *
 * Features:
 * - Gradient track (lighter at top/left, darker at bottom/right)
 * - Animated thumb with shadow, gradient, highlight
 * - Hover glow effect
 * - Drag interaction with smooth sensitivity
 * - Value tooltip showing current value
 * - Double-click to reset to default value
 * - Configurable range (min, max, default)
 * - Label and suffix support
 * - 60 Hz smooth animations
 */
class ZenithSlider : public juce::Component,
                     public juce::Timer,
                     public juce::TooltipClient {
public:
  enum Orientation { Vertical, Horizontal };

  ZenithSlider(Orientation orientation = Vertical);
  ~ZenithSlider() override;

  //==========================================================================
  // Value control
  //==========================================================================

  /**
   * @brief Set the slider value (0.0 to 1.0 normalized)
   * @param newValue Normalized value 0.0 to 1.0
   * @param sendNotification Whether to call onValueChange callback
   */
  void setValue(float newValue, bool sendNotification = true);

  /**
   * @brief Get the current normalized value (0.0 to 1.0)
   */
  float getValue() const { return value_; }

  /**
   * @brief Set the value range and default value
   * @param min Minimum value
   * @param max Maximum value
   * @param defaultValue Default value for double-click reset
   */
  void setRange(float min, float max, float defaultValue);

  /**
   * @brief Get the display value (denormalized to actual range)
   */
  float getDisplayValue() const;

  /**
   * @brief Set the label text displayed at the bottom/side
   */
  void setLabel(const juce::String &label) {
    label_ = label;
    repaint();
  }

  /**
   * @brief Set the suffix for the value display (e.g., "dB", "Hz", "%")
   */
  /**
   * @brief Set the suffix for the value display (e.g., "dB", "Hz", "%")
   */
  void setSuffix(const juce::String &suffix) { suffix_ = suffix; }

  /**
   * @brief Set the tooltip text
   */
  void setTooltip(const juce::String &text) { tooltipText_ = text; }

  /**
   * @brief Get the tooltip text (TooltipClient override)
   */
  juce::String getTooltip() override { return tooltipText_; }

  /**
   * @brief Callback when value changes
   * @param displayValue The actual value (not normalized)
   */
  std::function<void(float)> onValueChange;

  //==========================================================================
  // Component interface
  //==========================================================================

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  //==========================================================================
  // Mouse interaction
  //==========================================================================

  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;
  void mouseUp(const juce::MouseEvent &event) override;
  void mouseEnter(const juce::MouseEvent &event) override;
  void mouseExit(const juce::MouseEvent &event) override;
  void mouseDoubleClick(const juce::MouseEvent &event) override;

private:
  //==========================================================================
  // Drawing methods
  //==========================================================================

  void drawVerticalSlider(juce::Graphics &g,
                          const juce::Rectangle<float> &bounds);
  void drawHorizontalSlider(juce::Graphics &g,
                            const juce::Rectangle<float> &bounds);
  void drawTrack(juce::Graphics &g, const juce::Rectangle<float> &trackBounds);
  void drawThumb(juce::Graphics &g, const juce::Rectangle<float> &thumbBounds);
  void drawLabel(juce::Graphics &g, const juce::Rectangle<float> &bounds);

  //==========================================================================
  // Member variables
  //==========================================================================

  Orientation orientation_;

  // Value state
  float value_ = 0.7f;       // Current animated value (0.0 to 1.0)
  float targetValue_ = 0.7f; // Target value for smooth animation
  float minValue_ = 0.0f;
  float maxValue_ = 1.0f;
  float defaultValue_ = 0.5f;

  // Display
  juce::String label_;
  juce::String suffix_;
  juce::String tooltipText_;

  // Interaction state
  bool isHovered_ = false;
  bool isDragging_ = false;
  juce::Point<int> dragStartPos_;
  float dragStartValue_ = 0.0f;

  // Animation state
  float hoverAnimation_ = 0.0f; // 0.0 to 1.0 for smooth hover animation
  float dragAnimation_ = 0.0f;  // 0.0 to 1.0 for drag feedback

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSlider)
};

} // namespace zenith
