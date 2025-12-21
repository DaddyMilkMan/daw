/*
  ==============================================================================

    TransportBar.h
    Created: 2025-11-28
    Updated: 2025-12-08 - Major UI Overhaul
    Author:  Leo "Lil Bit" Rossi, UI Overhaul Team

    Transport controls with modern microinteractions and spring physics.
    Inspired by Ableton Live and FL Studio transport design.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"
#include "ZenithAnimation.h"
#include "../ZenithTypography.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkFont.h>
#include <core/SkRRect.h>
#include <core/SkColor.h>
#include <core/SkTextBlob.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * Modern transport bar with spring-animated buttons and smooth interactions.
 * 
 * Features:
 * - Spring physics on button hover/press
 * - Smooth glow animations for active states
 * - Animated play/record pulse effects
 * - High-quality typography with mono font for time display
 */
class TransportBar : public SkiaComponent {
public:
    TransportBar();
    ~TransportBar() override = default;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    // State setters
    void setPlaying(bool playing);
    void setRecording(bool recording);
    void setTempo(double bpm);
    void setCPU(float percent);
    void setPosition(double seconds);
    void setProjectName(const juce::String& name);
    void setTimeSignature(int num, int den);

    // Callbacks
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onViewToggleClicked;
    std::function<void()> onSettingsClicked;

protected:
    void timerCallback() override;

private:
    // ========================================================================
    // BUTTON STATE
    // ========================================================================
    
    struct ButtonState {
        juce::Rectangle<float> bounds;
        juce::String label;
        SkColor activeColor = 0xFF00FF64;
        bool isActive = false;
        
        // Animation state
        animation::Spring hoverScale{1.0f, animation::Spring::Config::snappy()};
        animation::Spring pressScale{1.0f, animation::Spring::Config::instant()};
        animation::Spring glowIntensity{0.0f, animation::Spring::Config::smooth()};
        
        bool isHovered = false;
        bool isPressed = false;
        
        void updateAnimations(float deltaSeconds) {
            hoverScale.update(deltaSeconds);
            pressScale.update(deltaSeconds);
            glowIntensity.update(deltaSeconds);
        }
        
        float getScale() const {
            return hoverScale.getValue() * pressScale.getValue();
        }
        
        bool isAnimating() const {
            return hoverScale.isAnimating() || 
                   pressScale.isAnimating() || 
                   glowIntensity.isAnimating();
        }
    };
    
    ButtonState playButton_;
    ButtonState stopButton_;
    ButtonState recordButton_;
    ButtonState viewToggleButton_;
    ButtonState settingsButton_;
    
    // ========================================================================
    // TRANSPORT STATE
    // ========================================================================
    
    bool isPlaying_ = false;
    bool isRecording_ = false;
    double tempo_ = 120.0;
    float cpuUsage_ = 0.0f;
    double position_ = 0.0;
    juce::String projectName_ = "Zenith DAW";
    int timeSigNum_ = 4;
    int timeSigDen_ = 4;
    
    // Animation
    animation::Spring playPulse_{0.0f, animation::Spring::Config::bouncy()};
    animation::Spring recordPulse_{0.0f, animation::Spring::Config::bouncy()};
    float pulsePhase_ = 0.0f;  // For continuous pulse animation
    
    // ========================================================================
    // RENDERING
    // ========================================================================
    
    void initializeButtons();
    void updateButtonBounds();
    
    void drawBackground(SkCanvas* canvas);
    void drawTransportButton(SkCanvas* canvas, ButtonState& button);
    void drawTimeDisplay(SkCanvas* canvas);
    void drawTempoDisplay(SkCanvas* canvas);
    void drawCPUMeter(SkCanvas* canvas);
    void drawProjectName(SkCanvas* canvas);
    
    ButtonState* findButtonAt(const juce::Point<int>& pos);
    void updateHoverState(const juce::Point<int>& pos);
    
    // Cached paints
    SkPaint bgPaint_;
    SkPaint glowPaint_;
    SkFont transportFont_;
    SkFont labelFont_;
    bool paintsInitialized_ = false;
    
    void initializePaints();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith

