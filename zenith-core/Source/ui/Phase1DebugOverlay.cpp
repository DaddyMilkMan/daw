/**
 * @file Phase1DebugOverlay.cpp
 * @brief Phase 1 debug HUD implementation
 */

#include "Phase1DebugOverlay.h"

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO && JUCE_DEBUG

//==============================================================================
// Constructor / Destructor
//==============================================================================

Phase1DebugOverlay::Phase1DebugOverlay(Engine& engine)
    : engine_(engine)
{
    // Non-interactive (click-through)
    setInterceptsMouseClicks(false, false);

    // Start timer at 50ms (~20 FPS)
    startTimer(50);

    // Set initial size
    setSize(300, 200);
}

Phase1DebugOverlay::~Phase1DebugOverlay()
{
    stopTimer();
}

//==============================================================================
// Timer Callback
//==============================================================================

void Phase1DebugOverlay::timerCallback()
{
    // Poll metrics from engine (RT-safe via atomics)
    metrics_ = engine_.getPhase1DebugMetrics();

    // Trigger repaint
    repaint();
}

//==============================================================================
// Rendering
//==============================================================================

void Phase1DebugOverlay::paint(juce::Graphics& g)
{
    // Semi-transparent black background
    g.fillAll(juce::Colour(0x80000000));

    // White border
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);

    // Set font
    g.setFont(monoFont_);

    // Text content
    int y = 10;
    const int lineHeight = 16;
    const int leftMargin = 10;

    // Title
    g.setColour(juce::Colours::yellow);
    g.drawText("Phase 1 Debug HUD", leftMargin, y, getWidth() - 20, lineHeight,
              juce::Justification::left);
    y += lineHeight + 5;

    // Separator
    g.setColour(juce::Colours::white);
    g.drawHorizontalLine(y, (float)leftMargin, (float)(getWidth() - leftMargin));
    y += 10;

    // Metrics (white text)
    g.setColour(juce::Colours::white);

    // Transport position
    juce::String timeStr = samplesToTimeString(metrics_.transportSamples, metrics_.sampleRate);
    g.drawText("Transport: " + juce::String(metrics_.transportSamples) + " samples",
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    g.drawText("           " + timeStr + " @ " + juce::String(metrics_.sampleRate / 1000.0, 1) + " kHz",
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    // State
    juce::String stateStr = metrics_.isPlaying ? "Playing" : "Stopped";
    juce::Colour stateColor = metrics_.isPlaying ? juce::Colours::green : juce::Colours::grey;
    g.setColour(stateColor);
    g.drawText("State:     " + stateStr,
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    g.setColour(juce::Colours::white);

    // Tracks
    g.drawText("Tracks:    " + juce::String(metrics_.numTracks),
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    // Voices
    g.drawText("Voices:    " + juce::String(metrics_.activeVoices) + " active",
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    // Events
    g.drawText("Events:    queued=" + juce::String(metrics_.queuedEvents) +
              " dropped=" + juce::String(metrics_.droppedEvents),
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    // Peak levels (with color coding)
    juce::String peakLStr = peakToDbString(metrics_.peakL);
    juce::String peakRStr = peakToDbString(metrics_.peakR);

    // Color code peaks (red if > -3dB, yellow if > -12dB, green otherwise)
    auto getPeakColor = [](float peak) -> juce::Colour {
        float db = peak > 0.00001f ? juce::Decibels::gainToDecibels(peak) : -100.0f;
        if (db > -3.0f) return juce::Colours::red;
        if (db > -12.0f) return juce::Colours::yellow;
        return juce::Colours::green;
    };

    g.setColour(getPeakColor(metrics_.peakL));
    g.drawText("Peak L:    " + peakLStr,
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    g.setColour(getPeakColor(metrics_.peakR));
    g.drawText("Peak R:    " + peakRStr,
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
    y += lineHeight;

    // Footer hint
    y += 5;
    g.setColour(juce::Colours::grey);
    g.drawText("Press F12 to toggle",
              leftMargin, y, getWidth() - 20, lineHeight, juce::Justification::left);
}

void Phase1DebugOverlay::resized()
{
    // Fixed size overlay, no child components
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::String Phase1DebugOverlay::peakToDbString(float peak)
{
    if (peak < 0.00001f)
        return "-inf dB";

    float db = juce::Decibels::gainToDecibels(peak);
    return juce::String(db, 1) + " dB";
}

juce::String Phase1DebugOverlay::samplesToTimeString(int64_t samples, double sampleRate)
{
    if (sampleRate <= 0.0)
        return "0:00.000";

    double seconds = (double)samples / sampleRate;
    int minutes = (int)(seconds / 60.0);
    seconds -= minutes * 60.0;

    int secs = (int)seconds;
    int millis = (int)((seconds - secs) * 1000.0);

    return juce::String(minutes) + ":" +
           juce::String(secs).paddedLeft('0', 2) + "." +
           juce::String(millis).paddedLeft('0', 3);
}

#endif // ZENITH_ENABLE_PHASE1_AUDIO && JUCE_DEBUG
