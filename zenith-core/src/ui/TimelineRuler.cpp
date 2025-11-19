/**
 * @file TimelineRuler.cpp
 * @brief Timeline ruler implementation
 */

#include "../../include/ui/TimelineRuler.h"

TimelineRuler::TimelineRuler()
{
    setSize(800, 30);
}

void TimelineRuler::setVisibleRange(double start, double length)
{
    viewStartBeat = start;
    viewLengthBeats = length;
    repaint();
}

int TimelineRuler::beatsToPixels(double beats) const
{
    return static_cast<int>((beats - viewStartBeat) * pixelsPerBeat);
}

double TimelineRuler::pixelsToBeats(int pixels) const
{
    return viewStartBeat + (pixels / pixelsPerBeat);
}

void TimelineRuler::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawLine(0, bounds.getBottom() - 1.0f, static_cast<float>(bounds.getWidth()),
               bounds.getBottom() - 1.0f, 1.0f);

    // Beat markers
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.0f));

    int startBeat = static_cast<int>(std::floor(viewStartBeat));
    int endBeat = static_cast<int>(std::ceil(viewStartBeat + viewLengthBeats));

    for (int beat = startBeat; beat <= endBeat; ++beat)
    {
        int x = beatsToPixels(beat);

        if (x < 0 || x > bounds.getWidth())
            continue;

        // Draw beat number every 4 beats (measure markers)
        if (beat % 4 == 0)
        {
            g.setColour(juce::Colours::white);
            g.drawLine(static_cast<float>(x), 0, static_cast<float>(x),
                      static_cast<float>(bounds.getHeight()), 2.0f);

            juce::String text = juce::String(beat / 4 + 1); // Measure number
            g.drawText(text, x + 4, 2, 40, 20, juce::Justification::centredLeft);
        }
        else
        {
            // Sub-beat markers
            g.setColour(juce::Colour(0xff606060));
            g.drawLine(static_cast<float>(x), bounds.getHeight() - 10.0f,
                      static_cast<float>(x), static_cast<float>(bounds.getHeight()), 1.0f);
        }
    }
}

void TimelineRuler::resized()
{
    // Calculate pixels per beat based on width
    if (viewLengthBeats > 0)
        pixelsPerBeat = getWidth() / viewLengthBeats;
}
