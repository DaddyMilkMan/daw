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
  
  // Mouse interaction
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  bool hitTest(int x, int y) override;

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
  
  bool isPlaying() const { return isPlaying_; }
  bool isRecording() const { return isRecording_; }

  // New setters
  void setTimeSignature(int num, int den) {
    timeSigNum_ = num;
    timeSigDen_ = den;
    repaint();
  }

  // Callbacks
  std::function<void()> onPlayClicked;
  std::function<void()> onStopClicked;
  std::function<void()> onRecordClicked;
  std::function<void()> onLoopToggled;
  std::function<void()> onRewind;
  std::function<void()> onViewToggleClicked;
  std::function<void()> onSettingsClicked;
  std::function<void()> onExportClicked;
  std::function<void()> onClearAllSolos;

  // Interaction Callbacks
  std::function<void(double)> onTempoChanged;
  std::function<void(int, int)> onTimeSignatureChanged;

private:
  bool isPlaying_ = false;
  bool isRecording_ = false;
  double tempo_ = 120.0;
  float cpuUsage_ = 0.0f;
  double position_ = 0.0;
  int timeSigNum_ = 4;
  int timeSigDen_ = 4;
  
  // Editors
  std::unique_ptr<juce::Label> bpmLabel_;
  std::unique_ptr<juce::Label> timeSigLabel_;       // Legacy (unused now)
  std::unique_ptr<juce::Label> timeSigNumLabel_;    // Numerator editor
  std::unique_ptr<juce::Label> timeSigDenLabel_;    // Denominator editor
  bool editingTimeSigNum_ = false;                  // Track which field is active
  
  // Interaction State
  bool isDraggingBpm_ = false;
  bool isDraggingTimeSig_ = false;
  double dragStartValue_ = 0.0;
  int dragStartNum_ = 0;
  int dragStartDen_ = 0;
  juce::Point<int> dragStartPos_;
  
  // Sub-bounds for hit testing
  juce::Rectangle<int> bpmHitBounds_;
  juce::Rectangle<int> timeSigHitBounds_;

  juce::Rectangle<int> playButtonBounds_;
  juce::Rectangle<int> stopButtonBounds_;
  juce::Rectangle<int> recordButtonBounds_;
  juce::Rectangle<int> viewToggleButtonBounds_;
  juce::Rectangle<int> settingsButtonBounds_;
  juce::Rectangle<int> exportButtonBounds_;

  // Dynamic layout bounds
  juce::Rectangle<int> lcdBounds_;
  juce::Rectangle<int> cpuMeterBounds_;

  // Interaction states
  InteractionState playState_;
  InteractionState stopState_;
  InteractionState recordState_;
  InteractionState viewToggleState_;
  InteractionState settingsState_;
  InteractionState exportState_;

  void drawTransportButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                           const SkPath &iconPath, bool isActive,
                           uint32_t color, const InteractionState &state);
  void drawMeter(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                 float value, const char *label);

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
    void paint(juce::Graphics& g) override {} // Handled by Skia
    void setPlaying(bool) {}
    void setRecording(bool) {}
    void setTempo(double) {}
    void setCPU(float) {}
    void setPosition(double) {}
    void setTimeSignature(int, int) {}
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onViewToggleClicked;
    std::function<void()> onSettingsClicked;
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
