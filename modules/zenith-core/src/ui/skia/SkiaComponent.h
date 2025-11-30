#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declare SkCanvas
class SkCanvas;

namespace zenith {

class SkiaComponent : public juce::Component {
public:
    SkiaComponent() {}
    ~SkiaComponent() override {}

    virtual void drawSkia(::SkCanvas* canvas) {}

    void paint(juce::Graphics& g) override {
        // Default paint does nothing, subclasses should override for JUCE fallback
    }
};

}
