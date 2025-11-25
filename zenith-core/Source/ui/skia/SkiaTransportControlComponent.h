/**
 * @file SkiaTransportControlComponent.h
 * @brief GPU-rendered transport controls with professional animations
 *
 * Features:
 * - Circular LED-style buttons with glow effects and hover animations
 * - Play/Stop/Record buttons with state indicators
 * - Pulse animation for recording state (breathing effect)
 * - Tempo display with tap tempo capability
 * - Timeline position indicator with LCD-style formatting
 * - 60fps smooth animations via Timer
 * - Complete Skia GPU rendering
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaTransportControlComponent : public juce::Component, private juce::Timer {
public:
  SkiaTransportControlComponent();
  ~SkiaTransportControlComponent() override;

  void setIsPlaying(bool playing);
  bool getIsPlaying() const { return isPlaying_; }

  void setIsRecording(bool recording);
  bool getIsRecording() const { return isRecording_; }

  void setTempo(float bpm);
  float getTempo() const { return tempo_; }

  void setTimelinePosition(double seconds);
  double getTimelinePosition() const { return timelinePosition_; }

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &event) override;
  void mouseMove(const juce::MouseEvent &event) override;
  void mouseExit(const juce::MouseEvent &event) override;

private:
  void timerCallback() override;

  // Skia rendering methods
  void drawButtonCircular(juce::Graphics& g, float x, float y, float diameter,
                          bool isActive, bool isHovered, const juce::String& icon,
                          const juce::Colour& activeColor);
  void drawTempoDisplay(juce::Graphics& g, int x, int y, int width, int height);
  void drawTimelineDisplay(juce::Graphics& g, int x, int y, int width, int height);

  // State
  bool isPlaying_ = false;
  bool isRecording_ = false;
  bool isLooping_ = false;
  float tempo_ = 120.0f;
  double timelinePosition_ = 0.0;

  // Animation state
  float playButtonGlow_ = 0.0f;
  float stopButtonGlow_ = 0.0f;
  float recordButtonGlow_ = 0.0f;
  float recordingPulse_ = 0.0f;  // For breathing/pulse effect (0-1)

  // Hover states
  bool isPlayHovered_ = false;
  bool isStopHovered_ = false;
  bool isRecordHovered_ = false;

  // Button hit regions (for mouse detection)
  juce::Rectangle<float> playButtonBounds_;
  juce::Rectangle<float> stopButtonBounds_;
  juce::Rectangle<float> recordButtonBounds_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTransportControlComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
