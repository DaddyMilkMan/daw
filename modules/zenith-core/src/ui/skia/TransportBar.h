/*
  ==============================================================================

    TransportBar.h
    Created: 2025-11-28
    Author:  Leo "Lil Bit" Rossi

    Transport controls with Neon Noir styling.
    Refactored to use SkiaButton, SkiaKnob, and SkiaSlider.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"
#include "SkiaButton.h"
#include "SkiaKnob.h"
#include "SkiaSlider.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkRRect.h>
#include <include/core/SkColor.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class TransportBar : public SkiaComponent {
public:
    TransportBar();
    ~TransportBar() override;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    // State setters
    void setPlaying(bool playing);
    void setRecording(bool recording);
    void setTempo(double bpm);
    void setCPU(float percent) { cpuUsage_ = percent; repaint(); }
    void setPosition(double seconds) { position_ = seconds; repaint(); }
    
    void setProjectName(const juce::String& name) { projectName_ = name; repaint(); }
    void setTimeSignature(int num, int den) { timeSigNum_ = num; timeSigDen_ = den; repaint(); }

    // Callbacks
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onViewToggleClicked;

private:
    // Child Components
    std::unique_ptr<SkiaButton> playBtn_;
    std::unique_ptr<SkiaButton> stopBtn_;
    std::unique_ptr<SkiaButton> recordBtn_;
    std::unique_ptr<SkiaButton> viewToggleBtn_;
    
    std::unique_ptr<SkiaKnob> tempoKnob_;
    std::unique_ptr<SkiaSlider> masterVolSlider_;

    // State
    bool isPlaying_ = false;
    bool isRecording_ = false;
    float cpuUsage_ = 0.0f;
    double position_ = 0.0;
    juce::String projectName_ = "Zenith DAW";
    int timeSigNum_ = 4;
    int timeSigDen_ = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
