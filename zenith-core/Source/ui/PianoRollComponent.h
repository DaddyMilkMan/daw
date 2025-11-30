#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class PianoRollComponent : public juce::Component {
public:
    PianoRollComponent() {}
    void paint(juce::Graphics& g) override {
        // Background
        g.fillAll(juce::Colour(0xFF202020));
        
        // Grid
        g.setColour(juce::Colour(0xFF303030));
        for (int i=0; i<getWidth(); i+=40) g.drawVerticalLine(i, 0, (float)getHeight());
        for (int i=0; i<getHeight(); i+=20) g.drawHorizontalLine(i, 0, (float)getWidth());

        // Simulated Notes with Velocity Coloring (Visualist Request)
        // Note 1: High Velocity (Bright Red)
        g.setColour(juce::Colours::red.withAlpha(1.0f)); 
        g.fillRect(40, 100, 80, 18);
        
        // Note 2: Medium Velocity (Medium Red)
        g.setColour(juce::Colours::red.withAlpha(0.6f)); 
        g.fillRect(120, 140, 80, 18);
        
        // Note 3: Low Velocity (Dark/Transparent Red)
        g.setColour(juce::Colours::red.withAlpha(0.3f)); 
        g.fillRect(200, 120, 80, 18);
        
        g.setColour(juce::Colours::white);
        g.drawText("Piano Roll (Velocity Mode)", 10, 10, 200, 20, juce::Justification::left, true);
    }
};
