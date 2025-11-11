/**
 * @file TrackView.cpp
 * @brief Implementation of arrangement/track view component
 */

#include "TrackView.h"

//==============================================================================
TrackView::TrackView()
{
    // Add some default tracks for testing
    addTrack("Audio 1");
    addTrack("Audio 2");
    addTrack("MIDI 1");

    // Start timer for playhead updates (60 Hz)
    startTimer(16);
}

TrackView::~TrackView()
{
    stopTimer();
}

void TrackView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(ZenithColours::backgroundDark);

    // Timeline ruler at top
    auto timelineBounds = bounds.removeFromTop(timelineHeight);
    drawTimelineRuler(g, timelineBounds);

    // Tracks
    drawTracks(g, bounds);

    // Loop region (if enabled)
    if (loopEnabled)
    {
        drawLoopRegion(g);
    }

    // Playhead (drawn last, on top)
    drawPlayhead(g);
}

void TrackView::resized()
{
    // Layout is handled in paint() for this custom-drawn component
}

void TrackView::mouseDown(const juce::MouseEvent& event)
{
    // Check if click is in timeline
    if (event.y < timelineHeight)
    {
        double clickTime = xToTime(event.x);
        if (onPlayheadClicked)
            onPlayheadClicked(clickTime);
    }
    else
    {
        // Check which track was clicked
        int trackY = event.y - timelineHeight;
        int trackIndex = (int)(trackY / (defaultTrackHeight * verticalZoom));

        if (juce::isPositiveAndBelow(trackIndex, trackNames.size()))
        {
            if (onTrackSelected)
                onTrackSelected(trackIndex);
        }
    }
}

void TrackView::mouseDrag(const juce::MouseEvent& event)
{
    // TODO: Implement clip dragging
}

void TrackView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Horizontal scroll
    if (event.mods.isShiftDown())
    {
        scrollX -= wheel.deltaY * 50.0f;
        scrollX = juce::jmax(0.0f, scrollX);
        repaint();
    }
    // Vertical scroll
    else
    {
        scrollY -= wheel.deltaY * 50.0f;
        scrollY = juce::jmax(0.0f, scrollY);
        repaint();
    }
}

//==============================================================================
void TrackView::timerCallback()
{
    if (isPlaying)
    {
        repaint(); // Repaint to update playhead position
    }
}

//==============================================================================
void TrackView::drawTimelineRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(ZenithColours::backgroundMedium);
    g.fillRect(bounds);

    // Draw border
    g.setColour(ZenithColours::border);
    g.drawLine(0.0f, (float)bounds.getBottom(), (float)bounds.getRight(), (float)bounds.getBottom(), 1.0f);

    // Draw time markers
    g.setColour(ZenithColours::textSecondary);
    g.setFont(juce::Font(11.0f));

    // Draw bar numbers
    int visibleBars = (int)(getWidth() / (horizontalZoom * 4)) + 2; // 4 quarter notes per bar

    for (int bar = 0; bar < visibleBars; ++bar)
    {
        float x = timeToX(bar * 4.0) - scrollX;

        if (x >= 0 && x < getWidth())
        {
            // Major grid line
            g.setColour(ZenithColours::border);
            g.drawLine(x, (float)bounds.getBottom() - 8, x, (float)bounds.getBottom(), 2.0f);

            // Bar number
            g.setColour(ZenithColours::textSecondary);
            g.drawText(juce::String(bar + 1), (int)x - 20, bounds.getY() + 4, 40, 20, juce::Justification::centred);

            // Beat markers (1/4 notes)
            for (int beat = 1; beat < 4; ++beat)
            {
                float beatX = timeToX(bar * 4.0 + beat) - scrollX;
                if (beatX >= 0 && beatX < getWidth())
                {
                    g.setColour(ZenithColours::border.withAlpha(0.5f));
                    g.drawLine(beatX, (float)bounds.getBottom() - 4, beatX, (float)bounds.getBottom(), 1.0f);
                }
            }
        }
    }
}

void TrackView::drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float y = bounds.getY() - scrollY;
    int trackHeight = (int)(defaultTrackHeight * verticalZoom);

    for (int i = 0; i < trackNames.size(); ++i)
    {
        // Track background (alternating colors)
        juce::Colour trackColor = (i % 2 == 0) ? ZenithColours::backgroundDark : ZenithColours::backgroundDark.brighter(0.05f);
        g.setColour(trackColor);
        g.fillRect(0.0f, y, (float)getWidth(), (float)trackHeight);

        // Track header
        g.setColour(ZenithColours::backgroundMedium);
        g.fillRect(0.0f, y, (float)trackHeaderWidth, (float)trackHeight);

        // Track name
        g.setColour(ZenithColours::textPrimary);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(trackNames[i], 8, (int)y, trackHeaderWidth - 16, trackHeight, juce::Justification::centredLeft);

        // Separator line
        g.setColour(ZenithColours::border);
        g.drawLine((float)trackHeaderWidth, y, (float)getWidth(), y, 1.0f);

        // Grid lines (bar markers)
        g.setColour(ZenithColours::border.withAlpha(0.2f));
        int visibleBars = (int)(getWidth() / (horizontalZoom * 4)) + 2;

        for (int bar = 0; bar < visibleBars; ++bar)
        {
            float x = timeToX(bar * 4.0) - scrollX;
            if (x >= trackHeaderWidth && x < getWidth())
            {
                g.drawLine(x, y, x, y + trackHeight, 1.0f);
            }
        }

        // TODO: Draw clips for this track

        y += trackHeight;

        // Stop if we've gone past the visible area
        if (y > bounds.getBottom())
            break;
    }
}

void TrackView::drawPlayhead(juce::Graphics& g)
{
    float x = timeToX(playheadPosition) - scrollX;

    if (x >= trackHeaderWidth && x < getWidth())
    {
        // Draw playhead line
        g.setColour(ZenithColours::accent);
        g.drawLine(x, 0.0f, x, (float)getHeight(), 2.0f);

        // Draw playhead triangle at top
        juce::Path triangle;
        triangle.addTriangle(x - 6, 0, x + 6, 0, x, 8);
        g.fillPath(triangle);
    }
}

void TrackView::drawLoopRegion(juce::Graphics& g)
{
    float startX = timeToX(loopStart) - scrollX;
    float endX = timeToX(loopEnd) - scrollX;

    if (endX >= trackHeaderWidth && startX < getWidth())
    {
        // Clamp to visible area
        startX = juce::jmax(startX, (float)trackHeaderWidth);
        endX = juce::jmin(endX, (float)getWidth());

        // Draw loop region highlight
        g.setColour(ZenithColours::accent.withAlpha(0.1f));
        g.fillRect(startX, (float)timelineHeight, endX - startX, (float)(getHeight() - timelineHeight));

        // Draw loop markers
        g.setColour(ZenithColours::accent);
        g.drawLine(startX, 0.0f, startX, (float)getHeight(), 2.0f);
        g.drawLine(endX, 0.0f, endX, (float)getHeight(), 2.0f);
    }
}

//==============================================================================
float TrackView::timeToX(double timeInQuarterNotes) const
{
    return (float)(timeInQuarterNotes * horizontalZoom) + trackHeaderWidth;
}

double TrackView::xToTime(float x) const
{
    return (x - trackHeaderWidth) / horizontalZoom;
}

//==============================================================================
void TrackView::setPlayheadPosition(double positionInQuarterNotes)
{
    playheadPosition = positionInQuarterNotes;
    repaint();
}

void TrackView::setBPM(double bpm)
{
    currentBPM = bpm;
    repaint();
}

void TrackView::setHorizontalZoom(float zoom)
{
    horizontalZoom = juce::jlimit(10.0f, 200.0f, zoom);
    repaint();
}

void TrackView::setVerticalZoom(float zoom)
{
    verticalZoom = juce::jlimit(0.5f, 3.0f, zoom);
    repaint();
}

void TrackView::addTrack(const juce::String& trackName)
{
    trackNames.add(trackName);
    repaint();
}

void TrackView::removeTrack(int trackIndex)
{
    if (juce::isPositiveAndBelow(trackIndex, trackNames.size()))
    {
        trackNames.remove(trackIndex);
        repaint();
    }
}

void TrackView::setLoopRegion(double startInQuarterNotes, double endInQuarterNotes)
{
    loopStart = startInQuarterNotes;
    loopEnd = endInQuarterNotes;
    loopEnabled = true;
    repaint();
}
