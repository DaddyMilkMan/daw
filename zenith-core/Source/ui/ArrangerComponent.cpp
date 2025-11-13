/**
 * @file ArrangerComponent.cpp
 * @brief Timeline/arranger view implementation with zoom and scroll
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

    // Enable keyboard focus for delete/duplicate shortcuts
    setWantsKeyboardFocus(true);
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

    // Hit-test for clip
    auto hitClip = hitTestClip(pos.toFloat());

    if (hitClip.isValid())
    {
        // Select this clip
        selected_ = hitClip;

        // Start dragging this clip
        const auto& project = editor_.getProject();
        const auto& track = project.tracks[hitClip.trackIndex];
        const auto& clip = track.clips[hitClip.clipIndex];

        drag_.active = true;
        drag_.trackIndex = hitClip.trackIndex;
        drag_.clipIndex = hitClip.clipIndex;
        drag_.originalClipStart = clip.startSample;
        drag_.dragStartSample = xToSamples(pos.x);

        repaint();
    }
    else
    {
        // Clear selection
        selected_ = SelectedClip{};

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

        // Update the clip by index
        if (drag_.clipIndex >= 0 && drag_.clipIndex < static_cast<int>(track.clips.size()))
        {
            track.clips[drag_.clipIndex].startSample = newStartSample;
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
    drag_.clipIndex = -1;
    drag_.originalClipStart = 0;
    drag_.dragStartSample = 0;
}

void ArrangerComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const bool cmdOrCtrl = e.mods.isCommandDown() || e.mods.isCtrlDown();
    const bool shift = e.mods.isShiftDown();

    if (cmdOrCtrl)
    {
        // Ctrl/Cmd + wheel = zoom
        const int anchorX = e.position.x;
        const juce::int64 anchorSample = xToSamples(anchorX);

        // Zoom factor: wheel.deltaY is typically -1.0 to 1.0
        // Negative = zoom in (decrease samplesPerPixel_)
        // Positive = zoom out (increase samplesPerPixel_)
        const double zoomFactor = 1.0 - (wheel.deltaY * 0.5);  // 0.5x to 1.5x per wheel notch
        double newSamplesPerPixel = samplesPerPixel_ * zoomFactor;

        // Clamp to limits
        newSamplesPerPixel = juce::jlimit(minSamplesPerPixel_, maxSamplesPerPixel_, newSamplesPerPixel);

        // Update zoom level
        samplesPerPixel_ = newSamplesPerPixel;

        // Adjust scroll offset to keep anchor point under cursor
        // anchorSample should still be at anchorX after zoom
        // anchorSample = scrollOffsetSamples_ + anchorX * samplesPerPixel_
        // => scrollOffsetSamples_ = anchorSample - anchorX * samplesPerPixel_
        scrollOffsetSamples_ = anchorSample - static_cast<juce::int64>(std::round(anchorX * samplesPerPixel_));

        // Clamp scroll offset to non-negative
        scrollOffsetSamples_ = juce::jmax((juce::int64) 0, scrollOffsetSamples_);

        repaint();
    }
    else if (shift)
    {
        // Shift + wheel = horizontal scroll
        const double scrollFactor = 0.1;  // scroll 10% of view per notch
        const auto visibleRange = getVisibleSampleRange();
        const juce::int64 scrollAmount = static_cast<juce::int64>(wheel.deltaY * scrollFactor * visibleRange.getLength());

        scrollOffsetSamples_ -= scrollAmount;  // deltaY positive = scroll right (decrease offset)

        // Clamp to non-negative
        scrollOffsetSamples_ = juce::jmax((juce::int64) 0, scrollOffsetSamples_);

        repaint();
    }
    else
    {
        // No modifiers = horizontal scroll (same as shift)
        const double scrollFactor = 0.1;
        const auto visibleRange = getVisibleSampleRange();
        const juce::int64 scrollAmount = static_cast<juce::int64>(wheel.deltaY * scrollFactor * visibleRange.getLength());

        scrollOffsetSamples_ -= scrollAmount;

        // Clamp to non-negative
        scrollOffsetSamples_ = juce::jmax((juce::int64) 0, scrollOffsetSamples_);

        repaint();
    }
}

//==============================================================================
// Keyboard Interactions
//==============================================================================

bool ArrangerComponent::keyPressed(const juce::KeyPress& key)
{
    // Delete/Backspace: delete selected clip
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (selected_.isValid())
        {
            if (editor_.deleteClip(selected_.trackIndex, selected_.clipIndex))
            {
                // Clear selection
                selected_ = SelectedClip{};
                repaint();
                return true;
            }
        }
    }

    // Ctrl/Cmd+D: duplicate selected clip
    if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0))
    {
        if (selected_.isValid())
        {
            // Offset by 0.5 seconds
            const auto sampleRate = editor_.getSampleRate();
            const juce::int64 offset = static_cast<juce::int64>(0.5 * sampleRate);

            if (editor_.duplicateClip(selected_.trackIndex, selected_.clipIndex, offset))
            {
                // Update selection to the new clip (last clip on the track)
                const auto& project = editor_.getProject();
                if (selected_.trackIndex >= 0 && selected_.trackIndex < static_cast<int>(project.tracks.size()))
                {
                    const auto& track = project.tracks[selected_.trackIndex];
                    selected_.clipIndex = static_cast<int>(track.clips.size()) - 1;
                }

                repaint();
                return true;
            }
        }
    }

    return false;  // Key not handled
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

    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(0, rulerY, getWidth(), rulerHeight_);

    // Get visible sample range
    const double sampleRate = getProjectSampleRate();
    const auto visibleRange = getVisibleSampleRange();
    const double startSec = static_cast<double>(visibleRange.getStart()) / sampleRate;
    const double endSec = static_cast<double>(visibleRange.getEnd()) / sampleRate;

    // Draw 1-second grid lines and labels for visible range only
    const int startSecInt = static_cast<int>(std::ceil(startSec));
    const int endSecInt = static_cast<int>(std::floor(endSec)) + 1;  // +1 to include last visible second

    g.setColour(juce::Colour(0xff3a3a3a));
    g.setFont(12.0f);

    for (int sec = startSecInt; sec <= endSecInt; ++sec)
    {
        const juce::int64 sample = static_cast<juce::int64>(sec * sampleRate);
        const int x = samplesToX(sample);

        // Skip if outside visible area
        if (x < trackHeaderWidth_ || x > getWidth())
            continue;

        // Vertical grid line (full height from ruler bottom to component bottom)
        g.drawLine(static_cast<float>(x), static_cast<float>(rulerHeight_),
                   static_cast<float>(x), static_cast<float>(getHeight()), 1.0f);

        // Time label in ruler
        g.setColour(juce::Colour(0xff999999));
        g.drawText(juce::String(sec) + "s",
                   x + 4, rulerY + 2, 60, rulerHeight_ - 4,
                   juce::Justification::centredLeft, false);

        g.setColour(juce::Colour(0xff3a3a3a));
    }
}

void ArrangerComponent::drawTracksAndClips(juce::Graphics& g)
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());
    const auto visibleRange = getVisibleSampleRange();

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

        // Draw clips (only if visible)
        for (int c = 0; c < static_cast<int>(track.clips.size()); ++c)
        {
            const auto& clip = track.clips[c];

            if (clip.muted)
                continue;  // Don't draw muted clips

            // Skip clips outside visible range (performance optimization)
            const juce::int64 clipEnd = clip.startSample + clip.lengthSamples;
            if (clipEnd < visibleRange.getStart() || clip.startSample > visibleRange.getEnd())
                continue;

            const auto clipBounds = getClipBounds(track, clip, t);

            // Check if this clip is selected
            const bool isSelected = (selected_.isValid() && selected_.trackIndex == t && selected_.clipIndex == c);

            // Clip rectangle (brighter if selected)
            if (isSelected)
                g.setColour(juce::Colour(0xff6ab0f2));  // Lighter blue
            else
                g.setColour(juce::Colour(0xff4a90e2));  // Normal blue
            g.fillRect(clipBounds);

            // Border (thicker/brighter if selected)
            if (isSelected)
            {
                g.setColour(juce::Colours::white);
                g.drawRect(clipBounds, 2);  // Thicker border
            }
            else
            {
                g.setColour(juce::Colour(0xff5a9ff2));
                g.drawRect(clipBounds, 1);
            }

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
    const int x = samplesToX(transportSample);

    // Red vertical line
    g.setColour(juce::Colours::red);
    g.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), static_cast<float>(getHeight()), 2.0f);

    // Playhead triangle at top
    juce::Path triangle;
    triangle.addTriangle(static_cast<float>(x - 6), 0.0f,
                        static_cast<float>(x + 6), 0.0f,
                        static_cast<float>(x), 12.0f);
    g.fillPath(triangle);
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::Rectangle<int> ArrangerComponent::getTrackBounds(int trackIndex) const
{
    const int y = rulerHeight_ + trackIndex * trackHeight_;
    return juce::Rectangle<int>(trackHeaderWidth_, y, getWidth() - trackHeaderWidth_, trackHeight_);
}

juce::Rectangle<int> ArrangerComponent::getClipBounds(const zenith::TrackModel& track,
                                                       const zenith::ClipModel& clip,
                                                       int trackIndex) const
{
    const auto trackBounds = getTrackBounds(trackIndex);

    const int x = samplesToX(clip.startSample);
    const int clipEndX = samplesToX(clip.startSample + clip.lengthSamples);
    const int width = clipEndX - x;

    // 4px padding on top/bottom
    const int padding = 4;

    return juce::Rectangle<int>(x, trackBounds.getY() + padding,
                                 width, trackBounds.getHeight() - 2 * padding);
}

double ArrangerComponent::getProjectSampleRate() const
{
    return editor_.getProject().sampleRate;
}

int ArrangerComponent::samplesToX(juce::int64 samples) const
{
    // samples → X coordinate (includes scroll offset)
    // X = (samples - scrollOffset) / samplesPerPixel
    return static_cast<int>(std::round((samples - scrollOffsetSamples_) / samplesPerPixel_));
}

juce::int64 ArrangerComponent::xToSamples(int x) const
{
    // X coordinate → samples (includes scroll offset)
    // samples = scrollOffset + X * samplesPerPixel
    return scrollOffsetSamples_ + static_cast<juce::int64>(std::round(x * samplesPerPixel_));
}

juce::Range<juce::int64> ArrangerComponent::getVisibleSampleRange() const
{
    const auto start = scrollOffsetSamples_;
    const auto end = scrollOffsetSamples_ + static_cast<juce::int64>(std::round(getWidth() * samplesPerPixel_));
    return juce::Range<juce::int64>(start, end);
}

ArrangerComponent::SelectedClip ArrangerComponent::hitTestClip(juce::Point<float> pos) const
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto& track = project.tracks[t];

        for (int c = 0; c < static_cast<int>(track.clips.size()); ++c)
        {
            const auto& clip = track.clips[c];

            if (clip.muted)
                continue;

            const auto clipBounds = getClipBounds(track, clip, t);

            if (clipBounds.contains(pos.toInt()))
            {
                SelectedClip result;
                result.trackIndex = t;
                result.clipIndex = c;
                return result;
            }
        }
    }

    // No clip hit - return invalid
    return SelectedClip{};
}
