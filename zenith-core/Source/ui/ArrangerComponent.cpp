/**
 * @file ArrangerComponent.cpp
 * @brief ArrangerComponent implementation
 */

#include "ArrangerComponent.h"

namespace zenith {

ArrangerComponent::ArrangerComponent(ProjectEditorState& editorState)
    : editor_(editorState)
{
    setOpaque(true);
}

ArrangerComponent::~ArrangerComponent()
{
}

//==============================================================================
// Component Interface
//==============================================================================

void ArrangerComponent::paint(juce::Graphics& g)
{
    // Background: dark grey
    g.fillAll(juce::Colour(0xff2b2b2b));

    // Draw tracks and clips
    drawTracks_(g);

    // Draw playhead on top
    drawPlayhead_(g);
}

void ArrangerComponent::resized()
{
    // v0.1: No child components, nothing to layout
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent& e)
{
    // Only handle left mouse button
    if (!e.mods.isLeftButtonDown())
        return;

    // Check if we hit a clip
    auto [trackIdx, clipIdx] = hitTestClip_(e.x, e.y);

    if (trackIdx >= 0 && clipIdx >= 0)
    {
        // Hit a clip - start drag operation
        const auto& project = editor_.getProject();
        const auto& clip = project.tracks[trackIdx].clips[clipIdx];

        drag_.active = true;
        drag_.trackIndex = trackIdx;
        drag_.clipIndex = clipIdx;
        drag_.originalStart = clip.startSample;
        drag_.dragStartX = e.x;

        DBG("Started dragging clip " << clip.id << " at sample " << clip.startSample);
    }
    else
    {
        // Clicked in timeline background - set playhead
        if (e.x >= kTrackHeaderWidth)
        {
            auto samples = xToSamples_(e.x);
            editor_.setPlayheadSamples(samples);
            repaint();

            DBG("Playhead set to sample " << samples);
        }
    }
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!drag_.active)
        return;

    // Compute pixel delta
    int dx = e.x - drag_.dragStartX;

    // Convert to sample delta
    double sr = getProjectSampleRate_();
    double secondsDelta = static_cast<double>(dx) / kPixelsPerSecond;
    auto samplesDelta = static_cast<int64_t>(std::llround(secondsDelta * sr));

    // Compute new start position (clamped to >= 0)
    auto newStart = drag_.originalStart + samplesDelta;
    if (newStart < 0)
        newStart = 0;

    // Update model via editor
    editor_.moveClipStart(drag_.trackIndex, drag_.clipIndex, newStart);

    // Repaint to show new position
    repaint();
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    if (drag_.active)
    {
        DBG("Finished dragging clip");
        drag_.active = false;
    }
}

//==============================================================================
// Internal Helpers
//==============================================================================

double ArrangerComponent::getProjectSampleRate_() const
{
    const auto& project = editor_.getProject();
    return project.sampleRate > 0.0 ? project.sampleRate : 48000.0;
}

float ArrangerComponent::timeToX_(int64_t samples) const
{
    double sr = getProjectSampleRate_();
    double timeSeconds = static_cast<double>(samples) / sr;
    return kTrackHeaderWidth + static_cast<float>(timeSeconds * kPixelsPerSecond);
}

int64_t ArrangerComponent::xToSamples_(int x) const
{
    if (x < kTrackHeaderWidth)
        return 0;

    int pixelsFromTimelineStart = x - kTrackHeaderWidth;
    double timeSeconds = static_cast<double>(pixelsFromTimelineStart) / kPixelsPerSecond;
    double sr = getProjectSampleRate_();
    return static_cast<int64_t>(std::llround(timeSeconds * sr));
}

void ArrangerComponent::drawTracks_(juce::Graphics& g)
{
    const auto& project = editor_.getProject();

    for (size_t trackIdx = 0; trackIdx < project.tracks.size(); ++trackIdx)
    {
        const auto& track = project.tracks[trackIdx];

        int y = static_cast<int>(trackIdx) * kTrackHeight;

        // Draw track header (left side)
        juce::Rectangle<int> headerRect(0, y, kTrackHeaderWidth, kTrackHeight);
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillRect(headerRect);

        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRect(headerRect, 1);

        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(juce::String(track.name),
                   headerRect.reduced(4),
                   juce::Justification::centredLeft,
                   true);

        // Draw timeline region background
        juce::Rectangle<int> timelineRect(kTrackHeaderWidth, y,
                                          getWidth() - kTrackHeaderWidth, kTrackHeight);
        g.setColour(juce::Colour(0xff242424));
        g.fillRect(timelineRect);

        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRect(timelineRect, 1);

        // Draw clips on this track
        for (size_t clipIdx = 0; clipIdx < track.clips.size(); ++clipIdx)
        {
            const auto& clip = track.clips[clipIdx];

            // Skip muted clips (don't draw)
            if (clip.muted)
                continue;

            // Compute clip bounds
            double sr = getProjectSampleRate_();
            double t0 = static_cast<double>(clip.startSample) / sr;
            double t1 = static_cast<double>(clip.startSample + clip.lengthSamples) / sr;

            float x0 = kTrackHeaderWidth + static_cast<float>(t0 * kPixelsPerSecond);
            float x1 = kTrackHeaderWidth + static_cast<float>(t1 * kPixelsPerSecond);
            float w = x1 - x0;

            if (w <= 0.0f)
                continue; // Skip zero-length clips

            float clipY = static_cast<float>(y + kTimelinePadding);
            float clipH = static_cast<float>(kTrackHeight - 2 * kTimelinePadding);

            juce::Rectangle<float> clipRect(x0, clipY, w, clipH);

            // Clip color: Use HSV with hue based on track index
            float hue = (static_cast<float>(trackIdx) * 0.15f);
            while (hue >= 1.0f)
                hue -= 1.0f;

            juce::Colour clipColour = juce::Colour::fromHSV(hue, 0.6f, 0.7f, 1.0f);

            // Draw clip rectangle
            g.setColour(clipColour);
            g.fillRoundedRectangle(clipRect, 3.0f);

            // Draw clip border
            g.setColour(clipColour.brighter(0.3f));
            g.drawRoundedRectangle(clipRect, 3.0f, 1.0f);

            // Draw clip name
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.setFont(12.0f);

            juce::String clipName = juce::String(clip.filePath);
            if (clipName.isEmpty())
                clipName = "Clip " + juce::String(clip.id);
            else
                clipName = juce::File(clipName).getFileName(); // Extract filename only

            g.drawText(clipName,
                       clipRect.reduced(4.0f),
                       juce::Justification::centredLeft,
                       true);
        }
    }
}

void ArrangerComponent::drawPlayhead_(juce::Graphics& g)
{
    auto playheadSamples = editor_.getPlayheadSamples();
    float px = timeToX_(playheadSamples);

    // Draw vertical line
    g.setColour(juce::Colours::red);
    g.drawLine(px, 0.0f, px, static_cast<float>(getHeight()), 2.0f);

    // Draw playhead triangle at top
    juce::Path triangle;
    triangle.addTriangle(px - 6.0f, 0.0f,
                         px + 6.0f, 0.0f,
                         px, 10.0f);
    g.fillPath(triangle);
}

std::pair<int, int> ArrangerComponent::hitTestClip_(int x, int y) const
{
    // Check if outside timeline region
    if (x < kTrackHeaderWidth)
        return {-1, -1};

    const auto& project = editor_.getProject();

    // Determine which track row we're in
    int trackIdx = y / kTrackHeight;
    if (trackIdx < 0 || trackIdx >= static_cast<int>(project.tracks.size()))
        return {-1, -1};

    const auto& track = project.tracks[trackIdx];

    // Check each clip on this track (in reverse order for overlap handling)
    for (int clipIdx = static_cast<int>(track.clips.size()) - 1; clipIdx >= 0; --clipIdx)
    {
        const auto& clip = track.clips[clipIdx];

        // Skip muted clips
        if (clip.muted)
            continue;

        // Compute clip bounds
        double sr = getProjectSampleRate_();
        double t0 = static_cast<double>(clip.startSample) / sr;
        double t1 = static_cast<double>(clip.startSample + clip.lengthSamples) / sr;

        float x0 = kTrackHeaderWidth + static_cast<float>(t0 * kPixelsPerSecond);
        float x1 = kTrackHeaderWidth + static_cast<float>(t1 * kPixelsPerSecond);

        int clipY = trackIdx * kTrackHeight + kTimelinePadding;
        int clipH = kTrackHeight - 2 * kTimelinePadding;

        juce::Rectangle<int> clipRect(
            static_cast<int>(x0),
            clipY,
            static_cast<int>(x1 - x0),
            clipH
        );

        if (clipRect.contains(x, y))
            return {trackIdx, clipIdx};
    }

    return {-1, -1};
}

} // namespace zenith
