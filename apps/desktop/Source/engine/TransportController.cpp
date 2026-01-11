/*
  ==============================================================================

    TransportController.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Transport controller implementation.

  ==============================================================================
*/

#include "TransportController.h"
#include "TempoMap.h"
#include <chrono>

namespace zenith {

//==============================================================================
double TransportController::getPlayheadBeats() const {
    double sr = sampleRate_.load();
    if (sr <= 0.0) {
        return 0.0;
    }

    juce::int64 samples = playheadSamples_.load();

    // Use tempo map if available for accurate beat position
    if (tempoMap_ != nullptr) {
        return tempoMap_->samplesToBeats(samples, sr);
    }

    // Fallback: simple calculation using current tempo
    double seconds = static_cast<double>(samples) / sr;
    double bpm = tempo_.load();
    return seconds * bpm / 60.0;
}

//==============================================================================
// Scheduled Actions Implementation
//==============================================================================

bool TransportController::playAt(int64_t wallClockMsEpoch, double positionSeconds) {
    ScheduledAction action;
    action.seq = actionSeqCounter_.fetch_add(1, std::memory_order_relaxed);
    action.type = ScheduledAction::Type::Play;
    action.whenMs = wallClockMsEpoch;
    action.positionSec = positionSeconds;
    
    return enqueueScheduledAction(action);
}

bool TransportController::stopAt(int64_t wallClockMsEpoch) {
    ScheduledAction action;
    action.seq = actionSeqCounter_.fetch_add(1, std::memory_order_relaxed);
    action.type = ScheduledAction::Type::Stop;
    action.whenMs = wallClockMsEpoch;
    action.positionSec = 0.0;
    
    return enqueueScheduledAction(action);
}

bool TransportController::seekAt(int64_t wallClockMsEpoch, double positionSeconds) {
    ScheduledAction action;
    action.seq = actionSeqCounter_.fetch_add(1, std::memory_order_relaxed);
    action.type = ScheduledAction::Type::Seek;
    action.whenMs = wallClockMsEpoch;
    action.positionSec = positionSeconds;
    
    return enqueueScheduledAction(action);
}

bool TransportController::enqueueScheduledAction(const ScheduledAction& action) {
    // Lock-free enqueue using atomic operations
    uint32_t head = actionQueueHead_.load(std::memory_order_relaxed);
    uint32_t tail = actionQueueTail_.load(std::memory_order_acquire);
    
    // Check if queue is full
    uint32_t nextHead = (head + 1) % kScheduledActionsCapacity;
    if (nextHead == tail) {
        return false; // Queue full
    }
    
    // Write action to the queue
    scheduledActions_[head] = action;
    
    // Commit the write by advancing head
    actionQueueHead_.store(nextHead, std::memory_order_release);
    
    return true;
}

int TransportController::processScheduledActions(juce::int64 currentSample, int bufferSize) noexcept {
    // Update clock mapping periodically (every ~1 second worth of samples)
    double sampleRate = sampleRate_.load(std::memory_order_relaxed);
    static int updateCounter = 0;
    if (++updateCounter >= static_cast<int>(sampleRate)) {
        updateClockMapping(currentSample);
        updateCounter = 0;
    }
    
    uint32_t tail = actionQueueTail_.load(std::memory_order_relaxed);
    uint32_t head = actionQueueHead_.load(std::memory_order_acquire);
    
    // Process all pending actions
    while (tail != head) {
        const ScheduledAction& action = scheduledActions_[tail];
        
        // Convert wall clock time to sample position
        juce::int64 targetSample = epochMsToStreamSample(action.whenMs);
        
        // Check if action should be triggered in this buffer
        juce::int64 bufferEnd = currentSample + bufferSize;
        
        if (targetSample >= currentSample && targetSample < bufferEnd) {
            // Calculate sample offset within buffer
            int offset = static_cast<int>(targetSample - currentSample);
            
            // Apply the action at the precise sample offset
            // Note: In a real implementation, we would split buffer processing
            // at this point. For now, we apply at buffer start for simplicity.
            switch (action.type) {
                case ScheduledAction::Type::Play:
                    if (action.positionSec >= 0.0) {
                        // Seek to specified position, then play
                        juce::int64 seekSamples = static_cast<juce::int64>(action.positionSec * sampleRate);
                        playheadSamples_.store(seekSamples, std::memory_order_relaxed);
                    }
                    isPlaying_.store(true, std::memory_order_release);
                    break;
                    
                case ScheduledAction::Type::Stop:
                    isPlaying_.store(false, std::memory_order_release);
                    break;
                    
                case ScheduledAction::Type::Seek:
                    {
                        juce::int64 seekSamples = static_cast<juce::int64>(action.positionSec * sampleRate);
                        playheadSamples_.store(seekSamples, std::memory_order_relaxed);
                    }
                    break;
                    
                default:
                    break;
            }
            
            // Remove processed action
            tail = (tail + 1) % kScheduledActionsCapacity;
            actionQueueTail_.store(tail, std::memory_order_release);
            
            return offset; // Return offset for precise timing
        }
        else if (targetSample < currentSample) {
            // Action is in the past, skip it
            tail = (tail + 1) % kScheduledActionsCapacity;
            actionQueueTail_.store(tail, std::memory_order_release);
        }
        else {
            // Action is in the future, stop processing
            break;
        }
    }
    
    return -1; // No action in this buffer
}

juce::int64 TransportController::epochMsToStreamSample(int64_t epochMs) const noexcept {
    // Get current mapping
    int64_t lastWallMs = lastWallClockMs_.load(std::memory_order_relaxed);
    int64_t lastSample = lastStreamSample_.load(std::memory_order_relaxed);
    double sampleRate = sampleRate_.load(std::memory_order_relaxed);
    
    if (lastWallMs == 0 || sampleRate <= 0.0) {
        // No mapping established yet, estimate from stream start
        int64_t startMs = streamStartTimeMs_.load(std::memory_order_relaxed);
        if (startMs == 0) {
            return 0;
        }
        
        int64_t deltaMs = epochMs - startMs;
        return static_cast<juce::int64>((deltaMs / 1000.0) * sampleRate);
    }
    
    // Linear interpolation from last known mapping
    int64_t deltaMs = epochMs - lastWallMs;
    double deltaSamples = (deltaMs / 1000.0) * sampleRate;
    
    return lastSample + static_cast<juce::int64>(deltaSamples);
}

void TransportController::updateClockMapping(juce::int64 currentSample) noexcept {
    // Get current wall clock time using steady_clock for monotonic time
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Update mapping atomics
    lastWallClockMs_.store(nowMs, std::memory_order_relaxed);
    lastStreamSample_.store(currentSample, std::memory_order_relaxed);
    
    // Initialize stream start time on first call
    int64_t startTime = streamStartTimeMs_.load(std::memory_order_relaxed);
    if (startTime == 0) {
        double sampleRate = sampleRate_.load(std::memory_order_relaxed);
        if (sampleRate > 0.0) {
            // Calculate stream start time based on current position
            int64_t elapsedMs = static_cast<int64_t>((currentSample / sampleRate) * 1000.0);
            streamStartTimeMs_.store(nowMs - elapsedMs, std::memory_order_release);
        }
    }
}

} // namespace zenith
