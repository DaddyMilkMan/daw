/**
 * @file TransportBar.h
 * @brief Top transport control bar with modern DAW styling
 *
 * Full-width bar (56-60px height) containing:
 * - Left: Project name + menu/shortcut buttons
 * - Center: Play/Stop/Rec controls, loop, tempo, time display
 * - Right: CPU/DSP meter, undo history, Wingman status indicator
 */

#pragma once

#include "SkiaCanvasComponent.h"
#include "SkiaTheme.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class TransportBar
 * @brief Modern DAW-style transport control bar with Skia rendering
 *
 * Provides comprehensive transport controls with visual feedback:
 * - Transport state (play/stop/record)
 * - Tempo and time signature
 * - Playback position display
 * - CPU/DSP usage meter
 * - Undo/redo status
 * - Wingman AI connection status
 */
class TransportBar : public SkiaCanvasComponent {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  TransportBar();
  ~TransportBar() override = default;

  //==========================================================================
  // State Management
  //==========================================================================

  void setPlaying(bool playing);
  void setRecording(bool recording);
  void setLooping(bool looping);
  void setTempo(double bpm);
  void setTimeSignature(int numerator, int denominator);
  void setPlaybackPosition(double seconds);
  void setCPUUsage(float percentage);
  void setWingmanConnected(bool connected);
  void setProjectName(const juce::String &name);

  bool isPlaying() const { return isPlaying_; }
  bool isRecording() const { return isRecording_; }
  bool isLooping() const { return isLooping_; }

  //==========================================================================
  // Callbacks
  //==========================================================================

  std::function<void()> onPlayClicked;
  std::function<void()> onStopClicked;
  std::function<void()> onRecordClicked;
  std::function<void()> onLoopClicked;
  std::function<void(double)> onTempoChanged;
  std::function<void()> onMenuClicked;
  std::function<void()> onUndoClicked;
  std::function<void()> onViewToggleClicked;

  //==========================================================================
  // Component Overrides
  //==========================================================================

  void resized() override;
  void mouseDown(const juce::MouseEvent &event) override;
  void mouseUp(const juce::MouseEvent &event) override;
  void mouseMove(const juce::MouseEvent &event) override;
  juce::String getTooltip();

protected:
  //==========================================================================
  // Skia Rendering
  //==========================================================================

  void paintSkia(SkCanvas &canvas, const juce::Rectangle<int> &bounds) override;

private:
  //==========================================================================
  // Internal Rendering Methods
  //==========================================================================

  void drawBackground(SkCanvas &canvas, const SkRect &bounds);
  void drawLeftSection(SkCanvas &canvas, const SkRect &bounds);
  void drawCenterSection(SkCanvas &canvas, const SkRect &bounds);
  void drawRightSection(SkCanvas &canvas, const SkRect &bounds);

  void drawTransportButton(SkCanvas &canvas, const SkRect &rect,
                           const juce::String &label, bool isActive,
                           bool isHovered, SkColor activeColor);
  void drawCPUMeter(SkCanvas &canvas, const SkRect &rect);
  void drawWingmanIndicator(SkCanvas &canvas, const SkRect &rect);

  juce::String formatTime(double seconds) const;

  //==========================================================================
  // Hit Testing
  //==========================================================================

  enum class HitZone {
    None,
    Play,
    Stop,
    Record,
    Loop,
    Menu,
    Undo,
    Wingman,
    ViewToggle
  };

  HitZone hitTest(const juce::Point<int> &point) const;
  juce::Rectangle<int> getPlayButtonBounds() const;
  juce::Rectangle<int> getStopButtonBounds() const;
  juce::Rectangle<int> getRecordButtonBounds() const;
  juce::Rectangle<int> getLoopButtonBounds() const;
  juce::Rectangle<int> getMenuButtonBounds() const;
  juce::Rectangle<int> getUndoButtonBounds() const;
  juce::Rectangle<int> getWingmanButtonBounds() const;
  juce::Rectangle<int> getViewToggleButtonBounds() const;

  //==========================================================================
  // State
  //==========================================================================

  bool isPlaying_ = false;
  bool isRecording_ = false;
  bool isLooping_ = false;
  double tempo_ = 120.0;
  int timeSigNumerator_ = 4;
  int timeSigDenominator_ = 4;
  double playbackPosition_ = 0.0;
  float cpuUsage_ = 0.0f;
  bool wingmanConnected_ = false;
  juce::String projectName_ = "Untitled Project";

  HitZone hoveredZone_ = HitZone::None;
  HitZone activeZone_ = HitZone::None;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace zenith
