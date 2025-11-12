/**
 * @file TrackView.cpp
 * @brief Implementation of virtualized arrangement/track view (W5)
 */

#include "TrackView.h"

//==============================================================================
TrackView::TrackView()
{
    // W5: Start with minimal default tracks (test data added via setSessionData)
    tracks.push_back({"Audio 1", 1});
    tracks.push_back({"Audio 2", 1});
    tracks.push_back({"MIDI 1", 1});
}

//==============================================================================
void TrackView::paint(juce::Graphics& g)
{
    // W5: Zero allocations in paint() - all fonts/paths are cached members

    auto bounds = getLocalBounds();

    // Background
    g.fillAll(ZenithColours::backgroundDark);

    // Timeline ruler at top
    auto timelineBounds = bounds.removeFromTop(timelineHeight);
    drawTimelineRuler(g, timelineBounds);

    // Tracks (virtualized: only visible range)
    drawTracks(g, bounds);

    // Loop region (if enabled and visible)
    if (loopEnabled)
    {
        drawLoopRegion(g);
    }

    // Playhead (only if visible)
    drawPlayhead(g);

    #if JUCE_DEBUG
        if (showDebugOverlay)
            drawDebugOverlay(g);
    #endif
}

void TrackView::resized()
{
    // W5: No layout needed - custom-drawn component
}

//==============================================================================
void TrackView::mouseDown(const juce::MouseEvent& event)
{
    // Check if click is in timeline
    if (event.y < timelineHeight)
    {
        double clickTime = xToTime(static_cast<double>(event.x));
        if (onPlayheadClicked)
            onPlayheadClicked(clickTime);
        return;
    }

    // W5: Check which track was clicked (using visible range)
    auto visibleTracks = visibleTrackIndexRange();
    int localY = event.y - timelineHeight;
    int trackIndex = static_cast<int>((localY + verticalOffsetPx) / trackHeight);

    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()))
    {
        if (onTrackSelected)
            onTrackSelected(trackIndex);
    }
}

void TrackView::mouseDrag(const juce::MouseEvent& event)
{
    // W5: Panning with drag (optional enhancement for later)
    // For now, use wheel scroll
}

void TrackView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // W5 Spec:
    // - Normal wheel = horizontal scroll
    // - Shift+Wheel = vertical scroll
    // - Ctrl+Wheel = horizontal zoom around mouse cursor

    bool hasCtrl = event.mods.isCommandDown() || event.mods.isCtrlDown();
    bool hasShift = event.mods.isShiftDown();

    if (hasCtrl)
    {
        // Zoom around mouse cursor (W5 requirement)
        double mouseX = static_cast<double>(event.x);
        double mouseTime = xToTime(mouseX);  // Time at mouse position

        // Adjust zoom
        double zoomFactor = 1.0 + (wheel.deltaY * 0.2);
        double newPixelsPerSecond = pixelsPerSecond * zoomFactor;
        newPixelsPerSecond = juce::jlimit(10.0, 500.0, newPixelsPerSecond);

        // Adjust scroll to keep mouse time at same screen position
        double newMouseX = timeToX(mouseTime);
        timeOffsetPx += (mouseX - newMouseX);

        pixelsPerSecond = newPixelsPerSecond;
        timeOffsetPx = juce::jmax(0.0, timeOffsetPx);

        repaint();
    }
    else if (hasShift)
    {
        // Shift+Wheel = vertical scroll (W5 spec)
        verticalOffsetPx -= wheel.deltaY * 50.0;
        verticalOffsetPx = juce::jmax(0.0, verticalOffsetPx);
        repaint();
    }
    else
    {
        // Normal wheel = horizontal scroll (W5 spec)
        timeOffsetPx -= wheel.deltaY * 50.0;
        timeOffsetPx = juce::jmax(0.0, timeOffsetPx);
        repaint();
    }
}

//==============================================================================
// W5: Virtualized drawing (only visible ranges)
//==============================================================================

void TrackView::drawTimelineRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(ZenithColours::backgroundMedium);
    g.fillRect(bounds);

    // Border
    g.setColour(ZenithColours::border);
    g.drawLine(0.0f, static_cast<float>(bounds.getBottom()),
               static_cast<float>(bounds.getRight()),
               static_cast<float>(bounds.getBottom()), 1.0f);

    // W5: Use cached font (zero allocations)
    g.setFont(rulerFont);
    g.setColour(ZenithColours::textSecondary);

    // W5: Virtualized - only draw visible time range
    auto visibleTime = visibleTimeSecondsRange();
    double secondsPerBar = (60.0 / currentBPM) * 4.0;  // 4/4 time signature

    int startBar = static_cast<int>(visibleTime.getStart() / secondsPerBar);
    int endBar = static_cast<int>(visibleTime.getEnd() / secondsPerBar) + 2;

    for (int bar = startBar; bar < endBar; ++bar)
    {
        double barTime = bar * secondsPerBar;
        double x = timeToX(barTime);

        if (x >= trackHeaderWidth && x < getWidth())
        {
            // Major grid line
            g.setColour(ZenithColours::border);
            g.drawLine(static_cast<float>(x),
                      static_cast<float>(bounds.getBottom()) - 8.0f,
                      static_cast<float>(x),
                      static_cast<float>(bounds.getBottom()), 2.0f);

            // Bar number
            g.setColour(ZenithColours::textSecondary);
            g.drawText(juce::String(bar + 1),
                      static_cast<int>(x) - 20, bounds.getY() + 4,
                      40, 20, juce::Justification::centred);

            // Beat markers
            double secondsPerBeat = 60.0 / currentBPM;
            for (int beat = 1; beat < 4; ++beat)
            {
                double beatTime = barTime + (beat * secondsPerBeat);
                double beatX = timeToX(beatTime);

                if (beatX >= trackHeaderWidth && beatX < getWidth())
                {
                    g.setColour(ZenithColours::border.withAlpha(0.5f));
                    g.drawLine(static_cast<float>(beatX),
                              static_cast<float>(bounds.getBottom()) - 4.0f,
                              static_cast<float>(beatX),
                              static_cast<float>(bounds.getBottom()), 1.0f);
                }
            }
        }
    }
}

void TrackView::drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // W5: VIRTUALIZED - only draw visible tracks (critical for performance)
    auto visibleTracks = visibleTrackIndexRange();

    if (visibleTracks.isEmpty())
        return;

    // W5: Use cached font (zero allocations)
    g.setFont(trackNameFont);

    for (int i = visibleTracks.getStart(); i < visibleTracks.getEnd(); ++i)
    {
        if (i >= static_cast<int>(tracks.size()))
            break;

        // Calculate Y position for this track
        double y = bounds.getY() + (i * trackHeight) - verticalOffsetPx;

        // Track background (alternating colors)
        juce::Colour trackColor = (i % 2 == 0)
            ? ZenithColours::backgroundDark
            : ZenithColours::backgroundDark.brighter(0.05f);

        g.setColour(trackColor);
        g.fillRect(0.0f, static_cast<float>(y),
                  static_cast<float>(getWidth()), static_cast<float>(trackHeight));

        // Track header
        g.setColour(ZenithColours::backgroundMedium);
        g.fillRect(0.0f, static_cast<float>(y),
                  static_cast<float>(trackHeaderWidth), static_cast<float>(trackHeight));

        // Track name
        g.setColour(ZenithColours::textPrimary);
        g.drawText(tracks[i].name, 8, static_cast<int>(y),
                  trackHeaderWidth - 16, trackHeight, juce::Justification::centredLeft);

        // Separator line
        g.setColour(ZenithColours::border);
        g.drawLine(static_cast<float>(trackHeaderWidth), static_cast<float>(y),
                  static_cast<float>(getWidth()), static_cast<float>(y), 1.0f);

        // W5: Draw grid lines (virtualized - only visible time range)
        auto visibleTime = visibleTimeSecondsRange();
        double secondsPerBar = (60.0 / currentBPM) * 4.0;

        int startBar = static_cast<int>(visibleTime.getStart() / secondsPerBar);
        int endBar = static_cast<int>(visibleTime.getEnd() / secondsPerBar) + 2;

        g.setColour(ZenithColours::border.withAlpha(0.2f));
        for (int bar = startBar; bar < endBar; ++bar)
        {
            double barTime = bar * secondsPerBar;
            double x = timeToX(barTime);

            if (x >= trackHeaderWidth && x < getWidth())
            {
                g.drawLine(static_cast<float>(x), static_cast<float>(y),
                          static_cast<float>(x), static_cast<float>(y + trackHeight), 1.0f);
            }
        }

        // W5: Draw clips for this track (virtualized - only visible time range)
        for (const auto& clip : clips)
        {
            if (clip.trackIndex != i)
                continue;

            // Convert clip times
            double clipStart = clip.startSamples / sampleRate;
            double clipEnd = (clip.startSamples + clip.lengthSamples) / sampleRate;

            // Check if clip is visible
            if (clipEnd < visibleTime.getStart() || clipStart > visibleTime.getEnd())
                continue;

            double clipX1 = timeToX(clipStart);
            double clipX2 = timeToX(clipEnd);

            // Clamp to visible area
            clipX1 = juce::jmax(clipX1, static_cast<double>(trackHeaderWidth));
            clipX2 = juce::jmin(clipX2, static_cast<double>(getWidth()));

            if (clipX2 > clipX1)
            {
                // Draw clip rectangle
                g.setColour(clip.colour.withAlpha(0.7f));
                g.fillRoundedRectangle(static_cast<float>(clipX1),
                                      static_cast<float>(y + 4),
                                      static_cast<float>(clipX2 - clipX1),
                                      static_cast<float>(trackHeight - 8), 4.0f);

                // Clip border
                g.setColour(clip.colour);
                g.drawRoundedRectangle(static_cast<float>(clipX1),
                                      static_cast<float>(y + 4),
                                      static_cast<float>(clipX2 - clipX1),
                                      static_cast<float>(trackHeight - 8), 4.0f, 1.5f);

                // Clip name (if wide enough)
                if (clipX2 - clipX1 > 50)
                {
                    g.setColour(ZenithColours::textPrimary);
                    g.setFont(juce::Font(12.0f));
                    g.drawText(clip.name, static_cast<int>(clipX1) + 8,
                              static_cast<int>(y + 4), static_cast<int>(clipX2 - clipX1) - 16,
                              trackHeight - 8, juce::Justification::centredLeft, true);
                }
            }
        }
    }
}

void TrackView::drawPlayhead(juce::Graphics& g)
{
    double x = timeToX(playheadSeconds);

    // W5: Only draw if visible
    if (x < trackHeaderWidth || x >= getWidth())
        return;

    // Draw playhead line
    g.setColour(ZenithColours::accent);
    g.drawLine(static_cast<float>(x), 0.0f,
              static_cast<float>(x), static_cast<float>(getHeight()), 2.0f);

    // W5: Draw playhead triangle using cached path (zero allocations)
    if (!playheadTriangleInitialized)
    {
        playheadTriangle.addTriangle(-6.0f, 0.0f, 6.0f, 0.0f, 0.0f, 8.0f);
        playheadTriangleInitialized = true;
    }

    auto transform = juce::AffineTransform::translation(static_cast<float>(x), 0.0f);
    g.fillPath(playheadTriangle, transform);
}

void TrackView::drawLoopRegion(juce::Graphics& g)
{
    double startX = timeToX(loopStartSeconds);
    double endX = timeToX(loopEndSeconds);

    // W5: Only draw if visible
    if (endX < trackHeaderWidth || startX >= getWidth())
        return;

    // Clamp to visible area
    startX = juce::jmax(startX, static_cast<double>(trackHeaderWidth));
    endX = juce::jmin(endX, static_cast<double>(getWidth()));

    if (endX > startX)
    {
        // Loop region highlight
        g.setColour(ZenithColours::accent.withAlpha(0.1f));
        g.fillRect(static_cast<float>(startX), static_cast<float>(timelineHeight),
                  static_cast<float>(endX - startX),
                  static_cast<float>(getHeight() - timelineHeight));

        // Loop markers
        g.setColour(ZenithColours::accent);
        g.drawLine(static_cast<float>(startX), 0.0f,
                  static_cast<float>(startX), static_cast<float>(getHeight()), 2.0f);
        g.drawLine(static_cast<float>(endX), 0.0f,
                  static_cast<float>(endX), static_cast<float>(getHeight()), 2.0f);
    }
}

#if JUCE_DEBUG
void TrackView::drawDebugOverlay(juce::Graphics& g)
{
    // W5: Debug overlay (JUCE_DEBUG only)
    g.setFont(debugFont);
    g.setColour(juce::Colours::yellow.withAlpha(0.9f));

    auto visibleTracks = visibleTrackIndexRange();
    auto visibleTime = visibleTimeSecondsRange();

    int visibleClips = 0;
    for (const auto& clip : clips)
    {
        double clipStart = clip.startSamples / sampleRate;
        double clipEnd = (clip.startSamples + clip.lengthSamples) / sampleRate;

        if (clip.trackIndex >= visibleTracks.getStart() &&
            clip.trackIndex < visibleTracks.getEnd() &&
            clipEnd >= visibleTime.getStart() &&
            clipStart <= visibleTime.getEnd())
        {
            visibleClips++;
        }
    }

    juce::String debugText =
        "W5 Debug Overlay\n"
        "Visible Tracks: " + juce::String(visibleTracks.getStart()) + "-" +
                            juce::String(visibleTracks.getEnd()) + " / " +
                            juce::String(tracks.size()) + "\n"
        "Visible Time: " + juce::String(visibleTime.getStart(), 2) + "s - " +
                          juce::String(visibleTime.getEnd(), 2) + "s\n"
        "Visible Clips: " + juce::String(visibleClips) + " / " + juce::String(clips.size()) + "\n"
        "Zoom: " + juce::String(pixelsPerSecond, 1) + " px/sec\n"
        "Scroll: H=" + juce::String(timeOffsetPx, 0) + "px, V=" +
                       juce::String(verticalOffsetPx, 0) + "px";

    // Background for readability
    auto textBounds = juce::Rectangle<int>(10, timelineHeight + 10, 300, 120);
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRect(textBounds);

    g.setColour(juce::Colours::yellow);
    g.drawMultiLineText(debugText, 15, timelineHeight + 25, 290);

    // Outline visible tracks
    for (int i = visibleTracks.getStart(); i < visibleTracks.getEnd(); ++i)
    {
        double y = timelineHeight + (i * trackHeight) - verticalOffsetPx;
        g.setColour(juce::Colours::green.withAlpha(0.3f));
        g.drawRect(0.0f, static_cast<float>(y),
                  static_cast<float>(getWidth()), static_cast<float>(trackHeight), 2.0f);
    }
}
#endif

//==============================================================================
// W5: Coordinate system helpers
//==============================================================================

double TrackView::timeToX(double seconds) const
{
    return (seconds * pixelsPerSecond) + trackHeaderWidth - timeOffsetPx;
}

double TrackView::xToTime(double x) const
{
    return (x - trackHeaderWidth + timeOffsetPx) / pixelsPerSecond;
}

juce::Range<int> TrackView::visibleTrackIndexRange() const
{
    int firstVisible = static_cast<int>(verticalOffsetPx / trackHeight);
    int lastVisible = static_cast<int>((verticalOffsetPx + getHeight() - timelineHeight) / trackHeight) + 1;

    firstVisible = juce::jmax(0, firstVisible);
    lastVisible = juce::jmin(lastVisible, static_cast<int>(tracks.size()));

    return juce::Range<int>(firstVisible, lastVisible);
}

juce::Range<double> TrackView::visibleTimeSecondsRange() const
{
    double startTime = xToTime(trackHeaderWidth);
    double endTime = xToTime(static_cast<double>(getWidth()));

    return juce::Range<double>(juce::jmax(0.0, startTime), juce::jmax(0.0, endTime));
}

//==============================================================================
// Public API
//==============================================================================

void TrackView::setPlayheadPosition(double positionInSeconds)
{
    playheadSeconds = positionInSeconds;
    repaint();
}

void TrackView::setBPM(double bpm)
{
    currentBPM = bpm;
    repaint();
}

void TrackView::setPixelsPerSecond(double zoom)
{
    pixelsPerSecond = juce::jlimit(10.0, 500.0, zoom);
    repaint();
}

void TrackView::setHorizontalOffset(double offset)
{
    timeOffsetPx = juce::jmax(0.0, offset);
    repaint();
}

void TrackView::setVerticalOffset(double offset)
{
    verticalOffsetPx = juce::jmax(0.0, offset);
    repaint();
}

void TrackView::setLoopRegion(double startInSeconds, double endInSeconds)
{
    loopStartSeconds = startInSeconds;
    loopEndSeconds = endInSeconds;
    loopEnabled = true;
    repaint();
}

void TrackView::setSessionData(std::vector<Track> newTracks, std::vector<Clip> newClips)
{
    tracks = std::move(newTracks);
    clips = std::move(newClips);
    repaint();
}
