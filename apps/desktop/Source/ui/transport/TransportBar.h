/*
  ==============================================================================

    TransportBar.h
    Created: 2025-11-28
    Author:  Leo "Lil Bit" Rossi

    Transport controls with Neon Noir styling.
    Play, Stop, Record, Tempo, CPU meter, Timeline.

  ==============================================================================
*/

#pragma once

#include "../design-system/InteractionHelper.h"
#include "SkiaComponent.h"
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkPath.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class TransportBar : public SkiaComponent {
public:
  TransportBar();
  ~TransportBar() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  void timerCallback() override;
  void visibilityChanged() override;

  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

  // State setters
  void setPlaying(bool playing) {
    isPlaying_ = playing;
    repaint();
  }
  void setRecording(bool recording) {
    isRecording_ = recording;
    repaint();
  }
  void setTempo(double bpm) {
    tempo_ = bpm;
    repaint();
  }
  void setCPU(float percent) {
    cpuUsage_ = percent;
    repaint();
  }
  void setPosition(double seconds) {
    position_ = seconds;
    repaint();
  }

  // New setters
  void setProjectName(const juce::String &name) {
    projectName_ = name;
    repaint();
  }
  void setTimeSignature(int num, int den) {
    timeSigNum_ = num;
    timeSigDen_ = den;
    repaint();
  }

  // Callbacks - Transport controls only
  std::function<void()> onPlayClicked;
  std::function<void()> onStopClicked;
  std::function<void()> onRecordClicked;
  std::function<void()> onReturnToStart;  // NEW: Return to position 0
  std::function<void()> onLoopToggled;

private:
  bool isPlaying_ = false;
  bool isRecording_ = false;
  double tempo_ = 120.0;
  float cpuUsage_ = 0.0f;
  double position_ = 0.0;
  juce::String projectName_ = "Zenith DAW";
  int timeSigNum_ = 4;
  int timeSigDen_ = 4;

  // Button bounds - centered transport group
  juce::Rectangle<int> returnToStartButtonBounds_;  // NEW
  juce::Rectangle<int> playButtonBounds_;
  juce::Rectangle<int> stopButtonBounds_;
  juce::Rectangle<int> recordButtonBounds_;

  // Interaction states
  InteractionState returnToStartState_;  // NEW
  InteractionState playState_;
  InteractionState stopState_;
  InteractionState recordState_;

  // Modern drawing methods
  void drawModernBackground(SkCanvas *canvas, const SkRect &bounds);
  void drawModernButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                        const SkPath &iconPath, bool isActive,
                        uint32_t accentColor, const InteractionState &state);

  // Legacy (kept for compatibility)
  void drawTransportButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                           const SkPath &iconPath, bool isActive,
                           uint32_t color, const InteractionState &state);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)

  // Cached resources for 60FPS rendering
  ::SkPaint bgPaint_;
  ::SkPaint borderPaint_;
  ::SkFont font_;
  ::SkFont smallFont_;
  ::SkRect cachedBounds_;

  void updateCachedPaints(const ::SkRect &bounds);
};

#else // ZENITH_USE_SKIA

class TransportBar : public juce::Component {
public:
    TransportBar() {}
    ~TransportBar() override = default;
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
    void setPlaying(bool) {}
    void setRecording(bool) {}
    void setTempo(double) {}
    void setCPU(float) {}
    void setPosition(double) {}
    void setProjectName(const juce::String&) {}
    void setTimeSignature(int, int) {}
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onReturnToStart;
    std::function<void()> onLoopToggled;
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
