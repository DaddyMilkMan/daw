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
  
  // Mouse interaction (Only for custom LCD dragging)
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  bool hitTest(int x, int y) override;
  
  void visibilityChanged() override;
  
  void onAnimationTick(float deltaMs) override;
  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
  
  // State setters
  void setPlaying(bool playing);
  void setRecording(bool recording);
  void setLooping(bool looping);
  void setMetronomeEnabled(bool enabled);
  void setTempo(double bpm);
  void setCPU(float percent);
  void setPosition(double seconds);
  
  bool isPlaying() const { return isPlaying_; }
  bool isRecording() const { return isRecording_; }
  bool isLooping() const { return isLooping_; }

  // New setters
  void setTimeSignature(int num, int den);

  // Callbacks
  std::function<void()> onPlayClicked;
  std::function<void()> onStopClicked;
  std::function<void()> onRecordClicked;
  std::function<void()> onLoopToggled;
  std::function<void()> onRewindClicked;
  std::function<void()> onMetronomeToggled;
  
  std::function<void()> onViewToggleClicked;
  std::function<void()> onSettingsClicked;
  std::function<void()> onExportClicked;
  std::function<void()> onClearAllSolos;

  // Interaction Callbacks
  std::function<void(double)> onTempoChanged;
  std::function<void(int, int)> onTimeSignatureChanged;

private:
  // Internal "Ghost" Button for Accessibility & Standard Input
  class GhostButton : public juce::Button {
  public:
      GhostButton(const juce::String& name) : juce::Button(name) {}
      void paintButton(juce::Graphics&, bool, bool) override {} // Invisible
  };

  void createButtons();
  void setupButton(GhostButton& btn, const juce::String& tooltip);

  bool isPlaying_ = false;
  bool isRecording_ = false;
  bool isLooping_ = false;
  bool isMetronomeOn_ = false;
  double tempo_ = 120.0;
  float cpuUsage_ = 0.0f;
  double position_ = 0.0;
  int timeSigNum_ = 4;
  int timeSigDen_ = 4;
  
  // Editors
  std::unique_ptr<juce::Label> bpmLabel_;
  std::unique_ptr<juce::Label> timeSigNumLabel_;
  std::unique_ptr<juce::Label> timeSigDenLabel_;
  bool editingTimeSigNum_ = false;
  
  // Interaction State
  bool isDraggingBpm_ = false;
  bool isDraggingTimeSig_ = false;
  double dragStartValue_ = 0.0;
  int dragStartNum_ = 0;
  int dragStartDen_ = 0;
  juce::Point<int> dragStartPos_;
  
  // Buttons (Accessibility + Input)
  std::unique_ptr<GhostButton> playBtn_;
  std::unique_ptr<GhostButton> stopBtn_;
  std::unique_ptr<GhostButton> recordBtn_;
  std::unique_ptr<GhostButton> loopBtn_;
  std::unique_ptr<GhostButton> rewindBtn_;
  std::unique_ptr<GhostButton> metroBtn_;
  std::unique_ptr<GhostButton> viewToggleBtn_;
  std::unique_ptr<GhostButton> settingsBtn_;
  std::unique_ptr<GhostButton> exportBtn_;

  // Layout Bounds
  juce::Rectangle<int> lcdBounds_;
  juce::Rectangle<int> cpuMeterBounds_;
  
  // Hit zones for LCD custom interaction
  juce::Rectangle<int> bpmHitBounds_;
  juce::Rectangle<int> timeSigHitBounds_;

  // Interaction states for Animation
  InteractionState playState_;
  InteractionState stopState_;
  InteractionState recordState_;
  InteractionState loopState_;
  InteractionState rewindState_;
  InteractionState metroState_;
  InteractionState viewToggleState_;
  InteractionState settingsState_;
  InteractionState exportState_;

  // Cached Text Layouts (Performance)
  struct CachedText {
      std::string text;
      float width;
      float xOffset;
  };
  CachedText cachedBpm_;
  CachedText cachedTimeSig_;
  void updateBpmCache();
  void updateTimeSigCache();

  void drawTransportButton(SkCanvas *canvas, GhostButton& btn,
                           const SkPath &iconPath, bool isActive,
                           uint32_t color, InteractionState &state);
                           
  void drawMeter(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                 float value, const char *label);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)

  // Cached resources for 60FPS rendering
  ::SkPaint bgPaint_;
  ::SkPaint borderPaint_;
  ::SkFont monoFont_;
  ::SkFont labelFont_;
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
