/**
 * @file SkiaPianoRollComponent.cpp  
 * @brief Logic Pro style piano roll implementation
 */

#include "SkiaPianoRollComponent.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaPianoRollComponent::SkiaPianoRollComponent()
{
    setSize(800, 400);
}

void SkiaPianoRollComponent::addNote(const MidiNote& note)
{
    notes_.push_back(note);
    repaint();
}

void SkiaPianoRollComponent::clearNotes()
{
    notes_.clear();
    repaint();
}

void SkiaPianoRollComponent::setPlayheadPosition(float beats)
{
    playheadPosition_ = beats;
    repaint();
}

void SkiaPianoRollComponent::setZoom(float pixelsPerBeat)
{
    pixelsPerBeat_ = pixelsPerBeat;
    repaint();
}

void SkiaPianoRollComponent::setScrollPosition(float beats)
{
    scrollPosition_ = beats;
    repaint();
}

void SkiaPianoRollComponent::setKeyRange(int lowestKey, int highestKey)
{
    lowestKey_ = lowestKey;
    highestKey_ = highestKey;
    repaint();
}

bool SkiaPianoRollComponent::isBlackKey(int pitch) const
{
    int note = pitch % 12;
    return (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
}

juce::String SkiaPianoRollComponent::getNoteName(int pitch) const
{
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (pitch / 12) - 1;
    return juce::String(noteNames[pitch % 12]) + juce::String(octave);
}

void SkiaPianoRollComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Background (#1E1E1E - Logic Pro piano roll)
    g.fillAll(juce::Colour(0xff1E1E1E));
    
    // Split into keyboard and note area
    auto keyboardBounds = bounds.removeFromLeft(keyboardWidth_);
    auto noteAreaBounds = bounds;
    
    drawPianoKeys(g, keyboardBounds);
    drawGrid(g, noteAreaBounds);
    drawNotes(g, noteAreaBounds);
    drawPlayhead(g, noteAreaBounds);
}

void SkiaPianoRollComponent::drawPianoKeys(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    int numKeys = highestKey_ - lowestKey_ + 1;
    
    for (int i = 0; i < numKeys; ++i)
    {
        int pitch = highestKey_ - i;  // Draw from top to bottom
        int y = i * keyHeight_;
        
        juce::Rectangle<int> keyBounds(bounds.getX(), bounds.getY() + y, 
                                       bounds.getWidth(), keyHeight_);
        
        if (isBlackKey(pitch))
        {
            // Black keys (#111111 - Logic Pro)
            g.setColour(juce::Colour(0xff111111));
            g.fillRect(keyBounds.reduced(0, 1));
        }
        else
        {
            // White keys (#444444 - Logic Pro)
            g.setColour(juce::Colour(0xff444444));
            g.fillRect(keyBounds.reduced(0, 1));
            
            // Note name for C notes
            if (pitch % 12 == 0)
            {
                g.setColour(juce::Colour(0xffAAAAAA));
                g.setFont(juce::Font(10.0f));
                g.drawText(getNoteName(pitch), keyBounds.reduced(4, 0), 
                          juce::Justification::centredRight);
            }
        }
        
        // Separator
        g.setColour(juce::Colour(0xff000000));
        g.drawLine(keyBounds.getX(), keyBounds.getBottom(), 
                  keyBounds.getRight(), keyBounds.getBottom(), 1.0f);
    }
}

void SkiaPianoRollComponent::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    int numKeys = highestKey_ - lowestKey_ + 1;
    
    // Horizontal lines (per key)
    g.setColour(juce::Colour(0xff2A2A2A));  // Fine grid
    for (int i = 0; i <= numKeys; ++i)
    {
        int y = bounds.getY() + (i * keyHeight_);
        g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
    }
    
    // Vertical lines (beats)
    float visibleStart = scrollPosition_;
    float visibleEnd = scrollPosition_ + (bounds.getWidth() / pixelsPerBeat_);
    
    g.setColour(juce::Colour(0xff333333));
    for (float beat = std::floor(visibleStart); beat <=visibleEnd; beat += 1.0f)
    {
        float x = bounds.getX() + ((beat - scrollPosition_) * pixelsPerBeat_);
        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        }
    }
    
    // Bar lines (every 4 beats for 4/4)
    g.setColour(juce::Colour(0xff555555));
    for (float beat = std::floor(visibleStart / 4.0f) * 4.0f; beat <= visibleEnd; beat += 4.0f)
    {
        float x = bounds.getX() + ((beat - scrollPosition_) * pixelsPerBeat_);
        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        }
    }
}

void SkiaPianoRollComponent::drawNotes(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    for (const auto& note : notes_)
    {
        // Calculate position
        float noteX = bounds.getX() + ((note.startBeat - scrollPosition_) * pixelsPerBeat_);
        float noteWidth = note.duration * pixelsPerBeat_;
        
        // Skip if not visible
        if (noteX + noteWidth < bounds.getX() || noteX > bounds.getRight())
            continue;
        
        int keyIndex = highestKey_ - note.pitch;
        int noteY = bounds.getY() + (keyIndex * keyHeight_);
        
        juce::Rectangle<float> noteBounds(noteX, noteY + 1, noteWidth, keyHeight_ - 2);
        
        // Note color based on velocity (hue shift from green to red)
        float velocityNormalized = note.velocity / 127.0f;
        juce::Colour noteColor = juce::Colour::fromHSV(
            0.33f * (1.0f - velocityNormalized),  // Green (0.33) to Red (0.0)
            0.7f,
            0.8f,
            1.0f
        );
        
        if (note.isSelected)
        {
            // Selected: bright and highlighted
            g.setColour(noteColor.brighter(0.3f));
        }
        else
        {
            g.setColour(noteColor);
        }
        
        // Note with 2px rounded corners
        g.fillRoundedRectangle(noteBounds, 2.0f);
        
        // Border (1px, slightly darker)
        g.setColour(noteColor.darker(0.3f));
        g.drawRoundedRectangle(noteBounds, 2.0f, 1.0f);
        
        // Selected notes get white border
        if (note.isSelected)
        {
            g.setColour(juce::Colour(0xffFFFFFF));
            g.drawRoundedRectangle(noteBounds, 2.0f, 1.5f);
        }
    }
}

void SkiaPianoRollComponent::drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    float playheadX = bounds.getX() + ((playheadPosition_ - scrollPosition_) * pixelsPerBeat_);
    
    // Only draw if visible
    if (playheadX < bounds.getX() || playheadX > bounds.getRight())
        return;
    
    // White playhead line
    g.setColour(juce::Colour(0xffFFFFFF));
    g.drawLine(playheadX, bounds.getY(), playheadX, bounds.getBottom(), 2.0f);
}

void SkiaPianoRollComponent::resized()
{
    // Layout handled in paint()
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
