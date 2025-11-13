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
    drawTrackHeaders(g);
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

    // Priority 1: Check for solo button click
    int soloTrackIndex = -1;
    if (hitTestSoloButton(pos, soloTrackIndex))
    {
        // Toggle solo for this track
        const bool newSolo = !editor_.isTrackSolo(soloTrackIndex);
        editor_.setTrackSolo(soloTrackIndex, newSolo);

        // Select this track
        selectedTrackIndex_ = soloTrackIndex;
        editor_.setSelectedTrack(soloTrackIndex);

        repaint();
        return;
    }

    // Priority 2: Check for mute button click
    int muteTrackIndex = -1;
    if (hitTestMuteButton(pos, muteTrackIndex))
    {
        // Toggle mute for this track
        const bool newMuted = !editor_.isTrackMuted(muteTrackIndex);
        editor_.setTrackMuted(muteTrackIndex, newMuted);

        // Select this track
        selectedTrackIndex_ = muteTrackIndex;
        editor_.setSelectedTrack(muteTrackIndex);

        repaint();
        return;
    }

    // Priority 3: Check for gain slider drag
    int gainTrackIndex = -1;
    if (hitTestGainSlider(pos, gainTrackIndex))
    {
        // Start gain adjustment
        dragMode_ = DragMode::AdjustGain;
        dragTrackForMixer_ = gainTrackIndex;

        // Select this track
        selectedTrackIndex_ = gainTrackIndex;
        editor_.setSelectedTrack(gainTrackIndex);

        // Apply initial value
        mouseDrag(e);
        return;
    }

    // Priority 4: Check for pan slider drag
    int panTrackIndex = -1;
    if (hitTestPanSlider(pos, panTrackIndex))
    {
        // Start pan adjustment
        dragMode_ = DragMode::AdjustPan;
        dragTrackForMixer_ = panTrackIndex;

        // Select this track
        selectedTrackIndex_ = panTrackIndex;
        editor_.setSelectedTrack(panTrackIndex);

        // Apply initial value
        mouseDrag(e);
        return;
    }

    // Priority 5: Check for track header click (selection only)
    const int headerTrack = hitTestTrackHeader(pos);
    if (headerTrack >= 0)
    {
        // Select this track
        selectedTrackIndex_ = headerTrack;
        editor_.setSelectedTrack(headerTrack);

        // Clear clip selection
        selected_ = SelectedClip{};
        repaint();
        return;
    }

    // Priority 3: Check for clip click with edge detection
    auto hitResult = hitTestClip(pos.toFloat());

    if (hitResult.clip.isValid())
    {
        // Select this clip
        selected_ = hitResult.clip;

        // Also select the track
        selectedTrackIndex_ = hitResult.clip.trackIndex;
        editor_.setSelectedTrack(hitResult.clip.trackIndex);

        // Set drag mode
        dragMode_ = hitResult.mode;
        dragClip_ = hitResult.clip;

        // Cache original positions
        const auto& project = editor_.getProject();
        const auto& track = project.tracks[dragClip_.trackIndex];
        const auto& clip = track.clips[dragClip_.clipIndex];

        dragOriginalStartSamples_ = clip.startSample;

        // Compute resolved end sample
        juce::int64 clipLen = clip.lengthSamples;
        if (clipLen == 0)
        {
            // For full-length clips, use a heuristic (1 second for now)
            // In real usage, this should come from decoded audio length
            clipLen = static_cast<juce::int64>(editor_.getSampleRate());
        }
        dragOriginalEndSamples_ = dragOriginalStartSamples_ + clipLen;

        repaint();
    }
    else
    {
        // Clear selection and drag mode
        selected_ = SelectedClip{};
        dragMode_ = DragMode::None;

        // Don't clear track selection on empty click (keep last selected)

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
    if (dragMode_ == DragMode::None)
        return;

    const auto pos = e.getPosition();

    switch (dragMode_)
    {
        case DragMode::AdjustGain:
        {
            if (dragTrackForMixer_ < 0)
                break;

            const int t = dragTrackForMixer_;
            const auto bounds = getGainSliderBounds(t);

            // Normalize X position to [0, 1]
            float norm = (pos.x - bounds.getX()) / static_cast<float>(bounds.getWidth());
            norm = juce::jlimit(0.0f, 1.0f, norm);

            // Convert to gain [0.0, 2.0]
            const float gain = norm * 2.0f;
            editor_.setTrackGain(t, gain);

            repaint();
            return;
        }

        case DragMode::AdjustPan:
        {
            if (dragTrackForMixer_ < 0)
                break;

            const int t = dragTrackForMixer_;
            const auto bounds = getPanSliderBounds(t);

            // Normalize X position to [0, 1]
            float norm = (pos.x - bounds.getX()) / static_cast<float>(bounds.getWidth());
            norm = juce::jlimit(0.0f, 1.0f, norm);

            // Convert to pan [-1.0, +1.0]
            const float pan = norm * 2.0f - 1.0f;
            editor_.setTrackPan(t, pan);

            repaint();
            return;
        }

        case DragMode::Move:
        {
            const juce::int64 mouseSamples = xToSamples(pos.x);

            // Calculate delta from original position
            const juce::int64 delta = mouseSamples - dragOriginalStartSamples_;

            // Apply delta to original position
            juce::int64 newStartSample = dragOriginalStartSamples_ + delta;

            // Clamp to non-negative
            newStartSample = juce::jmax((juce::int64) 0, newStartSample);

            // Update model (mutable)
            auto& project = editor_.getProjectMutable();
            if (dragClip_.trackIndex >= 0 && dragClip_.trackIndex < static_cast<int>(project.tracks.size()))
            {
                auto& track = project.tracks[dragClip_.trackIndex];

                // Update the clip by index
                if (dragClip_.clipIndex >= 0 && dragClip_.clipIndex < static_cast<int>(track.clips.size()))
                {
                    track.clips[dragClip_.clipIndex].startSample = newStartSample;
                }
            }

            repaint();
            break;
        }

        case DragMode::TrimLeft:
        {
            const juce::int64 mouseSamples = xToSamples(pos.x);

            // Clamp to valid range: [originalStart, originalEnd - minLength]
            auto newStart = juce::jlimit(
                dragOriginalStartSamples_,
                dragOriginalEndSamples_ - zenith::ProjectEditorState::kMinClipLengthSamples,
                mouseSamples
            );

            // Call trim function (which reloads playback)
            editor_.trimClipLeft(dragClip_.trackIndex, dragClip_.clipIndex, newStart);

            repaint();
            break;
        }

        case DragMode::TrimRight:
        {
            const juce::int64 mouseSamples = xToSamples(pos.x);

            // Clamp to valid range: [originalStart + minLength, infinity]
            auto newEnd = juce::jmax(
                dragOriginalStartSamples_ + zenith::ProjectEditorState::kMinClipLengthSamples,
                mouseSamples
            );

            // Call trim function (which reloads playback)
            editor_.trimClipRight(dragClip_.trackIndex, dragClip_.clipIndex, newEnd);

            repaint();
            break;
        }

        default:
            break;
    }
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& e)
{
    if (dragMode_ == DragMode::None)
        return;

    // For Move mode, apply changes to engine
    if (dragMode_ == DragMode::Move)
    {
        editor_.reloadPlayback();
    }

    // For trim modes, reloadPlayback() already called in trim functions
    // For mixer modes (gain/pan), reloadPlayback() already called in setters

    // Reset drag mode (keep selection)
    if (dragMode_ == DragMode::AdjustGain || dragMode_ == DragMode::AdjustPan)
    {
        dragMode_ = DragMode::None;
        dragTrackForMixer_ = -1;
    }
    else
    {
        dragMode_ = DragMode::None;
        dragOriginalStartSamples_ = 0;
        dragOriginalEndSamples_ = 0;
    }
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
        if (x < kTrackHeaderWidth || x > getWidth())
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

void ArrangerComponent::drawTrackHeaders(juce::Graphics& g)
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto& track = project.tracks[t];
        const auto headerBounds = getTrackHeaderBounds(t);
        const bool isSelected = (selectedTrackIndex_ == t || (selected_.isValid() && selected_.trackIndex == t));
        const bool isMuted = editor_.isTrackMuted(t);

        // Background color (highlight if selected, dim if muted)
        if (isSelected)
            g.setColour(juce::Colour(0xff3a3a3a));  // Brighter for selection
        else if (isMuted)
            g.setColour(juce::Colour(0xff1a1a1a));  // Dimmer for muted
        else
            g.setColour(juce::Colour(0xff2a2a2a));  // Normal
        g.fillRect(headerBounds);

        // Border
        g.setColour(isSelected ? juce::Colour(0xff5a5a5a) : juce::Colour(0xff444444));
        g.drawRect(headerBounds, 1);

        // Track index (1-based) in top-left
        g.setColour(isMuted ? juce::Colour(0xff666666) : juce::Colour(0xff999999));
        g.setFont(11.0f);
        g.drawText(juce::String(t + 1),
                   8, headerBounds.getY() + 4, 30, 16,
                   juce::Justification::centredLeft, false);

        // Track name (centered vertically)
        g.setColour(isMuted ? juce::Colour(0xff888888) : juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(track.name,
                   8, headerBounds.getY() + 20, kTrackHeaderWidth - 60, headerBounds.getHeight() - 24,
                   juce::Justification::centredLeft, true);

        // Solo button ("S") to the left of mute
        const auto soloButtonBounds = getSoloButtonBounds(t);
        const bool isSolo = editor_.isTrackSolo(t);

        if (isSolo)
        {
            // Filled button when solo
            g.setColour(juce::Colours::yellow.withBrightness(0.7f));
            g.fillRect(soloButtonBounds);
            g.setColour(juce::Colours::black);
            g.drawRect(soloButtonBounds, 1);
        }
        else
        {
            // Outlined button when not solo
            g.setColour(juce::Colour(0xff666666));
            g.drawRect(soloButtonBounds, 1);
        }

        // "S" label
        g.setColour(isSolo ? juce::Colours::black : juce::Colour(0xff999999));
        g.setFont(12.0f);
        g.drawText("S", soloButtonBounds, juce::Justification::centred, false);

        // Mute button ("M") in top-right
        const auto muteButtonBounds = getMuteButtonBounds(t);

        if (isMuted)
        {
            // Filled button when muted
            g.setColour(juce::Colour(0xffff6b6b));  // Red
            g.fillRect(muteButtonBounds);
            g.setColour(juce::Colours::white);
            g.drawRect(muteButtonBounds, 1);
        }
        else
        {
            // Outlined button when not muted
            g.setColour(juce::Colour(0xff666666));
            g.drawRect(muteButtonBounds, 1);
        }

        // "M" label
        g.setColour(isMuted ? juce::Colours::white : juce::Colour(0xff999999));
        g.setFont(12.0f);
        g.drawText("M", muteButtonBounds, juce::Justification::centred, false);

        // Gain slider (horizontal bar)
        const auto gainBounds = getGainSliderBounds(t);
        const float gain = editor_.getTrackGain(t);  // [0.0, 2.0]
        const float gainNorm = juce::jlimit(0.0f, 1.0f, gain / 2.0f);

        // Background bar
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillRect(gainBounds);

        // Fill bar (proportional to gain)
        const int fillWidth = static_cast<int>(gainBounds.getWidth() * gainNorm);
        auto fillRect = gainBounds.withWidth(fillWidth);
        g.setColour(juce::Colour(0xff66cc66));  // Light green
        g.fillRect(fillRect);

        // Border
        g.setColour(juce::Colour(0xff555555));
        g.drawRect(gainBounds, 1);

        // Pan slider (horizontal bar with center mark)
        const auto panBounds = getPanSliderBounds(t);
        const float pan = editor_.getTrackPan(t);  // [-1.0, 1.0]
        const float panNorm = (pan + 1.0f) * 0.5f;  // [0.0, 1.0]

        // Background bar
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillRect(panBounds);

        // Center mark (vertical line at 0.0 pan)
        const int centerX = panBounds.getX() + panBounds.getWidth() / 2;
        g.setColour(juce::Colour(0xff666666));
        g.drawLine(static_cast<float>(centerX), static_cast<float>(panBounds.getY()),
                   static_cast<float>(centerX), static_cast<float>(panBounds.getBottom()), 1.0f);

        // Handle (small rectangle at pan position)
        const int handleX = panBounds.getX() + static_cast<int>(panBounds.getWidth() * panNorm);
        const int handleWidth = 4;
        auto handleRect = juce::Rectangle<int>(handleX - handleWidth / 2, panBounds.getY(),
                                                handleWidth, panBounds.getHeight());
        g.setColour(juce::Colour(0xff66cccc));  // Cyan
        g.fillRect(handleRect);

        // Border
        g.setColour(juce::Colour(0xff555555));
        g.drawRect(panBounds, 1);
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
        const bool isSelected = (selectedTrackIndex_ == t || (selected_.isValid() && selected_.trackIndex == t));
        const bool isMuted = editor_.isTrackMuted(t);

        // Track lane (clip area) - highlight if selected, dim if muted
        g.setColour(juce::Colour(0xff242424));
        g.fillRect(kTrackHeaderWidth, trackBounds.getY(),
                   getWidth() - kTrackHeaderWidth, trackBounds.getHeight());

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
    return juce::Rectangle<int>(kTrackHeaderWidth, y, getWidth() - kTrackHeaderWidth, trackHeight_);
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
    // samples → X coordinate (includes scroll offset and header width)
    // X = kTrackHeaderWidth + (samples - scrollOffset) / samplesPerPixel
    return kTrackHeaderWidth + static_cast<int>(std::round((samples - scrollOffsetSamples_) / samplesPerPixel_));
}

juce::int64 ArrangerComponent::xToSamples(int x) const
{
    // X coordinate → samples (includes scroll offset and header width)
    // If x < kTrackHeaderWidth, we're in the header area (return 0)
    if (x < kTrackHeaderWidth)
        return 0;

    // samples = scrollOffset + (x - kTrackHeaderWidth) * samplesPerPixel
    return scrollOffsetSamples_ + static_cast<juce::int64>(std::round((x - kTrackHeaderWidth) * samplesPerPixel_));
}

juce::Range<juce::int64> ArrangerComponent::getVisibleSampleRange() const
{
    const auto start = scrollOffsetSamples_;
    const auto end = scrollOffsetSamples_ + static_cast<juce::int64>(std::round(getWidth() * samplesPerPixel_));
    return juce::Range<juce::int64>(start, end);
}

ArrangerComponent::HitTestResult ArrangerComponent::hitTestClip(juce::Point<float> pos) const
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
                HitTestResult result;
                result.clip.trackIndex = t;
                result.clip.clipIndex = c;

                // Detect edge zones
                const float x = pos.x;
                const float left = static_cast<float>(clipBounds.getX());
                const float right = static_cast<float>(clipBounds.getRight());

                if (std::abs(x - left) <= kEdgeHotZonePixels)
                {
                    result.mode = DragMode::TrimLeft;
                }
                else if (std::abs(x - right) <= kEdgeHotZonePixels)
                {
                    result.mode = DragMode::TrimRight;
                }
                else
                {
                    result.mode = DragMode::Move;
                }

                return result;
            }
        }
    }

    // No clip hit - return invalid
    return HitTestResult{};
}

int ArrangerComponent::hitTestTrackHeader(juce::Point<int> pos) const
{
    // Check if x is in header area
    if (pos.x >= kTrackHeaderWidth)
        return -1;

    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto headerBounds = getTrackHeaderBounds(t);
        if (headerBounds.contains(pos))
            return t;
    }

    return -1;
}

bool ArrangerComponent::hitTestMuteButton(juce::Point<int> pos, int& outTrackIndex) const
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto muteButtonBounds = getMuteButtonBounds(t);
        if (muteButtonBounds.contains(pos))
        {
            outTrackIndex = t;
            return true;
        }
    }

    return false;
}

bool ArrangerComponent::hitTestSoloButton(juce::Point<int> pos, int& outTrackIndex) const
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto soloButtonBounds = getSoloButtonBounds(t);
        if (soloButtonBounds.contains(pos))
        {
            outTrackIndex = t;
            return true;
        }
    }

    return false;
}

bool ArrangerComponent::hitTestGainSlider(juce::Point<int> pos, int& outTrackIndex) const
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto gainSliderBounds = getGainSliderBounds(t);
        if (gainSliderBounds.contains(pos))
        {
            outTrackIndex = t;
            return true;
        }
    }

    return false;
}

bool ArrangerComponent::hitTestPanSlider(juce::Point<int> pos, int& outTrackIndex) const
{
    const auto& project = editor_.getProject();
    const int numTracks = static_cast<int>(project.tracks.size());

    for (int t = 0; t < numTracks; ++t)
    {
        const auto panSliderBounds = getPanSliderBounds(t);
        if (panSliderBounds.contains(pos))
        {
            outTrackIndex = t;
            return true;
        }
    }

    return false;
}

juce::Rectangle<int> ArrangerComponent::getTrackHeaderBounds(int trackIndex) const
{
    const int y = rulerHeight_ + trackIndex * trackHeight_;
    return juce::Rectangle<int>(0, y, kTrackHeaderWidth, trackHeight_);
}

juce::Rectangle<int> ArrangerComponent::getMuteButtonBounds(int trackIndex) const
{
    const auto headerBounds = getTrackHeaderBounds(trackIndex);

    // Position mute button in top-right corner of header
    const int buttonSize = 24;
    const int margin = 8;
    return juce::Rectangle<int>(
        headerBounds.getRight() - buttonSize - margin,
        headerBounds.getY() + margin,
        buttonSize,
        buttonSize
    );
}

juce::Rectangle<int> ArrangerComponent::getSoloButtonBounds(int trackIndex) const
{
    const auto headerBounds = getTrackHeaderBounds(trackIndex);
    const auto muteButtonBounds = getMuteButtonBounds(trackIndex);

    // Position solo button to the left of mute button
    const int buttonSize = 24;
    const int spacing = 4;
    return juce::Rectangle<int>(
        muteButtonBounds.getX() - buttonSize - spacing,
        headerBounds.getY() + 8,
        buttonSize,
        buttonSize
    );
}

juce::Rectangle<int> ArrangerComponent::getGainSliderBounds(int trackIndex) const
{
    const auto headerBounds = getTrackHeaderBounds(trackIndex);

    // Position gain slider below track name
    const int sliderHeight = 12;
    const int margin = 8;
    const int y = headerBounds.getY() + 36;  // Below track name
    return juce::Rectangle<int>(
        margin,
        y,
        kTrackHeaderWidth - 2 * margin,
        sliderHeight
    );
}

juce::Rectangle<int> ArrangerComponent::getPanSliderBounds(int trackIndex) const
{
    const auto headerBounds = getTrackHeaderBounds(trackIndex);

    // Position pan slider below gain slider
    const int sliderHeight = 12;
    const int margin = 8;
    const int y = headerBounds.getY() + 52;  // Below gain slider
    return juce::Rectangle<int>(
        margin,
        y,
        kTrackHeaderWidth - 2 * margin,
        sliderHeight
    );
}
