/*
  ==============================================================================

    StereoAudioFifo.h
    Created: 2025-12-13
    Author:  Zenith DAW

    Lock-free single-producer single-consumer FIFO for STEREO audio samples.
    Safe for transferring data from Audio Thread to UI Thread.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>


namespace zenith {

class StereoAudioFifo {
public:
  explicit StereoAudioFifo(int size = 16384) : abstractFifo_(size) {
    buffer_.setSize(2, size); // Stereo buffer
  }

  void setSize(int newSize) {
    abstractFifo_.setTotalSize(newSize);
    buffer_.setSize(2, newSize);
  }

  // Push stereo samples
  void push(const juce::AudioBuffer<float> &source, int numSamples) {
    // Range check
    if (numSamples <= 0)
      return;

    // Prepare temporary storage for pointers
    int start1, size1, start2, size2;
    abstractFifo_.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0) {
      copyToBuffer(source, start1, size1, 0); // 0 offset in source for now
    }
    if (size2 > 0) {
      copyToBuffer(source, start2, size2, size1);
    }

    abstractFifo_.finishedWrite(size1 + size2);
  }

  // Pop samples into destination buffer (interleaved or separate? let's do
  // AudioBuffer)
  void pop(juce::AudioBuffer<float> &destination) {
    int numWanted = destination.getNumSamples();
    int start1, size1, start2, size2;
    abstractFifo_.prepareToRead(numWanted, start1, size1, start2, size2);

    if (size1 > 0) {
      destination.copyFrom(0, 0, buffer_, 0, start1, size1);
      destination.copyFrom(1, 0, buffer_, 1, start1, size1);
    }
    if (size2 > 0) {
      destination.copyFrom(0, size1, buffer_, 0, start2, size2);
      destination.copyFrom(1, size1, buffer_, 1, start2, size2);
    }

    abstractFifo_.finishedRead(size1 + size2);
  }

  int getNumReady() const { return abstractFifo_.getNumReady(); }
  int getFreeSpace() const { return abstractFifo_.getFreeSpace(); }

private:
  void copyToBuffer(const juce::AudioBuffer<float> &source, int destStart,
                    int numSamples, int sourceOffset) {
    // Copy Left
    if (source.getNumChannels() > 0) {
      buffer_.copyFrom(0, destStart, source, 0, sourceOffset, numSamples);
    } else {
      buffer_.clear(0, destStart, numSamples);
    }

    // Copy Right
    if (source.getNumChannels() > 1) {
      buffer_.copyFrom(1, destStart, source, 1, sourceOffset, numSamples);
    } else if (source.getNumChannels() > 0) {
      // Duplicate mono to right if source is mono
      buffer_.copyFrom(1, destStart, source, 0, sourceOffset, numSamples);
    } else {
      buffer_.clear(1, destStart, numSamples);
    }
  }

  juce::AbstractFifo abstractFifo_;
  juce::AudioBuffer<float> buffer_;
};

} // namespace zenith
