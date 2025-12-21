/*
  ==============================================================================

    InstrumentTrack.h
    Created: 2025-12-19
    Author:  Zenith DAW

    MIDI track with a virtual instrument plugin.
    Combines MIDI clip playback with instrument audio generation.

  ==============================================================================
*/

#pragma once

#include "ClipTrack.h"

namespace zenith {

// Forward declare Instrument
class Instrument;

//==============================================================================
/**
    A MIDI track with an attached virtual instrument.

    InstrumentTrack processes MIDI clips and routes them through an instrument
    plugin (either built-in or VST3) to generate audio output.

    Key features:
    - Owns and manages an Instrument instance
    - Processes MIDI clips and live MIDI input
    - Routes MIDI through the instrument to generate audio
    - Supports instrument presets and parameter control
*/
class InstrumentTrack : public ClipTrack {
public:
    //==========================================================================
    // Construction
    //==========================================================================

    explicit InstrumentTrack(const juce::String& name);
    ~InstrumentTrack() override;

    //==========================================================================
    // AudioSource Interface
    //==========================================================================

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                           int64_t playheadSamples,
                           const juce::MidiBuffer* incomingMidi = nullptr,
                           const std::vector<juce::AudioBuffer<float>*>& auxBuffers = {},
                           const TempoMap* tempoMap = nullptr) override;

    // State management
    juce::ValueTree getState() const override;
    void loadState(const juce::ValueTree& state) override;

    //==========================================================================
    // Instrument Management
    //==========================================================================

    /**
     * @brief Set the instrument for this track
     * @param instrument The instrument to use (ownership transferred)
     * @note Message thread only
     */
    void setInstrument(std::unique_ptr<Instrument> instrument);

    /**
     * @brief Get the current instrument
     * @return Pointer to instrument, or nullptr if none set
     */
    Instrument* getInstrument() const override { return instrument_.get(); }

    /**
     * @brief Check if this track has an instrument assigned
     */
    bool hasInstrument() const override { return instrument_ != nullptr; }

    /**
     * @brief Remove the current instrument
     * @note Message thread only
     */
    void removeInstrument();

    //==========================================================================
    // State Management
    //==========================================================================

    /**
     * @brief Get instrument state for serialization
     */
    juce::ValueTree getInstrumentState() const;

    /**
     * @brief Load instrument state from ValueTree
     */
    void loadInstrumentState(const juce::ValueTree& state);

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    std::unique_ptr<Instrument> instrument_;
    juce::MidiBuffer midiBuffer_;           // Scratch buffer for MIDI processing
    juce::AudioBuffer<float> instrumentBuffer_;  // Scratch buffer for instrument output

    //==========================================================================
    // Audio Processing Helpers
    //==========================================================================

    void collectMidiFromClips(juce::MidiBuffer& midiBuffer, 
                              int64_t playheadSamples, 
                              int numSamples);

    void processInstrument(juce::AudioBuffer<float>& buffer, 
                          juce::MidiBuffer& midiBuffer, 
                          int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentTrack)
};

} // namespace zenith
