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
        g.fillAll(ZenithTheme::Colors::bg_00); // Dark background
        
        // Header
        g.setColour(ZenithTheme::Colors::bg_02);
        g.fillRect(0, 0, getWidth(), 30);
        
        g.setColour(ZenithTheme::Colors::text_primary);
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText("Instruments", 10, 0, getWidth() - 20, 30, juce::Justification::centredLeft, true);

        // List
        g.setFont(juce::Font(14.0f));
        int y = 40;
        
        if (instrumentIds_.isEmpty()) {
             g.setColour(ZenithTheme::Colors::text_secondary);
             g.drawText("No instruments found.", 0, 40, getWidth(), 40, juce::Justification::centred, true);
             return;
        }

        for (const auto& id : instrumentIds_) {
            // Simple hover effect could be added here if we tracked mouse
            g.setColour(ZenithTheme::Colors::text_primary);
            g.drawText(id, 20, y, getWidth() - 40, 24, juce::Justification::left, true);
            
            // Separator
            g.setColour(ZenithTheme::Colors::border_subtle);
            g.fillRect(10, y + 24, getWidth() - 20, 1);
            
            y += 28;
        }
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
