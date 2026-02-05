/*
  ==============================================================================

    ZenithPolySynthUI_WingmanIntegration.cpp
    Created: 2025-01-29
    Author:  Zenith DAW

    UI integration with WingmanSynthBridge for real-time knob animation.
    Add this to ZenithPolySynthUI.cpp to make UI a Wingman listener.

  ==============================================================================*/

// Add this include to ZenithPolySynthUI.cpp:
// #include "../ai/WingmanSynthBridge.h"

namespace zenith {

//==============================================================================
// Add to ZenithPolySynthUI constructor:
/*

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor& p)
    : processor(p)
{
    // Register as Wingman listener for real-time parameter animation
    if (auto* bridge = processor.getWingmanBridge()) {
        bridge->addListener(this);
    }
    
    // ... rest of constructor
}

*/

//==============================================================================
// Add to ZenithPolySynthUI destructor:
/*

ZenithPolySynthUI::~ZenithPolySynthUI()
{
    // Unregister from Wingman
    if (auto* bridge = processor.getWingmanBridge()) {
        bridge->removeListener(this);
    }
    
    // ... rest of destructor
}

*/

//==============================================================================
// Implement WingmanSynthListener interface
// Add these methods to ZenithPolySynthUI class:

void ZenithPolySynthUI::wingmanParameterChanged(const WingmanParameterChange& change) {
    // Called when Wingman changes a parameter
    // Animate the corresponding UI control
    
    // Find the widget for this parameter
    // This depends on your widget naming/ID system
    for (auto& widget : widgets_) {
        if (widget->paramId == change.parameterId) {
            // Animate to new value
            animateWidgetToValue(widget.get(), change.newValue, change.animationSpeed);
            
            // Update display text if available
            if (change.displayValue.isNotEmpty()) {
                widget->displayValue = change.displayValue;
            }
            
            // Trigger repaint
            repaint();
            break;
        }
    }
}

//==============================================================================

void ZenithPolySynthUI::wingmanBatchStart() {
    // Wingman is about to change multiple parameters
    // Optional: Disable repaints during batch for performance
    setBufferedToImage(true);
}

//==============================================================================

void ZenithPolySynthUI::wingmanBatchEnd() {
    // Wingman finished changing parameters
    // Re-enable normal repainting
    setBufferedToImage(false);
    repaint();
}

//==============================================================================

void ZenithPolySynthUI::wingmanSoundGenerated(const juce::String& description) {
    // Wingman generated a new sound/preset
    // Show notification to user
    
    // You could display this in a status bar or popup
    DBG("Wingman generated: " + description);
    
    // Trigger visual feedback
    if (visualizer_) {
        visualizer_->flashFeedback(juce::Colours::green.withAlpha(0.3f));
    }
}

//==============================================================================
// Helper: Animate widget to new value
// Add this private method to ZenithPolySynthUI class:

void ZenithPolySynthUI::animateWidgetToValue(SkiaWidget* widget, float targetValue, float speed) {
    if (!widget) return;
    
    // Store animation state
    struct WidgetAnimation {
        SkiaWidget* widget;
        float startValue;
        float targetValue;
        float progress;
        float speed;
        double startTime;
    };
    
    // Create animation
    WidgetAnimation anim;
    anim.widget = widget;
    anim.startValue = widget->currentValue;
    anim.targetValue = targetValue;
    anim.progress = 0.0f;
    anim.speed = speed;
    anim.startTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    
    // Add to active animations
    activeAnimations_.add(anim);
    
    // Start timer if not already running
    if (!isTimerRunning()) {
        startTimerHz(60); // 60 FPS animation
    }
}

//==============================================================================
// Add to timerCallback():
/*

void ZenithPolySynthUI::timerCallback() {
    // Update animations
    double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    
    for (int i = activeAnimations_.size(); --i >= 0;) {
        auto& anim = activeAnimations_.getReference(i);
        
        // Calculate progress based on speed
        anim.progress += 0.016 * anim.speed; // Assume ~60fps
        
        if (anim.progress >= 1.0f) {
            // Animation complete
            anim.widget->currentValue = anim.targetValue;
            activeAnimations_.remove(i);
        } else {
            // Interpolate current value
            float t = anim.progress;
            
            // Ease-out interpolation for smooth feel
            t = 1.0f - std::pow(1.0f - t, 3.0f);
            
            anim.widget->currentValue = anim.startValue + (anim.targetValue - anim.startValue) * t;
        }
    }
    
    // Stop timer if no animations
    if (activeAnimations_.isEmpty()) {
        stopTimer();
    }
    
    // Repaint to show updated values
    repaint();
    
    // ... existing timer callback code
}

*/

//==============================================================================
// Add member to ZenithPolySynthUI.h:
/*

private:
    struct WidgetAnimation {
        SkiaWidget* widget;
        float startValue;
        float targetValue;
        float progress;
        float speed;
        double startTime;
    };
    
    juce::Array<WidgetAnimation> activeAnimations_;
    
    void animateWidgetToValue(SkiaWidget* widget, float targetValue, float speed);
*/

//==============================================================================
// Modify class declaration to inherit from WingmanSynthListener:
/*

class ZenithPolySynthUI : public juce::AudioProcessorEditor,
                          public juce::ChangeListener,
                          public juce::Timer,
                          public WingmanSynthListener  // <-- Add this
{
public:
    // ... existing declarations
    
    // WingmanSynthListener interface
    void wingmanParameterChanged(const WingmanParameterChange& change) override;
    void wingmanBatchStart() override;
    void wingmanBatchEnd() override;
    void wingmanSoundGenerated(const juce::String& description) override;
    
    // ... rest of class
};

*/

//==============================================================================
// Add to SkiaWidget structure (if not already present):
/*

struct SkiaWidget {
    juce::String paramId;
    juce::String displayValue;
    float currentValue;
    
    // ... existing members
};

*/

//==============================================================================
// Example usage flow:
/*

1. User says: "Wingman, make a dark bass"

2. Wingman calls command API:
   CommandAPI.executeCommand({
       "command": "set_synth_filter_cutoff",
       "params": {"cutoff": 800}
   })

3. CommandAPI calls WingmanSynthBridge.setFilterCutoff(800)

4. WingmanSynthBridge:
   - Sets parameter in audio processor
   - Notifies listeners (UI)

5. ZenithPolySynthUI.wingmanParameterChanged() is called:
   - Finds filter cutoff knob widget
   - Starts smooth animation to 800 Hz
   - Knob visually moves over 300ms

6. User sees knob smoothly animate to new value

Result: Magical, real-time feedback that no competitor has!

*/

} // namespace zenith