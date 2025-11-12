/*
  ==============================================================================

    Clip.h
    Ported from: VexelDAW-Native/Source/Audio/Clip.h (2025-11-11)
    Author:  Vexel DAW → Zenith DAW

    Audio/MIDI clip with transport synchronization and playback control

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - Kept as Track::Clip (nested class)
    - No container changes needed (uses std::unique_ptr internally)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Track.h"

namespace zenith {

//==============================================================================
/**
    Represents an audio or MIDI clip on the timeline.

    Each clip has a position on the timeline, a length, and can contain either
    audio samples or MIDI events. Clips support fade in/out, looping, and
    time offset for trimming.

    Thread-safe design allows clips to be modified from the UI thread while
    playing back on the audio thread.
*/
class Track::Clip : public juce::AudioSource
{
public:
    //==============================================================================
    enum class Type
    {
        Audio,
        MIDI
    };

    //==============================================================================
    Clip();
    ~Clip() override;

    //==============================================================================
    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    // Clip properties
    Type getType() const { return clipType; }
    void setType(Type type) { clipType = type; }

    const juce::String& getName() const { return clipName; }
    void setName(const juce::String& name) { clipName = name; }

    //==============================================================================
    // Timeline position (in samples)
    void setStartPosition(int64_t position);
    int64_t getStartPosition() const { return startPosition.load(); }

    void setLength(int64_t lengthInSamples);
    int64_t getLength() const { return clipLength.load(); }

    int64_t getEndPosition() const { return startPosition.load() + clipLength.load(); }

    // Offset within the source material (for trimming)
    void setOffset(int64_t offsetInSamples);
    int64_t getOffset() const { return clipOffset.load(); }

    //==============================================================================
    // Transport control
    void setTransportPosition(int64_t position);
    int64_t getTransportPosition() const { return transportPosition.load(); }

    void setPlaying(bool shouldPlay);
    bool isPlaying() const { return playing.load(); }

    // Check if clip is active at current transport position
    bool isActive() const;

    //==============================================================================
    // Audio clip specific
    void setAudioFile(const juce::File& file);
    juce::File getAudioFile() const { return audioFile; }

    void setAudioBuffer(const juce::AudioBuffer<float>& buffer);
    const juce::AudioBuffer<float>* getAudioBuffer() const { return &audioBuffer; }

    //==============================================================================
    // MIDI clip specific
    void setMidiSequence(const juce::MidiMessageSequence& sequence);
    const juce::MidiMessageSequence* getMidiSequence() const { return &midiSequence; }

    //==============================================================================
    // Fades (in samples)
    void setFadeIn(int64_t fadeInSamples);
    int64_t getFadeIn() const { return fadeInLength.load(); }

    void setFadeOut(int64_t fadeOutSamples);
    int64_t getFadeOut() const { return fadeOutLength.load(); }

    //==============================================================================
    // Gain control
    void setGain(float newGain);
    float getGain() const { return gain.load(); }

    //==============================================================================
    // Looping
    void setLooping(bool shouldLoop);
    bool isLooping() const { return looping.load(); }

    //==============================================================================
    // Color for visual representation
    void setColor(juce::Colour color);
    juce::Colour getColor() const { return clipColor; }

    //==============================================================================
    // State management
    juce::ValueTree getState() const;
    void loadState(const juce::ValueTree& state);

private:
    //==============================================================================
    // Clip properties
    Type clipType = Type::Audio;
    juce::String clipName{"Clip"};
    juce::Colour clipColor{juce::Colours::blue};

    //==============================================================================
    // Timeline position (atomic for lock-free access)
    std::atomic<int64_t> startPosition{0};
    std::atomic<int64_t> clipLength{0};
    std::atomic<int64_t> clipOffset{0};
    std::atomic<int64_t> transportPosition{0};

    //==============================================================================
    // Playback state (atomic for lock-free access)
    std::atomic<bool> playing{false};
    std::atomic<int64_t> fadeInLength{0};
    std::atomic<int64_t> fadeOutLength{0};
    std::atomic<float> gain{1.0f};
    std::atomic<bool> looping{false};

    //==============================================================================
    // Audio data
    juce::File audioFile;
    juce::AudioBuffer<float> audioBuffer;
    std::unique_ptr<juce::AudioFormatReaderSource> audioSource;
    juce::CriticalSection audioLock;

    //==============================================================================
    // MIDI data
    juce::MidiMessageSequence midiSequence;
    juce::CriticalSection midiLock;

    //==============================================================================
    // Processing state
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    //==============================================================================
    // Helper methods
    void processAudioClip(const juce::AudioSourceChannelInfo& bufferToFill);
    void processMidiClip(const juce::AudioSourceChannelInfo& bufferToFill);
    float calculateFadeMultiplier(int64_t positionInClip) const;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Clip)
};

} // namespace zenith
