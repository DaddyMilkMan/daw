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
#include "engine/Track.h"
#include "engine/Clip.h"

//==============================================================================
ArrangerComponent::ArrangerComponent(Engine& eng)
    : engine(eng)
{
    // Start timer for playhead animation (60 FPS)
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
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

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
        // Select this clip
        selectedClip = hit;
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
        // Clicked empty space - deselect
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

    // Update clip cache to reflect new position
    updateClipCache();
    repaint();
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
    selectedClip = nullptr;
    isDragging = false;
}

//==============================================================================
// Timer callback
//==============================================================================

void ArrangerComponent::timerCallback()
{
    // Repaint to update playhead position
    // Only repaint the playhead area for efficiency
    if (engine.isPlaying())
    {
        repaint();  // TODO: optimize to only repaint playhead column
    }
}

//==============================================================================
// Rendering helpers
//==============================================================================

void ArrangerComponent::drawTimeRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Background
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRect(bounds);

    // Draw bar/beat markers
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(12.0f));

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
                      static_cast<int>(x) + 2,
                      bounds.getY(),
                      50,
                      bounds.getHeight(),
                      juce::Justification::centredLeft,
                      false);
        }
    }

    // Border at bottom
    g.setColour(juce::Colours::black);
    g.drawLine(static_cast<float>(bounds.getX()),
               static_cast<float>(bounds.getBottom()),
               static_cast<float>(bounds.getRight()),
               static_cast<float>(bounds.getBottom()));
}

void ArrangerComponent::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.3f));

    // Vertical grid lines (beats)
    int maxBeats = static_cast<int>(pixelsToBeats(static_cast<float>(getWidth())) + 1);

    for (int beat = 0; beat <= maxBeats; ++beat)
    {
        float x = beatsToPixels(beat);

        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            // Thicker line every 4 beats (bar line)
            if (beat % 4 == 0)
                g.setColour(juce::Colour(0xff5a5a5a).withAlpha(0.5f));
            else
                g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.3f));

            g.drawVerticalLine(static_cast<int>(x),
                             static_cast<float>(bounds.getY() + rulerHeight),
                             static_cast<float>(bounds.getBottom()));
        }
    }

    // Horizontal grid lines (tracks)
    int numTracks = engine.getNumTracks();

    for (int i = 0; i <= numTracks; ++i)
    {
        float y = bounds.getY() + i * trackHeight;

        g.setColour(juce::Colour(0xff3a3a3a));
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

        // Draw track background (alternating colors)
        if (trackIndex % 2 == 0)
            g.setColour(juce::Colour(0xff2e2e2e));
        else
            g.setColour(juce::Colour(0xff2a2a2a));

        g.fillRect(bounds.getX(),
                   static_cast<int>(trackY),
                   bounds.getWidth(),
                   static_cast<int>(trackHeight));

        // Draw clips on this track
        for (const auto& clipVisual : clipCache)
        {
            if (clipVisual.trackIndex != trackIndex)
                continue;

            // Clip color
            if (clipVisual.isMidi)
                g.setColour(juce::Colours::green.darker(0.3f));
            else
                g.setColour(juce::Colours::blue.darker(0.3f));

            // Draw clip rectangle
            g.fillRect(clipVisual.bounds.reduced(1.0f, 2.0f));

            // Highlight if selected
            if (selectedClip == &clipVisual)
            {
                g.setColour(juce::Colours::yellow);
                g.drawRect(clipVisual.bounds, 2.0f);
            }

            // Draw clip name
            if (clipVisual.clip != nullptr)
            {
                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(12.0f));

                auto textBounds = clipVisual.bounds.reduced(4.0f, 2.0f);
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

    // Draw playhead line
    g.setColour(juce::Colours::white);
    g.drawVerticalLine(static_cast<int>(playheadX),
                      static_cast<float>(bounds.getY()),
                      static_cast<float>(bounds.getBottom()));

    // Draw playhead triangle at top
    juce::Path triangle;
    triangle.addTriangle(playheadX - 6, static_cast<float>(bounds.getY()),
                        playheadX + 6, static_cast<float>(bounds.getY()),
                        playheadX, static_cast<float>(bounds.getY()) + 10);

    g.setColour(juce::Colours::white);
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
    // beats = quarter notes
    // tempo = BPM (beats per minute)
    // samples per beat = sampleRate * 60.0 / tempo
    double samplesPerBeat = currentSampleRate * 60.0 / currentTempo;
    return static_cast<int64_t>(beats * samplesPerBeat);
}

double ArrangerComponent::samplesToBeats(int64_t samples) const
{
    double samplesPerBeat = currentSampleRate * 60.0 / currentTempo;
    return static_cast<double>(samples) / samplesPerBeat;
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
