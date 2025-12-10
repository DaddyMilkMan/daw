/*
  ==============================================================================

    AudioFifo.h
    Created: 2025-12-08
    Author:  Zenith DAW

    Lock-free single-producer single-consumer FIFO for audio samples.
    Safe for transferring data from Audio Thread to UI Thread.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

class AudioFifo {
public:
    explicit AudioFifo(int size = 4096) 
        : abstractFifo_(size) 
    {
        buffer_.setSize(1, size); // Mono buffer
    }

    // Push stereo samples (mixes to mono)
    void pushStereoAsMono(const juce::AudioBuffer<float>& source, int numSamples) {
        // Range check
        if (numSamples <= 0) return;
        
        // Prepare temporary storage for pointers
        int start1, size1, start2, size2;
        abstractFifo_.prepareToWrite(numSamples, start1, size1, start2, size2);
        
        if (size1 > 0) {
            mixToMono(source, start1, size1, 0); // 0 offset in source for now, assuming block processing
        }
        if (size2 > 0) {
            mixToMono(source, start2, size2, size1);
        }
        
        abstractFifo_.finishedWrite(size1 + size2);
    }
    
    // Pop samples into destination buffer
    void pop(std::vector<float>& destination) {
        int numWanted = (int)destination.size();
        int start1, size1, start2, size2;
        abstractFifo_.prepareToRead(numWanted, start1, size1, start2, size2);
        
        if (size1 > 0) {
            // Copy from ring buffer to destination
            const float* src = buffer_.getReadPointer(0, start1);
            memcpy(destination.data(), src, size1 * sizeof(float));
        }
        if (size2 > 0) {
            const float* src = buffer_.getReadPointer(0, start2);
            memcpy(destination.data() + size1, src, size2 * sizeof(float));
        }
        
        abstractFifo_.finishedRead(size1 + size2);
    }
    
    int getNumReady() const { return abstractFifo_.getNumReady(); }

private:
    void mixToMono(const juce::AudioBuffer<float>& source, int destStart, int numSamples, int sourceOffset) {
        float* dest = buffer_.getWritePointer(0, destStart);
        
        if (source.getNumChannels() == 1) {
            // Mono copy
            memcpy(dest, source.getReadPointer(0, sourceOffset), numSamples * sizeof(float));
        } else {
            // Stereo mix
            const float* l = source.getReadPointer(0, sourceOffset);
            const float* r = source.getReadPointer(1, sourceOffset);
            
            for (int i = 0; i < numSamples; ++i) {
                dest[i] = (l[i] + r[i]) * 0.5f;
            }
        }
    }

    juce::AbstractFifo abstractFifo_;
    juce::AudioBuffer<float> buffer_;
};

} // namespace zenith
