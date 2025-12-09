/**
 * @file EngineTransport.cpp
 * @brief Engine transport controls (Play, Stop, Record, Loop)
 */

#include "../include/Engine.h"
#include "../include/ProjectState.h"
#include "TrackAutomationSynchronizer.h"
#include "TempoMap.h"

namespace zenith {

//==============================================================================
// Transport Controls
//==============================================================================

void Engine::play()
{
    DBG("Engine: Play");
    isPlaying_.store(true);

    // Phase 1.3: Use new playhead system
    // If playhead is at or past loop end, reset to loop start or 0
    const juce::int64 loopEnd = loopEndSamples_.load();
    const juce::int64 loopStart = loopStartSamples_.load();
    const juce::int64 currentPos = playheadSamples_.load();

    if (loopEnd > 0 && currentPos >= loopEnd)
    {
        playheadSamples_.store(loopStart);
    }

    // Enable test tone for Phase 0 fallback (when no tracks)
    enableTestTone_.store(true);

    // Phase 13: Start automation synchronizer
    if (automationSynchronizer)
    {
        automationSynchronizer->start(60);  // 60 Hz update rate
        DBG("Engine: Started automation synchronizer");
    }
}

void Engine::stop()
{
    DBG("Engine: Stop");

    // Phase 2C: If recording, bake recordings into clips first
    if (isRecording_.load())
    {
        stopRecording();
    }

    isPlaying_.store(false);
    enableTestTone_.store(false);

    // Phase 13: Stop automation synchronizer
    if (automationSynchronizer)
    {
        automationSynchronizer->stop();
        DBG("Engine: Stopped automation synchronizer");
    }
}

double Engine::getPlaybackPositionBeats() const
{
    if (projectState_ == nullptr)
        return 0.0;

    const double tempo = projectState_->getTempo();
    const double sampleRate = currentSampleRate.load();
    const juce::int64 positionSamples = playheadSamples_.load();

    // Use TempoMap for accurate conversion
    if (tempoMap_)
    {
        const double seconds = static_cast<double>(positionSamples) / sampleRate;
        return tempoMap_->secondsToBeats(seconds, sampleRate);
    }

    // Fallback
    const double seconds = static_cast<double>(positionSamples) / sampleRate;
    const double beats = (seconds * tempo) / 60.0;

    return beats;
}

void Engine::toggleRecording()
{
    if (isRecording())
    {
        stopRecording();
    }
    else
    {
        record();
    }
}

//==============================================================================
// Phase 1.3: Transport Position & Looping
//==============================================================================

void Engine::setPlayheadSamples(juce::int64 position)
{
    playheadSamples_.store(juce::jmax(juce::int64(0), position));
}

void Engine::setLooping(bool shouldLoop)
{
    isLooping_.store(shouldLoop);
    DBG("Engine: Looping " + juce::String(shouldLoop ? "enabled" : "disabled"));
}

void Engine::setLoopRegion(juce::int64 start, juce::int64 end)
{
    loopStartSamples_.store(juce::jmax(juce::int64(0), start));
    loopEndSamples_.store(juce::jmax(juce::int64(0), end));

    DBG("Engine: Loop region set: " + juce::String(start) + " - " + juce::String(end) + " samples");
}

} // namespace zenith
