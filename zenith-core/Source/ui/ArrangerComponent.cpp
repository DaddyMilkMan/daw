/*
  ==============================================================================

    ArrangerComponent.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 4: Timeline/Arranger View

    Arranger component implementation

  ==============================================================================
*/

#include "ArrangerComponent.h"
#include "PianoRollComponent.h"
#include "../include/Engine.h"
#include "../include/TempoMap.h"
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"

//==============================================================================
ArrangerComponent::ArrangerComponent(Engine& eng)
    : engine(eng)
{
    // Enable mouse tracking for hover effects
    setMouseCursor(juce::MouseCursor::NormalCursor);
    
    // Start timer for playhead AND hover animations (60 FPS)
    startTimer(16);

    // Get current sample rate from engine
    currentSampleRate = engine.getSampleRate();
    if (currentSampleRate <= 0)
        currentSampleRate = 44100.0;

    DBG("ArrangerComponent: Initialized");
}

ArrangerComponent::~ArrangerComponent()
{
    stopTimer();
}

//==============================================================================
// Component interface
//==============================================================================

void ArrangerComponent::paint(juce::Graphics& g)
{
    // Modern dark background (#1A1A1D - deep charcoal)
    g.fillAll(juce::Colour(0xff1a1a1d));

    auto bounds = getLocalBounds();

    // Reserve top area for ruler
    auto rulerBounds = bounds.removeFromTop(static_cast<int>(rulerHeight));

    // Draw time ruler
    drawTimeRuler(g, rulerBounds);

    // Draw grid in main area
    drawGrid(g, bounds);

    // Draw tracks and clips
    drawTracks(g, bounds);

    // Draw playhead on top
    drawPlayhead(g, bounds);
}

void ArrangerComponent::resized()
{
    // Rebuild clip cache when size changes
    updateClipCache();
}

//==============================================================================
// Mouse interaction
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent& e)
{
    // Hit-test for clips
    auto* hit = hitTestClip(e.position);

    if (hit != nullptr)
    {
        // Select this clip with animation
        selectedClip = hit;
        selectionAlpha = 0.0f;
        selectionAnimating = true;
        
        clipDragStartSamples = hit->clip->getStartPosition();
        isDragging = false;
        dragStartPosition = e.position;

        DBG("ArrangerComponent: Selected clip - "
            + (hit->isMidi ? "MIDI" : "Audio")
            + " on track " + juce::String(hit->trackIndex));

        repaint();
    }
    else
    {
        // Clicked empty space - deselect with fade-out
        clearSelection();
        repaint();
    }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (selectedClip == nullptr)
        return;

    isDragging = true;

    // Calculate new start time based on mouse X position
    // Offset by ruler height
    float yOffset = rulerHeight;
    float relativeY = e.position.getY() - yOffset;

    // Only allow horizontal dragging for now
    float deltaX = e.position.getX() - dragStartPosition.getX();
    double deltaBeats = pixelsToBeats(deltaX);
    int64_t deltaSamples = beatsToSamples(deltaBeats);

    int64_t newStartSamples = clipDragStartSamples + deltaSamples;

    // Clamp to non-negative
    if (newStartSamples < 0)
        newStartSamples = 0;

    // TODO: Actually move the clip in the engine/model
    // For now, just update the visual (this is a stub)
    // selectedClip->clip->setStartPosition(newStartSamples);

    // Don't rebuild full cache on every drag - too expensive
    // Just invalidate the dragged clip area
    if (selectedClip != nullptr)
    {
        repaint(selectedClip->bounds.toNearestInt().expanded(2));
    }
}

void ArrangerComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Hit-test for clips
    auto* hit = hitTestClip(e.position);

    if (hit != nullptr && hit->isMidi)
    {
        DBG("ArrangerComponent: Double-clicked MIDI clip - opening piano roll");

        // Open piano roll window for this clip
        auto* pianoRollWindow = new PianoRollWindow(hit->clip);

        // Window will delete itself when closed
        juce::ignoreUnused(pianoRollWindow);
    }
}

void ArrangerComponent::mouseMove(const juce::MouseEvent& e)
{
    // Update hover state
    auto* hit = hitTestClip(e.position);
    
    if (hit != hoveredClip)
    {
        hoveredClip = hit;
        hoverAlpha = 0.0f;
        repaint();
    }
}

void ArrangerComponent::mouseExit(const juce::MouseEvent& /*e*/)
{
    hoveredClip = nullptr;
    hoverAlpha = 0.0f;
    repaint();
}

//==============================================================================
// View control
//==============================================================================

void ArrangerComponent::setPixelsPerBeat(float ppb)
{
    pixelsPerBeat = juce::jlimit(10.0f, 200.0f, ppb);
    updateClipCache();
    repaint();
}

void ArrangerComponent::setTrackHeight(float height)
{
    trackHeight = juce::jlimit(40.0f, 200.0f, height);
    updateClipCache();
    repaint();
}

void ArrangerComponent::clearSelection()
{
    if (selectedClip != nullptr)
    {
        selectionAlpha = 1.0f;
        selectionAnimating = true;  // Animate fade-out
    }
    selectedClip = nullptr;
    isDragging = false;
}

//==============================================================================
// Timer callback
//==============================================================================

void ArrangerComponent::timerCallback()
{
    bool needsRepaint = false;
    
    // Animate hover alpha
    if (hoveredClip != nullptr && hoverAlpha < 1.0f)
    {
        hoverAlpha = std::min(1.0f, hoverAlpha + 0.15f);
        needsRepaint = true;
    }
    
    // Animate selection fade-in
    if (selectionAnimating && selectedClip != nullptr)
    {
        if (selectionAlpha < 1.0f)
        {
            selectionAlpha = std::min(1.0f, selectionAlpha + 0.2f);
            needsRepaint = true;
        }
        else
        {
            selectionAnimating = false;
        }
    }
    
    // Animate selection fade-out
    if (selectionAnimating && selectedClip == nullptr)
    {
        if (selectionAlpha > 0.0f)
        {
            selectionAlpha = std::max(0.0f, selectionAlpha - 0.2f);
            needsRepaint = true;
        }
        else
        {
            selectionAnimating = false;
        }
    }
    
    // Only repaint playhead area when playing to avoid full screen repaints
    if (engine.isPlaying())
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(static_cast<int>(rulerHeight));
        
        // Get playhead position
        int64_t playheadSamples = engine.getPlaybackPosition();
        double playheadBeats = samplesToBeats(playheadSamples);
        float playheadX = beatsToPixels(playheadBeats);
        
        // Only repaint a small column around playhead for efficiency
        int repaintX = static_cast<int>(playheadX) - 2;
        int repaintWidth = 5;
        repaint(repaintX, 0, repaintWidth, getHeight());
    }
    else if (needsRepaint)
    {
        repaint();
    }
}

//==============================================================================
// Rendering helpers
//==============================================================================

void ArrangerComponent::drawTimeRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Ruler background - slightly lighter (#242428)
    g.setColour(juce::Colour(0xff242428));
    g.fillRect(bounds);

    // Draw bar/beat markers with modern font
    g.setColour(juce::Colour(0xffc0c0c0));  // Light gray for text
    g.setFont(juce::Font("Inter", 11.0f, juce::Font::plain));

    int maxBeats = static_cast<int>(pixelsToBeats(static_cast<float>(getWidth())) + 1);

    for (int beat = 0; beat <= maxBeats; beat += 4)  // Draw every 4 beats (1 bar in 4/4)
    {
        float x = beatsToPixels(beat);

        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            // Draw bar number
            int barNumber = beat / 4 + 1;
            juce::String label = juce::String(barNumber);

            g.drawText(label,
                      static_cast<int>(x) + 4,
                      bounds.getY() + 2,
                      50,
                      bounds.getHeight() - 2,
                      juce::Justification::centredLeft,
                      false);
        }
    }

    // Subtle bottom border
    g.setColour(juce::Colour(0xff404040));
    g.drawLine(static_cast<float>(bounds.getX()),
               static_cast<float>(bounds.getBottom()) - 0.5f,
               static_cast<float>(bounds.getRight()),
               static_cast<float>(bounds.getBottom()) - 0.5f,
               1.0f);
}

void ArrangerComponent::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Vertical grid lines (beats) - subtle
    int maxBeats = static_cast<int>(pixelsToBeats(static_cast<float>(getWidth())) + 1);

    for (int beat = 0; beat <= maxBeats; ++beat)
    {
        float x = beatsToPixels(beat);

        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            // Bar lines brighter, beat lines subtle
            if (beat % 4 == 0)
                g.setColour(juce::Colour(0xff333333));  // Bar lines
            else
                g.setColour(juce::Colour(0xff252525));  // Beat lines

            g.drawVerticalLine(static_cast<int>(x),
                             static_cast<float>(bounds.getY() + rulerHeight),
                             static_cast<float>(bounds.getBottom()));
        }
    }

    // Horizontal grid lines (tracks) - very subtle
    int numTracks = engine.getNumTracks();

    for (int i = 0; i <= numTracks; ++i)
    {
        float y = bounds.getY() + i * trackHeight;

        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawHorizontalLine(static_cast<int>(y),
                           static_cast<float>(bounds.getX()),
                           static_cast<float>(bounds.getRight()));
    }
}

void ArrangerComponent::drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto& tracks = engine.tracks();

    int trackIndex = 0;

    for (const auto& track : tracks)
    {
        if (track == nullptr)
            continue;

        float trackY = bounds.getY() + trackIndex * trackHeight;

        // Draw track background with subtle alternating colors
        if (trackIndex % 2 == 0)
            g.setColour(juce::Colour(0xff1e1e1e));
        else
            g.setColour(juce::Colour(0xff1a1a1d));

        g.fillRect(bounds.getX(),
                   static_cast<int>(trackY),
                   bounds.getWidth(),
                   static_cast<int>(trackHeight));

        // Draw clips on this track
        for (const auto& clipVisual : clipCache)
        {
            if (clipVisual.trackIndex != trackIndex)
                continue;

            // Determine base color
            juce::Colour baseColor;
            if (clipVisual.isMidi)
            {
                // MIDI - vibrant green (#42C52E from Ableton palette)
                baseColor = juce::Colour(0xff42c52e).darker(0.2f);
            }
            else
            {
                // Audio - professional blue (#5C86E1 from Ableton palette)
                baseColor = juce::Colour(0xff5c86e1).darker(0.3f);
            }

            // Apply hover brightness
            if (&clipVisual == hoveredClip && hoverAlpha > 0.0f)
            {
                baseColor = baseColor.brighter(hoverAlpha * 0.15f);
            }

            // Draw clip with rounded corners
            auto clipRect = clipVisual.bounds.reduced(2.0f, 3.0f);
            g.setColour(baseColor);
            g.fillRoundedRectangle(clipRect, 2.0f);

            // Add subtle highlight on top edge for depth
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRoundedRectangle(clipRect.removeFromTop(2.0f), 2.0f);

            // Hover glow effect (glassmorphic)
            if (&clipVisual == hoveredClip && hoverAlpha > 0.0f)
            {
                g.setColour(juce::Colours::white.withAlpha(hoverAlpha * 0.12f));
                g.drawRoundedRectangle(clipVisual.bounds.reduced(1.0f, 2.0f).toFloat().expanded(1.0f), 3.0f, 2.0f);
            }

            // Selection highlight with modern accent color and animation
            if (selectedClip == &clipVisual)
            {
                float animAlpha = selectionAnimating ? selectionAlpha : 1.0f;
                
                // Animated selection border
                g.setColour(juce::Colour(0xffffd054).withAlpha(animAlpha));
                g.drawRoundedRectangle(clipVisual.bounds.reduced(1.0f, 2.0f), 2.0f, 2.5f);
                
                // Glassmorphic glow
                g.setColour(juce::Colour(0xffffd054).withAlpha(animAlpha * 0.2f));
                g.drawRoundedRectangle(clipVisual.bounds.reduced(0.5f, 1.5f).expanded(1.5f), 3.0f, 2.0f);
            }

            // Draw clip name with better contrast
            if (clipVisual.clip != nullptr)
            {
                g.setColour(juce::Colours::white.withAlpha(0.9f));
                g.setFont(juce::Font("Inter", 11.0f, juce::Font::plain));

                auto textBounds = clipVisual.bounds.reduced(6.0f, 4.0f);
                g.drawFittedText(clipVisual.clip->getName(),
                               textBounds.toNearestInt(),
                               juce::Justification::centredLeft,
                               1);
            }
        }

        trackIndex++;
    }
}

void ArrangerComponent::drawPlayhead(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (!engine.isPlaying())
        return;

    // Get playhead position from engine
    int64_t playheadSamples = engine.getPlaybackPosition();

    double playheadBeats = samplesToBeats(playheadSamples);
    float playheadX = beatsToPixels(playheadBeats);

    // Draw playhead line with modern accent color (#FE9AAA - soft pink from Ableton)
    g.setColour(juce::Colour(0xfffe9aaa));
    // JUCE 8: drawVerticalLine takes (x, y1, y2) - use drawLine for thickness
    g.drawLine(playheadX, static_cast<float>(bounds.getY()),
               playheadX, static_cast<float>(bounds.getBottom()),
               2.0f);  // Thicker line

    // Draw playhead handle at top with shadow for depth
    juce::Path triangle;
    triangle.addTriangle(playheadX - 7, static_cast<float>(bounds.getY()),
                        playheadX + 7, static_cast<float>(bounds.getY()),
                        playheadX, static_cast<float>(bounds.getY()) + 12);

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillPath(triangle, juce::AffineTransform::translation(0, 1));
    
    // Triangle itself
    g.setColour(juce::Colour(0xfffe9aaa));
    g.fillPath(triangle);
}

//==============================================================================
// Time mapping
//==============================================================================

float ArrangerComponent::beatsToPixels(double beats) const
{
    return static_cast<float>(beats * pixelsPerBeat);
}

double ArrangerComponent::pixelsToBeats(float pixels) const
{
    return pixels / pixelsPerBeat;
}

int64_t ArrangerComponent::beatsToSamples(double beats) const
{
    // Use Engine's TempoMap for accurate conversion
    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) sampleRate = currentSampleRate;
    
    return engine.getTempoMap().beatsToSamples(beats, sampleRate);
}

double ArrangerComponent::samplesToBeats(int64_t samples) const
{
    // Use Engine's TempoMap for accurate conversion
    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) sampleRate = currentSampleRate;
    
    return engine.getTempoMap().samplesToBeats(samples, sampleRate);
}

//==============================================================================
// Clip cache management
//==============================================================================

void ArrangerComponent::updateClipCache()
{
    clipCache.clear();

    const auto& tracks = engine.tracks();

    int trackIndex = 0;

    for (const auto& track : tracks)
    {
        if (track == nullptr)
            continue;

        float trackY = rulerHeight + trackIndex * trackHeight;

        // Iterate clips on this track
        int numClips = track->getNumClips();

        for (int i = 0; i < numClips; ++i)
        {
            auto* clip = track->getClip(i);

            if (clip == nullptr)
                continue;

            // Convert clip position to screen coordinates
            int64_t startSamples = clip->getStartPosition();
            int64_t lengthSamples = clip->getLength();

            double startBeats = samplesToBeats(startSamples);
            double lengthBeats = samplesToBeats(lengthSamples);

            float clipX = beatsToPixels(startBeats);
            float clipWidth = beatsToPixels(lengthBeats);

            // Create ClipVisual
            ClipVisual visual;
            visual.track = track.get();
            visual.clip = clip;
            visual.bounds = juce::Rectangle<float>(clipX, trackY, clipWidth, trackHeight);
            visual.isMidi = (clip->getType() == zenith::Track::Clip::Type::MIDI);
            visual.trackIndex = trackIndex;

            clipCache.push_back(visual);
        }

        trackIndex++;
    }

    DBG("ArrangerComponent: Clip cache updated - " + juce::String(clipCache.size()) + " clips");
}

ClipVisual* ArrangerComponent::hitTestClip(juce::Point<float> position)
{
    // Iterate in reverse order so top-most clips are hit first
    for (auto it = clipCache.rbegin(); it != clipCache.rend(); ++it)
    {
        if (it->bounds.contains(position))
        {
            return &(*it);
        }
    }

    return nullptr;
}
