/**
 * @file SkiaArrangementViewComponent.cpp
 * @brief Logic Pro style arrangement view implementation
 */

#include "SkiaArrangementViewComponent.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA

#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>

namespace zenith {

// Helper to create ARGB color
static constexpr SkColor ARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

SkiaArrangementViewComponent::SkiaArrangementViewComponent()
{
    setSize(800, 400);
}

void SkiaArrangementViewComponent::setPlayheadPosition(float beats)
{
    if (playheadPosition_ != beats)
    {
        playheadPosition_ = beats;
        repaint();
    }
}

void SkiaArrangementViewComponent::setLoopEnabled(bool enabled)
{
    if (loopEnabled_ != enabled)
    {
        loopEnabled_ = enabled;
        repaint();
    }
}

void SkiaArrangementViewComponent::setLoopRange(float startBeats, float endBeats)
{
    loopStart_ = startBeats;
    loopEnd_ = endBeats;
    repaint();
}

void SkiaArrangementViewComponent::addRegion(const AudioRegion& region)
{
    regions_.push_back(region);
    repaint();
}

void SkiaArrangementViewComponent::clearRegions()
{
    regions_.clear();
    repaint();
}

void SkiaArrangementViewComponent::setZoom(float pixelsPerBeat)
{
    pixelsPerBeat_ = std::max(10.0f, std::min(200.0f, pixelsPerBeat));
    repaint();
}

void SkiaArrangementViewComponent::setScrollPosition(float beats)
{
    scrollPosition_ = std::max(0.0f, beats);
    repaint();
}

void SkiaArrangementViewComponent::setTimeSignature(int numerator, int denominator)
{
    timeSigNumerator_ = numerator;
    timeSigDenominator_ = denominator;
    repaint();
}

void SkiaArrangementViewComponent::setTempo(float bpm)
{
    tempo_ = bpm;
    repaint();
}

float SkiaArrangementViewComponent::beatsToPixels(float beats) const
{
    return (beats - scrollPosition_) * pixelsPerBeat_;
}

float SkiaArrangementViewComponent::pixelsToBeats(float pixels) const
{
    return (pixels / pixelsPerBeat_) + scrollPosition_;
}

void SkiaArrangementViewComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Background (#202020 - Logic Pro arrangement background)
    g.fillAll(juce::Colour(0xff202020));
    
    // Split into ruler and arrangement areas
    auto rulerBounds = bounds.removeFromTop(rulerHeight_);
    auto arrangementBounds = bounds;
    
    // Draw components in order
    drawGrid(g, arrangementBounds);
    if (loopEnabled_)
        drawLoopRegion(g, arrangementBounds);
    drawRegions(g, arrangementBounds);
    drawRuler(g, rulerBounds);
    drawPlayhead(g, getLocalBounds());
}

void SkiaArrangementViewComponent::drawRuler(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Ruler background (#262626 - Logic Pro ruler)
    g.setColour(juce::Colour(0xff262626));
    g.fillRect(bounds);
    
    // Draw bar numbers
    g.setColour(juce::Colour(0xffAAAAAA));  // Logic Pro ruler text
    g.setFont(juce::Font(10.0f));
    
    int beatsPerBar = timeSigNumerator_;
    float visibleStartBeat = scrollPosition_;
    float visibleEndBeat = pixelsToBeats(bounds.getWidth());
    
    int startBar = (int)(visibleStartBeat / beatsPerBar);
    int endBar = (int)(visibleEndBeat / beatsPerBar) + 1;
    
    for (int bar = startBar; bar <= endBar; ++bar)
    {
        float barBeat = bar * beatsPerBar;
        float x = beatsToPixels(barBeat);
        
        if (x >= 0 && x < bounds.getWidth())
        {
            // Bar number
            juce::String barText = juce::String(bar + 1);  // 1-indexed
            g.drawText(barText, (int)x + 2, bounds.getY(), 40, bounds.getHeight(),
                      juce::Justification::centredLeft);
            
            // Tick mark
            g.setColour(juce::Colour(0xff555555));
            g.drawLine(x, bounds.getBottom() - 4, x, bounds.getBottom(), 1.0f);
        }
    }
    
    // Bottom border
    g.setColour(juce::Colour(0xff000000));
    g.drawLine(0, bounds.getBottom(), bounds.getWidth(), bounds.getBottom(), 1.0f);
}

void SkiaArrangementViewComponent::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    int beatsPerBar = timeSigNumerator_;
    float visibleStartBeat = scrollPosition_;
    float visibleEndBeat = pixelsToBeats(bounds.getWidth());
    
    // Draw beat lines (subtle)
    g.setColour(juce::Colour(0xff333333));  // Logic Pro beat grid (#333333)
    for (float beat = std::floor(visibleStartBeat); beat <= visibleEndBeat; beat += 1.0f)
    {
        float x = beatsToPixels(beat);
        if (x >= 0 && x < bounds.getWidth())
        {
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        }
    }
    
    // Draw bar lines (prominent)
    g.setColour(juce::Colour(0xff555555));  // Logic Pro bar grid (#555555)
    int startBar = (int)(visibleStartBeat / beatsPerBar);
    int endBar = (int)(visibleEndBeat / beatsPerBar) + 1;
    
    for (int bar = startBar; bar <= endBar; ++bar)
    {
        float barBeat = bar * beatsPerBar;
        float x = beatsToPixels(barBeat);
        
        if (x >= 0 && x < bounds.getWidth())
        {
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        }
    }
}

void SkiaArrangementViewComponent::drawRegions(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    int trackHeight = 80;  // Height per track
    
    for (const auto& region : regions_)
    {
        float regionX = beatsToPixels(region.startTime);
        float regionWidth = region.duration * pixelsPerBeat_;
        int regionY = bounds.getY() + (region.trackIndex * trackHeight);
        
        // Skip if not visible
        if (regionX + regionWidth < 0 || regionX > bounds.getWidth())
            continue;
        
        juce::Rectangle<float> regionBounds(regionX, regionY + 4, regionWidth, trackHeight - 8);
        
        // Region background (darker header, lighter body)
        juce::Rectangle<float> headerBounds = regionBounds.removeFromTop(16);
        
        if (region.isMuted)
        {
            // Muted: greyed out with diagonal stripes
            g.setColour(region.color.darker(0.6f));
        }
        else
        {
            g.setColour(region.color.darker(0.3f));
        }
        
        // Draw with 6px rounded corners (Logic Pro)
        g.fillRoundedRectangle(regionBounds.withTop(headerBounds.getY()), 6.0f);
        
        // Darker header
        g.setColour(region.color.darker(0.5f));
        g.fillRoundedRectangle(headerBounds.getX(), headerBounds.getY(), headerBounds.getWidth(), headerBounds.getHeight() + 6,
                              6.0f);
        
        // Region name (white with drop shadow)
        g.setColour(juce::Colour(0xffFFFFFF));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(region.name, headerBounds.reduced(4, 0).toNearestInt(), 
                  juce::Justification::centredLeft);
        
        // Border (1px white when selected, otherwise subtle)
        g.setColour(juce::Colour(0xff888888));
        g.drawRoundedRectangle(regionBounds.withTop(headerBounds.getY()), 6.0f, 1.0f);
        
        // Draw waveform if available
        if (!region.waveform.empty() && !region.isMuted)
        {
            juce::Rectangle<float> waveformBounds = regionBounds.withTop(headerBounds.getBottom() + 2);
            
            g.setColour(region.color.brighter(0.2f).withAlpha(0.8f));
            
            juce::Path waveformPath;
            float centerY = waveformBounds.getCentreY();
            float halfHeight = waveformBounds.getHeight() * 0.4f;
            
            for (size_t i = 0; i < region.waveform.size(); ++i)
            {
                float x = waveformBounds.getX() + (i * waveformBounds.getWidth() / region.waveform.size());
                float y = centerY - (region.waveform[i] * halfHeight);
                
                if (i == 0)
                    waveformPath.startNewSubPath(x, y);
                else
                    waveformPath.lineTo(x, y);
            }
            
            // Mirror for filled effect
            for (int i = region.waveform.size() - 1; i >= 0; --i)
            {
                float x = waveformBounds.getX() + (i * waveformBounds.getWidth() / region.waveform.size());
                float y = centerY + (region.waveform[i] * halfHeight);
                waveformPath.lineTo(x, y);
            }
            
            waveformPath.closeSubPath();
            g.fillPath(waveformPath);
        }
    }
}

void SkiaArrangementViewComponent::drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& totalBounds)
{
    float playheadX = beatsToPixels(playheadPosition_);
    
    // Only draw if visible
    if (playheadX < 0 || playheadX > totalBounds.getWidth())
        return;
    
    auto rulerBounds = totalBounds.withHeight(rulerHeight_);
    auto arrangementBounds = totalBounds.withTop(rulerHeight_);
    
    // White playhead line (2px, Logic Pro)
    g.setColour(juce::Colour(0xffFFFFFF));
    g.drawLine(playheadX, rulerBounds.getY(), playheadX, arrangementBounds.getBottom(), 2.0f);
    
    // Triangle cap at top
    juce::Path topTriangle;
    float triSize = 8.0f;
    topTriangle.addTriangle(playheadX, rulerBounds.getBottom(),
                           playheadX - triSize, rulerBounds.getBottom() - triSize,
                           playheadX + triSize, rulerBounds.getBottom() - triSize);
    g.fillPath(topTriangle);
}

void SkiaArrangementViewComponent::drawLoopRegion(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    float loopStartX = beatsToPixels(loopStart_);
    float loopEndX = beatsToPixels(loopEnd_);
    
    // Loop region with green tint
    juce::Rectangle<float> loopBounds(loopStartX, bounds.getY(), 
                                     loopEndX - loopStartX, bounds.getHeight());
    
    g.setColour(juce::Colour(0xff00FF00).withAlpha(0.1f));  // 10% green overlay
    g.fillRect(loopBounds);
    
    // Loop start/end markers (green lines)
    g.setColour(juce::Colour(0xff00FF00));
    g.drawLine(loopStartX, bounds.getY(), loopStartX, bounds.getBottom(), 2.0f);
    g.drawLine(loopEndX, bounds.getY(), loopEndX, bounds.getBottom(), 2.0f);
}

void SkiaArrangementViewComponent::resized()
{
    // Layout handled in paint()
}

void SkiaArrangementViewComponent::mouseDown(const juce::MouseEvent& event)
{
    // Click to set playhead position
    float clickedBeat = pixelsToBeats(event.x);
    setPlayheadPosition(clickedBeat);
}

void SkiaArrangementViewComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Drag to scrub playhead
    float clickedBeat = pixelsToBeats(event.x);
    setPlayheadPosition(clickedBeat);
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
