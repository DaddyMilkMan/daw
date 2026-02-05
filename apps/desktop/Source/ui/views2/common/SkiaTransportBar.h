/*
  ==============================================================================

    SkiaTransportBar.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Transport controls bar - play, stop, record, loop, tempo, position.
    Fixed at top of window.

  ==============================================================================
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
    
    //==========================================================================
    // Time/Position
    //==========================================================================
    
    void setPosition(double beats);
    void setPositionSMPTE(int hours, int mins, int secs, int frames);
    
    void setTempo(float bpm);
    float getTempo() const { return tempo_; }
    
    void setTimeSignature(int numerator, int denominator);
    
    void setProjectLength(double beats);
    
    //==========================================================================
    // CPU Meter
    //==========================================================================
    
    void setCPULoad(float load);  // 0.0 - 1.0
    
    //==========================================================================
    // View Toggle
    //==========================================================================
    
    enum class ActiveView { Arrangement, Session, AIJam };
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
    std::function<void(ActiveView)> onViewChange;
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void onAnimationTick(float deltaMs) override;

private:
    //==========================================================================
    // Layout
    //==========================================================================
    
    static constexpr float kBarHeight = 48.0f;
    static constexpr float kButtonSize = 32.0f;
    static constexpr float kButtonSpacing = 8.0f;
    
    //==========================================================================
    // State
    //==========================================================================
    
    TransportState state_ = TransportState::Stopped;
    bool recordArmed_ = false;
    bool loopEnabled_ = false;
    bool metronomeEnabled_ = false;
    
    double positionBeats_ = 0.0;
    float tempo_ = 120.0f;
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
    
    void drawButton(SkCanvas* canvas, float x, float y, int buttonId,
                    const char* icon, bool isActive, bool isToggle = false);
    
    //==========================================================================
    // Hit Testing
    //==========================================================================
    
    int hitTestButton(float x, float y) const;
    bool hitTestTempo(float x, float y) const;
    int hitTestViewToggle(float x, float y) const;
    
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
        BTN_VIEW_JAM
    };
    
    float getButtonX(int buttonId) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTransportBar)
};

} // namespace zenith::ui
