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

#include <JuceHeader.h>
#include "SkiaComponent.h"

#ifdef ZENITH_USE_SKIA
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkRRect.h>
#include <skia/include/core/SkColor.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class TransportBar : public SkiaComponent {
public:
    TransportBar();
    ~TransportBar() override = default;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // State setters
    void setPlaying(bool playing) { isPlaying_ = playing; repaint(); }
    void setRecording(bool recording) { isRecording_ = recording; repaint(); }
    void setTempo(double bpm) { tempo_ = bpm; repaint(); }
    void setCPU(float percent) { cpuUsage_ = percent; repaint(); }
    void setPosition(double seconds) { position_ = seconds; repaint(); }
    
    // New setters
    void setProjectName(const juce::String& name) { projectName_ = name; repaint(); }
    void setTimeSignature(int num, int den) { timeSigNum_ = num; timeSigDen_ = den; repaint(); }

    // Callbacks
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onViewToggleClicked;

private:
    bool isPlaying_ = false;
    bool isRecording_ = false;
    double tempo_ = 120.0;
    float cpuUsage_ = 0.0f;
    double position_ = 0.0;
    juce::String projectName_ = "Zenith DAW";
    int timeSigNum_ = 4;
    int timeSigDen_ = 4;

    juce::Rectangle<int> playButtonBounds_;
    juce::Rectangle<int> stopButtonBounds_;
    juce::Rectangle<int> recordButtonBounds_;
    juce::Rectangle<int> viewToggleButtonBounds_;

    void drawButton(SkCanvas* canvas, const juce::Rectangle<int>& bounds, 
                    const char* label, bool isActive, uint32_t color);
    void drawMeter(SkCanvas* canvas, const juce::Rectangle<int>& bounds,
                   float value, const char* label);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
