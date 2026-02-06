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

#include "../design-system/InteractionHelper.h"
#include "SkiaComponent.h"
#include "../../network/UpdateService.h" // Added UpdateService
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include "../design-system/SvgIcon.h"
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
  void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;
  
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  bool hitTest(int x, int y) override;

  void onAnimationTick(float deltaMs) override;
  void visibilityChanged() override;

  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

  bool isUpdateAvailable() const { return isUpdateAvailable_; }
  
  // State setters
  void setPlaying(bool playing) {
    isPlaying_ = playing;
    requestRepaint();
  }
  void setRecording(bool recording) {
    isRecording_ = recording;
    requestRepaint();
  }
  void setTempo(double bpm) {
    tempo_ = bpm;
    requestRepaint();
  }
  void setCPU(float percent) {
    cpuUsage_ = juce::jlimit(0.0f, 100.0f, percent);
    // Repaint only the CPU meter area when possible to reduce flicker and improve performance
    if (cpuMeterBounds_.getWidth() > 0 && cpuMeterBounds_.getHeight() > 0) {
        repaint(cpuMeterBounds_.getX(), cpuMeterBounds_.getY(), cpuMeterBounds_.getWidth(), cpuMeterBounds_.getHeight());
    } else {
        requestRepaint();
    }
  }
  void setPosition(double seconds) {
    position_ = seconds;
    requestRepaint();
  }
  
  bool isPlaying() const { return isPlaying_; }
  bool isRecording() const { return isRecording_; }

  // Loop state
  void setLooping(bool looping) {
    isLooping_ = looping;
    requestRepaint();
  }
  bool isLooping() const { return isLooping_; }
  void setLoopPosition(double startSeconds, double endSeconds) {
    loopStart_ = startSeconds;
    loopEnd_ = endSeconds;
  }

  // New setters
  void setTimeSignature(int num, int den) {
    timeSigNum_ = num;
    timeSigDen_ = den;
    requestRepaint();
  }

  // Callbacks
  std::function<void()> onPlayClicked;
  std::function<void()> onStopClicked;
  std::function<void()> onRecordClicked;
  std::function<void()> onLoopToggled;
  std::function<void()> onRewind;
  std::function<void()> onViewToggleClicked;
  std::function<void()> onSettingsClicked;
  std::function<void()> onUpdateAvailable;
  std::function<void()> onClearAllSolos;
  std::function<void()> onWingmanClicked;  // AI Assistant button

  // Interaction Callbacks
  std::function<void(double)> onTempoChanged;
  std::function<void(int, int)> onTimeSignatureChanged;

private:
  bool isPlaying_ = false;
  bool isRecording_ = false;
  bool isLooping_ = false;      // Loop enable state
  double loopStart_ = 0.0;      // Loop start position in seconds
  double loopEnd_ = 8.0;        // Loop end position in seconds
  double tempo_ = 120.0;
  float cpuUsage_ = 0.0f;
  float smoothedCpu_ = 0.0f; // FIX: Smoothed value for display
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
  bool isDraggingTimeSigNum_ = false;   // Dragging numerator
  bool isDraggingTimeSigDen_ = false;   // Dragging denominator
  double dragStartValue_ = 0.0;
  int dragStartNum_ = 0;
  int dragStartDen_ = 0;
  juce::Point<int> dragStartPos_;
  
  // Sub-bounds for hit testing
  juce::Rectangle<int> bpmHitBounds_;
  juce::Rectangle<int> timeSigHitBounds_;      // Overall area
  juce::Rectangle<int> timeSigNumBounds_;      // Numerator zone
  juce::Rectangle<int> timeSigDenBounds_;      // Denominator zone
  juce::Rectangle<int> timeSigTemplateBounds_; // Templates dropdown button

  juce::Rectangle<int> playButtonBounds_;
  juce::Rectangle<int> stopButtonBounds_;
  juce::Rectangle<int> recordButtonBounds_;
  juce::Rectangle<int> loopButtonBounds_;       // Loop toggle button
  juce::Rectangle<int> viewToggleButtonBounds_;
  juce::Rectangle<int> wingmanButtonBounds_;   // AI Assistant button
  juce::Rectangle<int> settingsButtonBounds_;


  // Update Service
  std::unique_ptr<zenith::network::UpdateService> updateService_;
  bool isUpdateAvailable_ = false;

  // Dynamic layout bounds
  juce::Rectangle<int> lcdBounds_;
  juce::Rectangle<int> cpuMeterBounds_;

  // Interaction states
  InteractionState playState_;
  InteractionState stopState_;
  InteractionState recordState_;
  InteractionState loopState_;      // Loop toggle button state
  InteractionState viewToggleState_;
  InteractionState wingmanState_;   // AI Assistant button
  InteractionState settingsState_;



  void drawTransportButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                           const SkPath &iconPath, bool isActive,
                           uint32_t color, const InteractionState &state,
                           bool isFilled = true);
  void drawTransportSvgButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                              svgicons::IconId iconId, bool isActive,
                              uint32_t color, const InteractionState &state,
                              bool isFilled = true, float iconScale = 0.48f);
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
  
  // Force full window repaint to prevent Linux compositing artifacts
  // Also explicitly marks sibling TitleBar as needing repaint
  void requestRepaint() {
    // If we have a valid CPU meter bounds, repaint that sub-rect only (fast path)
    if (cpuMeterBounds_.getWidth() > 0 && cpuMeterBounds_.getHeight() > 0) {
        repaint(cpuMeterBounds_.getX(), cpuMeterBounds_.getY(),
                cpuMeterBounds_.getWidth(), cpuMeterBounds_.getHeight());
    } else {
        // Fallback to repainting only this component
        repaint();
    }
}
};

#else // ZENITH_USE_SKIA

class TransportBar : public juce::Component {
public:
    TransportBar() {}
    ~TransportBar() override = default;
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
