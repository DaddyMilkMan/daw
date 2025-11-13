/**
 * @file ArrangerComponent.cpp
 * @brief Minimal timeline/arranger view implementation
 */

#include "ArrangerComponent.h"

//==============================================================================
// Constructor
//==============================================================================

ArrangerComponent::ArrangerComponent(zenith::ProjectEditorState& editorState)
    : editor_(editorState)
{
    // Start timer for playhead animation (30 fps is fine for v0.1)
    startTimer(33);
}

//==============================================================================
// Component Interface
//==============================================================================

void ArrangerComponent::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawTracksAndClips(g);
    drawPlayhead(g);
}

void ArrangerComponent::resized()
{
    // Nothing to layout - all drawing is coordinate-based
}

//==============================================================================
// Mouse Interactions
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Check if clicking on a clip
    int trackIndex = -1;
    juce::int64 clipId = -1;

    if (findClipAtPosition(pos, trackIndex, clipId))
    {
        // Start dragging this clip
        const auto& project = editor_.getProject();
        if (trackIndex >= 0 && trackIndex < static_cast<int>(project.tracks.size()))
        {
            const auto& track = project.tracks[trackIndex];

            // Find the clip model
            for (const auto& clip : track.clips)
            {
                if (clip.id == clipId)
                {
                    drag_.active = true;
                    drag_.trackIndex = trackIndex;
                    drag_.clipId = clipId;
                    drag_.originalClipStart = clip.startSample;
                    drag_.dragStartSample = xToSamples(pos.x);
                    break;
                }
            }
        }
    }
    else
    {
        // Clicking on background - set playhead without playing
        const juce::int64 clickSample = xToSamples(pos.x);

        // Clamp to non-negative
        const juce::int64 targetSample = juce::jmax((juce::int64) 0, clickSample);

        // Seek to clicked position
        editor_.setPlayheadSamples(targetSample);

        repaint();
    }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!drag_.active)
        return;

    const auto pos = e.getPosition();
    const juce::int64 currentSample = xToSamples(pos.x);

    // Calculate delta from drag start
    const juce::int64 delta = currentSample - drag_.dragStartSample;

    // Apply delta to original position
    juce::int64 newStartSample = drag_.originalClipStart + delta;

    // Clamp to non-negative
    newStartSample = juce::jmax((juce::int64) 0, newStartSample);

    // Update model (mutable)
    auto& project = editor_.getProjectMutable();
    if (drag_.trackIndex >= 0 && drag_.trackIndex < static_cast<int>(project.tracks.size()))
    {
        auto& track = project.tracks[drag_.trackIndex];

        // Find and update the clip
        for (auto& clip : track.clips)
        {
            if (clip.id == drag_.clipId)
            {
                clip.startSample = newStartSample;
                break;
            }
        }
    }

    // Repaint to show updated position
    repaint();
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& e)
{
    if (!drag_.active)
        return;

    // Apply changes to engine
    editor_.reloadPlayback();

    // Clear drag state
    drag_.active = false;
    drag_.trackIndex = -1;
    drag_.clipId = -1;
    drag_.originalClipStart = 0;
    drag_.dragStartSample = 0;
}

//==============================================================================
// Timer Callback
//==============================================================================

void ArrangerComponent::timerCallback()
{
    // Repaint during playback to animate playhead
    if (editor_.isPlaying())
    {
        repaint();
    }
}

//==============================================================================
// Drawing Methods
//==============================================================================

void ArrangerComponent::drawBackground(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Draw time ruler at top
    const int rulerY = 0;
    const int rulerHeight = timelineHeight_;

    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(0, rulerY, getWidth(), rulerHeight);

    // Draw 1-second grid lines and labels
    const double sampleRate = getProjectSampleRate();
    const juce::int64 samplesPerSecond = static_cast<juce::int64>(sampleRate);

    g.setColour(juce::Colour(0xff3a3a3a));
    g.setFont(12.0f);

    // Draw grid lines every second
    for (int sec = 0; sec < 100; ++sec)  // Arbitrary max for v0.1
    {
        const juce::int64 sample = sec * samplesPerSecond;
        const int x = static_cast<int>(samplesToX(sample));

        if (x > getWidth())
            break;

        // Vertical grid line
        g.drawLine(x, rulerHeight, x, getHeight(), 1.0f);

        // Time label
        g.setColour(juce::Colour(0xff999999));
        g.drawText(juce::String(sec) + "s",
                   x + 4, rulerY + 4, 60, rulerHeight - 8,
                   juce::Justification::centredLeft, false);

        g.setColour(juce::Colour(0xff3a3a3a));
    }
}

void ArrangerComponent::drawTracksAndClips(juce::Graphics& g)
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto& track = project.tracks[t];
        const auto trackBounds = getTrackBounds(t);

        // Track header (left side)
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(0, trackBounds.getY(), trackHeaderWidth_, trackBounds.getHeight());

        g.setColour(juce::Colour(0xff444444));
        g.drawRect(0, trackBounds.getY(), trackHeaderWidth_, trackBounds.getHeight(), 1);

        // Track name
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(track.name,
                   8, trackBounds.getY(), trackHeaderWidth_ - 16, trackBounds.getHeight(),
                   juce::Justification::centredLeft, true);

        // Track lane (clip area)
        g.setColour(juce::Colour(0xff242424));
        g.fillRect(trackHeaderWidth_, trackBounds.getY(),
                   getWidth() - trackHeaderWidth_, trackBounds.getHeight());

        // Separator line
        g.setColour(juce::Colour(0xff444444));
        g.drawLine(0, trackBounds.getBottom(), getWidth(), trackBounds.getBottom(), 1.0f);

        // Draw clips
        for (const auto& clip : track.clips)
        {
            if (clip.muted)
                continue;  // Don't draw muted clips

            const auto clipBounds = getClipBounds(track, clip, t);

            // Clip rectangle
            g.setColour(juce::Colour(0xff4a90e2));  // Blue
            g.fillRect(clipBounds);

            // Border
            g.setColour(juce::Colour(0xff5a9ff2));
            g.drawRect(clipBounds, 1);

            // Clip name (if wide enough)
            if (clipBounds.getWidth() > 50)
            {
                g.setColour(juce::Colours::white);
                g.setFont(12.0f);
                g.drawText(clip.file.getFileNameWithoutExtension(),
                           clipBounds.getX() + 4, clipBounds.getY(),
                           clipBounds.getWidth() - 8, clipBounds.getHeight(),
                           juce::Justification::centredLeft, true);
            }
        }
    }
}

void ArrangerComponent::drawPlayhead(juce::Graphics& g)
{
    const juce::int64 transportSample = editor_.getTransportSamples();
    const int x = static_cast<int>(samplesToX(transportSample));

    // Red vertical line
    g.setColour(juce::Colours::red);
    g.drawLine(x, 0, x, getHeight(), 2.0f);

    // Playhead triangle at top
    juce::Path triangle;
    triangle.addTriangle(x - 6, 0, x + 6, 0, x, 12);
    g.fillPath(triangle);
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::Rectangle<int> ArrangerComponent::getTrackBounds(int trackIndex) const
{
    const int y = timelineHeight_ + trackIndex * trackHeight_;
    return juce::Rectangle<int>(trackHeaderWidth_, y, getWidth() - trackHeaderWidth_, trackHeight_);
}

juce::Rectangle<int> ArrangerComponent::getClipBounds(const zenith::TrackModel& track,
                                                       const zenith::ClipModel& clip,
                                                       int trackIndex) const
{
    const auto trackBounds = getTrackBounds(trackIndex);

    const int x = static_cast<int>(samplesToX(clip.startSample));
    const int width = static_cast<int>(samplesToX(clip.lengthSamples));

    // 4px padding on top/bottom
    const int padding = 4;

    return juce::Rectangle<int>(x, trackBounds.getY() + padding,
                                 width, trackBounds.getHeight() - 2 * padding);
}

double ArrangerComponent::getProjectSampleRate() const
{
    return editor_.getProject().sampleRate;
}

double ArrangerComponent::samplesToX(juce::int64 samples) const
{
    const double seconds = samples / getProjectSampleRate();
    return trackHeaderWidth_ + seconds * pixelsPerSecond_;
}

juce::int64 ArrangerComponent::xToSamples(int x) const
{
    const int timelineX = x - trackHeaderWidth_;
    const double seconds = timelineX / pixelsPerSecond_;
    return static_cast<juce::int64>(seconds * getProjectSampleRate());
}

bool ArrangerComponent::findClipAtPosition(juce::Point<int> pos, int& outTrackIndex, juce::int64& outClipId)
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto& track = project.tracks[t];

        for (const auto& clip : track.clips)
        {
            if (clip.muted)
                continue;

            const auto clipBounds = getClipBounds(track, clip, t);

            if (clipBounds.contains(pos))
            {
                outTrackIndex = t;
                outClipId = clip.id;
                return true;
            }
        }
    }

    return false;
}
