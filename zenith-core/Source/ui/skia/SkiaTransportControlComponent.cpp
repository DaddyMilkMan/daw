/**
 * @file SkiaTransportControlComponent.cpp
 * @brief Implementation of Skia transport control component
 */

#include "SkiaTransportControlComponent.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaTransportControlComponent::SkiaTransportControlComponent()
    : isPlaying_(false), isRecording_(false), isLooping_(false), tempo_(120.0f), timelinePosition_(0.0)
{
    setSize(300, 60);
}

void SkiaTransportControlComponent::setIsPlaying(bool playing)
{
    if (isPlaying_ != playing)
    {
        isPlaying_ = playing;
        repaint();
    }
}

void SkiaTransportControlComponent::setIsRecording(bool recording)
{
    if (isRecording_ != recording)
    {
        isRecording_ = recording;
        repaint();
    }
}

void SkiaTransportControlComponent::setTempo(float bpm)
{
    if (tempo_ != bpm)
    {
        tempo_ = bpm;
        repaint();
    }
}

void SkiaTransportControlComponent::setTimelinePosition(double seconds)
{
    if (timelinePosition_ != seconds)
    {
        timelinePosition_ = seconds;
        repaint();
    }
}

void SkiaTransportControlComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    // Draw play button
    juce::Rectangle<int> playButtonBounds(10, 10, 40, 40);
    g.setColour(isPlaying_ ? juce::Colours::green : juce::Colours::grey);
    g.fillRect(playButtonBounds);
    g.setColour(juce::Colours::white);
    g.drawRect(playButtonBounds, 1);
    g.drawText("Play", playButtonBounds, juce::Justification::centred);

    // Draw stop button
    juce::Rectangle<int> stopButtonBounds(55, 10, 40, 40);
    g.setColour(juce::Colours::grey);
    g.fillRect(stopButtonBounds);
    g.setColour(juce::Colours::white);
    g.drawRect(stopButtonBounds, 1);
    g.drawText("Stop", stopButtonBounds, juce::Justification::centred);

    // Draw record button
    juce::Rectangle<int> recordButtonBounds(100, 10, 40, 40);
    g.setColour(isRecording_ ? juce::Colours::red : juce::Colours::grey);
    g.fillRect(recordButtonBounds);
    g.setColour(juce::Colours::white);
    g.drawRect(recordButtonBounds, 1);
    g.drawText("Rec", recordButtonBounds, juce::Justification::centred);

    // Draw tempo display
    g.setColour(juce::Colours::white);
    g.drawText(juce::String(tempo_, 1) + " BPM", 150, 10, 100, 20,
               juce::Justification::left);

    // Draw timeline position
    g.drawText(juce::String(timelinePosition_, 2) + "s", 150, 35, 100, 20,
               juce::Justification::left);
}

void SkiaTransportControlComponent::resized()
{
    // Layout components
}

void SkiaTransportControlComponent::mouseDown(const juce::MouseEvent& event)
{
    // Handle button clicks
    if (event.x < 50)
    {
        setIsPlaying(!isPlaying_);
    }
    else if (event.x < 95)
    {
        setIsPlaying(false);
    }
    else if (event.x < 140)
    {
        setIsRecording(!isRecording_);
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
