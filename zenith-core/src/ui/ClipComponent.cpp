/**
 * @file ClipComponent.cpp
 * @brief Clip component implementation
 */

#include "../../include/ui/ClipComponent.h"
#include "../../include/ProjectState.h"

ClipComponent::ClipComponent(juce::ValueTree clipNode)
    : clip(clipNode)
{
}

juce::String ClipComponent::getClipId() const
{
    return clip[ProjectState::PROP_ID].toString();
}

double ClipComponent::getStartBeats() const
{
    return clip[ProjectState::PROP_START_BEATS];
}

double ClipComponent::getLengthBeats() const
{
    return clip[ProjectState::PROP_LENGTH_BEATS];
}

void ClipComponent::updateBounds(double pixelsPerBeat, int yPosition, int height)
{
    int x = static_cast<int>(getStartBeats() * pixelsPerBeat);
    int width = static_cast<int>(getLengthBeats() * pixelsPerBeat);
    setBounds(x, yPosition, width, height);
}

void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Determine color based on clip type
    juce::Colour clipColor = juce::Colours::blue;
    if (clip[ProjectState::PROP_TYPE].toString() == "midi")
        clipColor = juce::Colours::green;

    // Fill
    g.setColour(clipColor.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(clipColor);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 2.0f);

    // Clip name/ID
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.0f));
    g.drawText(getClipId(), bounds.reduced(4), juce::Justification::centredLeft, true);
}

void ClipComponent::mouseDown(const juce::MouseEvent& event)
{
    dragStartPos = event.getPosition();
    dragStartBeats = getStartBeats();
}

void ClipComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Simple drag visualization (actual state changes would go through ProjectState)
    auto delta = event.getPosition() - dragStartPos;
    setTopLeftPosition(getX() + delta.x, getY());
}
