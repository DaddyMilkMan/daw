/*
  ==============================================================================

    AudioToUIFifo.h
    Created: 2025-12-29
    Author:  Zenith DAW

    Lock-free FIFO for transferring state from Audio Thread to UI Thread.
    Uses juce::AbstractFifo for thread-safe ring buffering.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

//==============================================================================
// Event Types
//==============================================================================
struct PlayheadEvent {
    int trackIndex;
    float normalizedPosition; // 0.0 to 1.0 progress within clip
};

struct ClipStateEvent {
    int trackIndex;
    int sceneIndex;
    bool isPlaying;
    bool isQueued;
    bool isRecording;
};

//==============================================================================
// AudioToUIFifo
//==============================================================================
class AudioToUIFifo {
public:
    static constexpr int FIFO_SIZE = 1024;

    AudioToUIFifo() : abstractFifo_(FIFO_SIZE) {
        playheadEvents_.resize(FIFO_SIZE);
        clipStateEvents_.resize(FIFO_SIZE);
    }

    //==========================================================================
    // Audio Thread Methods (Wait-Free)
    //==========================================================================
    void pushPlayheadUpdate(int trackIndex, float position) {
        // Simple single-item push
        // In a real scenario, we might batch these or use a specific specialized queue
        // For now, we assume low contention or acceptable drop rate?
        // Actually, AbstractFifo isn't purely "drop oldest", it blocks if full?
        // No, we use write/read pointers.
        
        // This is a simplified "fire and forget" if space available
        int start1, size1, start2, size2;
        abstractFifo_.prepareToWrite(1, start1, size1, start2, size2);
        
        if (size1 > 0) {
            playheadEvents_[static_cast<size_t>(start1)] = {trackIndex, position};
            abstractFifo_.finishedWrite(1);
        }
    }

    //==========================================================================
    // UI Thread Methods
    //==========================================================================
    // TODO: This needs to distinguish between event types.
    // For simplicity in this iteration, let's just use it for Playhead updates
    // and rely on existing mechanisms for state? 
    // Or we can use a Variant or Union.
    
    bool popPlayheadUpdate(PlayheadEvent& event) {
        int start1, size1, start2, size2;
        abstractFifo_.prepareToRead(1, start1, size1, start2, size2);
        
        if (size1 > 0) {
            event = playheadEvents_[static_cast<size_t>(start1)];
            abstractFifo_.finishedRead(1);
            return true;
        }
        return false;
    }

private:
    juce::AbstractFifo abstractFifo_;
    std::vector<PlayheadEvent> playheadEvents_;
    std::vector<ClipStateEvent> clipStateEvents_; // Unused in this simple version
};

} // namespace zenith
