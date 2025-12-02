#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"

namespace zenith {
class InstrumentBrowserPanel : public juce::Component {
public:
    InstrumentBrowserPanel(Engine& engine, ProjectState& state) {
        juce::ignoreUnused(engine, state);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawText("Instrument Browser (Stub)", getLocalBounds(), juce::Justification::centred, true);
    }
};
}
