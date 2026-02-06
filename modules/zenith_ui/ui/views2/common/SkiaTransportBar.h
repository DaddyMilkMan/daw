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

#include "../../framework/SkiaComponent.h"
#include "../../design-system/ZenithTheme.h"
#include <functional>

namespace zenith::ui {

/**
 * @brief Transport playback state
 */
enum class TransportState {
    Stopped,
    Playing,
    Paused,
    Recording
};

/**
 * @class SkiaTransportBar
 * @brief Main transport controls bar
 */
class SkiaTransportBar : public SkiaComponent {
public:
    SkiaTransportBar();
    ~SkiaTransportBar() override = default;

    //==========================================================================
    // Transport State
    //==========================================================================
    
    void setState(TransportState state);
    TransportState getState() const { return state_; }
    
    void setRecordArmed(bool armed);
    bool isRecordArmed() const { return recordArmed_; }
    
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled_; }
    
    void setMetronomeEnabled(bool enabled);
    bool isMetronomeEnabled() const { return metronomeEnabled_; }

    void setWingmanActive(bool active);
    
    //==========================================================================
    // Time/Position
    //==========================================================================
    
    void setPosition(double beats);
    void setPositionSMPTE(int hours, int mins, int secs, int frames);
    
    void setTempo(float bpm);
    float getTempo() const { return tempo_; }
    
    void setTimeSignature(int numerator, int denominator);
    void setUpdateAvailable(bool available);
    
    void setProjectLength(double beats);
    
    //==========================================================================
    // CPU Meter
    //==========================================================================
    
    void setCPULoad(float load);  // 0.0 - 1.0
    
    //==========================================================================
    // View Toggle
    //==========================================================================
    
    enum class ActiveView { Arrangement, Session };
    void setActiveView(ActiveView view);
    ActiveView getActiveView() const { return activeView_; }
    
    //==========================================================================
    // Callbacks
    //==========================================================================
    
    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void()> onPause;
    std::function<void()> onRecord;
    std::function<void()> onRewind;
    std::function<void()> onFastForward;
    std::function<void()> onLoopToggle;
    std::function<void()> onMetronomeToggle;
    std::function<void(float)> onTempoChange;
    std::function<void(int, int)> onTimeSignatureChange;
    std::function<void(ActiveView)> onViewChange;
    std::function<void()> onSettings;
    std::function<void()> onWingman;
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void onAnimationTick(float deltaMs) override;

private:
    //==========================================================================
    // Layout
    //==========================================================================
    
    static constexpr float kBarHeight = 48.0f;
    static constexpr float kButtonSize = 32.0f;
    static constexpr float kButtonSpacing = 8.0f;
    static constexpr float kRightPadding = 16.0f;
    static constexpr float kTempoX = 280.0f;
    static constexpr float kTempoWidth = 70.0f;
    static constexpr float kTempoSectionGap = 105.0f;
    static constexpr float kTimeSigWidth = 60.0f;
    static constexpr float kControlHeight = 22.0f;
    static constexpr float kTimeSigDragStep = 6.0f;
    
    //==========================================================================
    // State
    //==========================================================================
    
    TransportState state_ = TransportState::Stopped;
    bool recordArmed_ = false;
    bool loopEnabled_ = false;
    bool metronomeEnabled_ = false;
    bool wingmanActive_ = false;
    bool updateAvailable_ = false;
    
    double positionBeats_ = 0.0;
    float tempo_ = 120.0f;
    float frameRate_ = 30.0f; // SMPTE frame rate (default 30 fps)
    int timeSigNum_ = 4;
    int timeSigDenom_ = 4;
    double projectLength_ = 128.0;
    
    ActiveView activeView_ = ActiveView::Arrangement;
    
    // CPU Load
    float cpuLoad_ = 0.0f;
    
    // Animation
    float recordPulse_ = 0.0f;
    
    // Interaction
    int hoveredButton_ = -1;
    bool isDraggingTempo_ = false;
    float tempoDragStartY_ = 0.0f;
    float tempoDragStartBPM_ = 0.0f;
    bool isDraggingTimeSig_ = false;
    bool draggingTimeSigNumerator_ = true;
    float timeSigDragStartY_ = 0.0f;
    int timeSigDragStartValue_ = 4;
    int pendingTimeSigEdit_ = -1;
    juce::Point<float> timeSigMouseDownPos_{};
    bool timeSigHover_ = false;

    std::unique_ptr<juce::Label> timeSigNumEditor_;
    std::unique_ptr<juce::Label> timeSigDenEditor_;
    
    //==========================================================================
    // Drawing
    //==========================================================================
    
    void drawBackground(SkCanvas* canvas);
    void drawTransportButtons(SkCanvas* canvas);
    void drawTempoSection(SkCanvas* canvas);
    void drawTimeDisplay(SkCanvas* canvas);
    void drawPositionSlider(SkCanvas* canvas);
    void drawViewToggle(SkCanvas* canvas);
    void drawCPUMeter(SkCanvas* canvas);
    void drawUtilityButtons(SkCanvas* canvas);
    
    void drawButton(SkCanvas* canvas, float x, float y, int buttonId,
                    const char* icon, bool isActive, bool isToggle = false);
    
    //==========================================================================
    // Hit Testing
    //==========================================================================
    
    int hitTestButton(float x, float y) const;
    bool hitTestTempo(float x, float y) const;
    int hitTestViewToggle(float x, float y) const;
    int hitTestUtilityButton(float x, float y) const;
    int hitTestTimeSignature(float x, float y) const;
    void beginTimeSigEdit(bool numerator);
    void endTimeSigEdit();
    void updateTimeSigEditorBounds();
    static int sanitizeDenominator(int denominator);
    
    // Button IDs
    enum ButtonId {
        BTN_REWIND = 0,
        BTN_STOP,
        BTN_PLAY,
        BTN_RECORD,
        BTN_LOOP,
        BTN_METRONOME,
        BTN_VIEW_ARR,
        BTN_VIEW_SES,
        BTN_VIEW_JAM,
        BTN_WINGMAN,
        BTN_SETTINGS
    };
    
    float getButtonX(int buttonId) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTransportBar)
};

} // namespace zenith::ui
