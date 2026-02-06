/*
  ==============================================================================

    ZenithUltraSynthVoice.h
    Created: [Date] Author: Claude AI
    Based on: ZenithPolySynthVoice architecture

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ZenithUltraSynth.h"

namespace Zenith
{

class ZenithUltraSynthVoice : public juce::MPESynthesiserVoice
{
public:
    ZenithUltraSynthVoice(ZenithUltraSynthProcessor* processor);
    ~ZenithUltraSynthVoice();

    // Voice state management
    void clearCurrentNote() override;
    bool canPlaySound(const juce::MPESound* sound) override;

    // Note on/off events
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePressureChanged() override;
    void notePitchbendChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;

    // Main audio processing
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

    // Synthesis mode control
    void setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode mode);
    ZenithUltraSynthProcessor::SynthesisMode getSynthesisMode() const;

    // Voice parameter control
    void setVoiceIndex(int index);
    int getVoiceIndex() const;
    void setUnisonDetune(float detuneAmount);
    float getUnisonDetune() const;

    // MIDI event handling
    void handleMidiEvent(const juce::MidiMessage& m) override;

    // Voice lifecycle
    bool isVoiceActive() const;
    float getVelocity() const;
    float getFrequency() const;
    int getMidiNote() const;

    // Voice-specific modulation
    void setModulationDepth(int modulatorIndex, float depth);
    float getModulationDepth(int modulatorIndex) const;

    // Performance monitoring
    float getVoiceCpuUsage() const;
    juce::uint64 getVoiceAge() const;

    // Voice state query
    bool isSustaining() const;
    bool isReleased() const;
    float getReleaseProgress() const;

private:
    // Synthesis mode state
    ZenithUltraSynthProcessor::SynthesisMode synthesisMode_;

    // Voice management
    int voiceIndex_;              // Voice index for unison/polyphony
    float unisonDetune_;          // Detune amount for unison voices
    bool isActive_;
    bool isSustaining_;
    bool isReleased_;

    // MIDI state
    int midiNote_;
    float velocity_;
    float pitchBend_;
    float notePressure_;
    float timbre_;
    juce::MPENote mpeNote_;

    // Voice age tracking
    juce::uint64 voiceStartTime_;
    juce::uint64 voiceStopTime_;

    // Per-engine voice objects
    std::unique_ptr<PhysicalModelingVoice> physicalModelingVoice_;
    std::unique_ptr<NeuralSynthVoice> neuralSynthVoice_;
    std::unique_ptr<AdvancedWavetableVoice> wavetableVoice_;
    std::unique_ptr<SubtractiveVoice> subtractiveVoice_;

    // Audio processing
    juce::AudioBuffer<float> internalBuffer_;
    int internalBufferSize_;

    // Voice-specific modulation
    std::array<float, 4> modulationDepths_;

    // Private helper methods
    void initializeVoiceEngines();
    void updateMPEParameters();
    void processVoiceModulation(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyVoiceEffects(juce::AudioBuffer<float>& buffer, int numSamples);
    void updateVoiceAge();

    // Engine-specific rendering
    void renderPhysicalModeling(juce::AudioBuffer<float>& buffer, int numSamples);
    void renderNeuralSynthesis(juce::AudioBuffer<float>& buffer, int numSamples);
    void renderWavetable(juce::AudioBuffer<float>& buffer, int numSamples);
    void renderSubtractive(juce::AudioBuffer<float>& buffer, int numSamples);

    // Voice lifecycle management
    void startVoice();
    void stopVoice(bool allowTailOff);
    void releaseVoice();

    // Parameter interpolation
    float interpolateParameter(float target, float current, float speed);
    void updateParametersSmoothly();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithUltraSynthVoice)
};

} // namespace Zenith