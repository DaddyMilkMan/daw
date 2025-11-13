/**
 * @file StatsOverlay.cpp
 * @brief Implementation of performance monitoring HUD
 */

#include "StatsOverlay.h"
#include "ZenithLookAndFeel.h"
#include "../../include/Engine.h"

#if ZENITH_ENABLE_PHASE1_AUDIO
    #include "../engine/Track.h"
#endif

//==============================================================================
StatsOverlay::StatsOverlay(Engine& engine)
    : engine_(engine)
{
    // Start timer at ~15 Hz (66ms) for stat updates
    startTimer(66);

    // Initialize cached text
    cachedStatsText = "Stats Overlay\nInitializing...";

    // Set initial size (will be repositioned by MainComponent)
    setSize(260, 200);  // Increased height for audio stats
}

//==============================================================================
void StatsOverlay::paint(juce::Graphics& g)
{
    // W6: Zero allocations in paint() - all text is pre-computed in timerCallback()

    // Translucent background (rounded rect)
    auto bounds = getLocalBounds().toFloat();
    g.setColour(ZenithColours::backgroundDark.withAlpha(0.85f));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Border
    g.setColour(ZenithColours::border.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    // Title
    g.setFont(labelFont);
    g.setColour(ZenithColours::accent);
    g.drawText("PERF STATS", bounds.removeFromTop(20).reduced(8, 2),
               juce::Justification::centredLeft, false);

    // Stats text (pre-computed in timer)
    auto textBounds = bounds.reduced(8, 4);
    g.setFont(valueFont);
    g.setColour(ZenithColours::textPrimary);
    g.drawText(cachedStatsText, textBounds, juce::Justification::topLeft, false);
}

void StatsOverlay::resized()
{
    // No child components, nothing to layout
}

//==============================================================================
void StatsOverlay::recordPaint(const char* componentName, double paintTimeMs)
{
    // W6: Record paint event in ring buffer (zero allocations)

    // Update TrackView-specific timing if this is TrackView
    if (std::strcmp(componentName, "TrackView") == 0)
    {
        lastTrackViewPaintMs = paintTimeMs;
    }

    // Add sample to ring buffer
    auto& sample = frameBuffer[bufferHead];
    sample.timestampMs = juce::Time::getMillisecondCounterHiRes();
    sample.paintTimeMs = paintTimeMs;

    bufferHead = (bufferHead + 1) % RING_BUFFER_SIZE;
    if (bufferCount < RING_BUFFER_SIZE)
        bufferCount++;
}

void StatsOverlay::updateTrackViewStats(int visibleTracks, int visibleClips)
{
    lastVisibleTracks = visibleTracks;
    lastVisibleClips = visibleClips;
}

void StatsOverlay::setOverlayVisible(bool shouldBeVisible)
{
    setVisible(shouldBeVisible);
}

//==============================================================================
void StatsOverlay::timerCallback()
{
    // W6: Update cached stats text at ~15 Hz (no allocations in paint())

    // Calculate stats for last 1s window
    int paintsPerSec = 0;
    double avgMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;

    calculateStats(paintsPerSec, avgMs, minMs, maxMs);

    // Dirty-check: only update if values changed significantly
    bool needsUpdate = false;

    if (paintsPerSec != displayPaintsPerSec ||
        std::abs(avgMs - displayAvgMs) > 0.01 ||
        std::abs(minMs - displayMinMs) > 0.01 ||
        std::abs(maxMs - displayMaxMs) > 0.01)
    {
        displayPaintsPerSec = paintsPerSec;
        displayAvgMs = avgMs;
        displayMinMs = minMs;
        displayMaxMs = maxMs;
        needsUpdate = true;
    }

    if (needsUpdate)
    {
        // Build stats text (allocates String, but only at 15 Hz, not in paint)
        juce::String text;

        // Frame stats
        text << "Paints/sec: " << displayPaintsPerSec << "\n";
        text << "Frame: avg " << juce::String(displayAvgMs, 2) << "ms"
             << " / min " << juce::String(displayMinMs, 2) << "ms"
             << " / max " << juce::String(displayMaxMs, 2) << "ms\n";
        text << "\n";
        text << "TrackView: " << juce::String(lastTrackViewPaintMs, 2) << "ms\n";
        text << "Tracks: " << lastVisibleTracks << " / Clips: " << lastVisibleClips;

        #if ZENITH_ENABLE_PHASE1_AUDIO
            // Audio engine stats (W13.1 Debug HUD)
            text << "\n\n--- AUDIO ENGINE ---\n";

            // Transport position
            const juce::int64 transportSamples = engine_.getTransportSamples();
            const double transportSeconds = transportSamples / juce::jmax(1.0, engine_.getSampleRate());
            text << "Transport: " << juce::String(transportSamples) << " samples\n";
            text << "           " << juce::String(transportSeconds, 2) << " sec\n";

            // Track/clip counts
            const int numTracks = engine_.getNumTracks();
            int totalClips = 0;
            for (int i = 0; i < numTracks; ++i)
            {
                if (auto* track = engine_.getTrack(i))
                    totalClips += track->getNumClips();
            }
            text << "Tracks: " << numTracks << " / Total clips: " << totalClips << "\n";

            #if JUCE_DEBUG
                // Peak meters (only in debug builds)
                const float peakL = engine_.getLastPeakL();
                const float peakR = engine_.getLastPeakR();
                const float peakLdB = peakL > 0.0f ? 20.0f * std::log10(peakL) : -96.0f;
                const float peakRdB = peakR > 0.0f ? 20.0f * std::log10(peakR) : -96.0f;

                text << "Peak L: " << juce::String(peakLdB, 1) << " dB\n";
                text << "Peak R: " << juce::String(peakRdB, 1) << " dB";
            #endif
        #endif

        cachedStatsText = text;
        repaint();
    }
}

//==============================================================================
void StatsOverlay::calculateStats(int& outPaintsPerSec,
                                  double& outAvgMs,
                                  double& outMinMs,
                                  double& outMaxMs) const
{
    // W6: Calculate stats for last 1s window (zero allocations)

    if (bufferCount == 0)
    {
        outPaintsPerSec = 0;
        outAvgMs = 0.0;
        outMinMs = 0.0;
        outMaxMs = 0.0;
        return;
    }

    double nowMs = juce::Time::getMillisecondCounterHiRes();
    double windowStartMs = nowMs - 1000.0;  // 1 second window

    // Iterate backwards from head to find samples in last 1s
    int count = 0;
    double sum = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = 0.0;

    for (size_t i = 0; i < bufferCount; ++i)
    {
        // Walk backwards from head
        size_t idx = (bufferHead + RING_BUFFER_SIZE - 1 - i) % RING_BUFFER_SIZE;
        const auto& sample = frameBuffer[idx];

        // Stop if sample is older than 1s
        if (sample.timestampMs < windowStartMs)
            break;

        count++;
        sum += sample.paintTimeMs;
        min = std::min(min, sample.paintTimeMs);
        max = std::max(max, sample.paintTimeMs);
    }

    outPaintsPerSec = count;  // Samples in last 1s ≈ paints/sec
    outAvgMs = (count > 0) ? (sum / count) : 0.0;
    outMinMs = (count > 0) ? min : 0.0;
    outMaxMs = (count > 0) ? max : 0.0;
}
