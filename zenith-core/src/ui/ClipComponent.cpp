/**
 * @file ClipComponent.cpp
 * @brief Clip component implementation
 */

#include "../../include/ui/ClipComponent.h"

//==============================================================================
ClipComponent::ClipComponent(const juce::String& id, const juce::String& tId,
                             double start, double length)
    : clipId(id)
    , trackId(tId)
    , startBeats(start)
    , lengthBeats(length)
{
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

ClipComponent::~ClipComponent()
{
}

//==============================================================================
// Component interface
//==============================================================================

void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background color
    juce::Colour clipColor = juce::Colour(0xff4a90e2);  // Blue

    if (isDragging)
        clipColor = clipColor.brighter(0.3f);
    else if (isMouseOver)
        clipColor = clipColor.brighter(0.15f);

    // Fill clip background
    g.setColour(clipColor);
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    // Border
    g.setColour(clipColor.brighter(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), 3.0f, 1.5f);

    // Clip label
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.0f));
    g.drawText(clipId, bounds.reduced(6, 2), juce::Justification::topLeft, true);

    // Duration info
    g.setFont(juce::Font(10.0f));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    juce::String durationText = juce::String(lengthBeats, 2) + " beats";
    g.drawText(durationText, bounds.reduced(6, 2), juce::Justification::bottomLeft, true);
}

void ClipComponent::resized()
{
    // Nothing to resize
}

void ClipComponent::mouseEnter(const juce::MouseEvent& e)
{
    isMouseOver = true;
    repaint();
}

void ClipComponent::mouseExit(const juce::MouseEvent& e)
{
    isMouseOver = false;
    repaint();
}

void ClipComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        isDragging = true;
        dragStartBeats = startBeats;
        dragStartX = e.getMouseDownX();
        repaint();
    }
}

void ClipComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging)
        return;

    // Calculate drag delta in pixels
    int deltaX = e.getDistanceFromDragStartX();

    // Convert to beats (we'll get pixelsPerBeat from parent)
    // For now, assume 40 pixels per beat (will be updated by parent)
    double pixelsPerBeat = 40.0;
    if (auto* parent = getParentComponent())
    {
        // The parent (track lane) should provide this
        // For now, use a fixed value
    }

    double deltaBeats = deltaX / pixelsPerBeat;
    double newStartBeats = dragStartBeats + deltaBeats;

    // Snap to grid
    if (snapEnabled)
        newStartBeats = snapToGrid(newStartBeats);

    // Don't allow negative start
    newStartBeats = juce::jmax(0.0, newStartBeats);

    // Update start position (visual only during drag)
    startBeats = newStartBeats;
    repaint();
}

void ClipComponent::mouseUp(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        isDragging = false;

        // Notify parent of the move
        if (onClipMoved)
            onClipMoved(clipId, startBeats);

        repaint();
    }
}

//==============================================================================
// Clip properties
//==============================================================================

void ClipComponent::setStartBeats(double newStart)
{
    startBeats = newStart;
    repaint();
}

void ClipComponent::setLengthBeats(double newLength)
{
    lengthBeats = newLength;
    repaint();
}

//==============================================================================
// Helper methods
//==============================================================================

double ClipComponent::snapToGrid(double beats) const
{
    if (snapGridBeats <= 0.0)
        return beats;

    return std::round(beats / snapGridBeats) * snapGridBeats;
}
