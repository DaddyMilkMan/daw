#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Engine.h"
#include "ProjectState.h"
#include "../instruments/InstrumentRegistry.h"
#include "../design-system/ZenithTheme.h"

namespace zenith {

class InstrumentBrowserPanel : public juce::Component {
public:
    InstrumentBrowserPanel(Engine& engine, ProjectState& state) 
        : engine_(engine) 
    {
        juce::ignoreUnused(state);
        refreshInstruments();
    }

    void paint(juce::Graphics& g) override {
        // Skia rendering used - no JUCE rendering needed
        juce::ignoreUnused(g);
    }
    
    void refreshInstruments() {
        // Fetch available instruments from the registry
        instrumentIds_ = engine_.getInstrumentRegistry().getInstrumentIds();
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        juce::ignoreUnused(e);
        // Simple click handling to refresh or select (future)
        refreshInstruments();
    }

private:
    Engine& engine_;
    juce::StringArray instrumentIds_;
};

} // namespace zenith
