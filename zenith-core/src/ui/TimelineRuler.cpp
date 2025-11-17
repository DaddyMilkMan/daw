/**
 * @file TimelineRuler.cpp
 * @brief Timeline ruler implementation
 */

#include "../../include/ui/TimelineRuler.h"

//==============================================================================
TimelineRuler::TimelineRuler()
{
    // Start timer for playhead animation (30 fps)
    startTimer(33);
}

TimelineRuler::~TimelineRuler()
{
    stopTimer();
}

//==============================================================================
// Component interface
//==============================================================================

void TimelineRuler::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw separator line at bottom
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawLine(0.0f, (float)bounds.getBottom(), (float)bounds.getRight(), (float)bounds.getBottom(), 1.0f);

    // Calculate visible range in beats
    double startBeat = scrollOffset / pixelsPerBeat;
    double endBeat = (scrollOffset + bounds.getWidth()) / pixelsPerBeat;

    // Draw beat markers
    double beatsPerBar = timeSignatureNumerator;

    // Calculate starting bar and beat
    int startBar = (int)std::floor(startBeat / beatsPerBar);
    int endBar = (int)std::ceil(endBeat / beatsPerBar) + 1;

    for (int bar = startBar; bar <= endBar; ++bar)
    {
        for (int beat = 0; beat < timeSignatureNumerator; ++beat)
        {
            double beatPosition = (bar * beatsPerBar) + beat;
            int x = (int)((beatPosition * pixelsPerBeat) - scrollOffset);

            if (x < 0 || x > bounds.getWidth())
                continue;

            bool isBarLine = (beat == 0);

            if (isBarLine)
            {
                // Draw bar line (darker, taller)
                g.setColour(juce::Colour(0xff606060));
                g.drawLine((float)x, 0.0f, (float)x, (float)bounds.getHeight(), 2.0f);

                // Draw bar number
                g.setColour(juce::Colours::lightgrey);
                g.setFont(juce::Font(12.0f, juce::Font::bold));
                juce::String barText = juce::String(bar + 1);
                g.drawText(barText, x + 4, 2, 40, 16, juce::Justification::centredLeft);
            }
            else
            {
                // Draw beat line (lighter, shorter)
                g.setColour(juce::Colour(0xff404040));
                int lineHeight = bounds.getHeight() / 2;
                g.drawLine((float)x, (float)(bounds.getHeight() - lineHeight),
                          (float)x, (float)bounds.getHeight(), 1.0f);
            }
        }
    }

    // Draw playhead
    int playheadX = (int)((playheadBeats * pixelsPerBeat) - scrollOffset);
    if (playheadX >= 0 && playheadX <= bounds.getWidth())
    {
        g.setColour(juce::Colour(0xffff6b35));  // Orange playhead
        g.drawLine((float)playheadX, 0.0f, (float)playheadX, (float)bounds.getHeight(), 2.0f);
    }
}

void TimelineRuler::resized()
{
    // Nothing to resize
}

//==============================================================================
// Timeline control
//==============================================================================

void TimelineRuler::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat = juce::jlimit(10.0, 200.0, ppb);
    repaint();
}

void TimelineRuler::setScrollOffset(int offset)
{
    scrollOffset = juce::jmax(0, offset);
    repaint();
}

void TimelineRuler::setTimeSignature(int numerator, int denominator)
{
    timeSignatureNumerator = numerator;
    timeSignatureDenominator = denominator;
    repaint();
}

void TimelineRuler::setPlayheadPosition(double positionInBeats)
{
    playheadBeats = juce::jmax(0.0, positionInBeats);
    repaint();
}

//==============================================================================
// Timer interface
//==============================================================================

void TimelineRuler::timerCallback()
{
    // For now, just repaint to update playhead
    // In the future, this could be connected to the transport
    repaint();
}
