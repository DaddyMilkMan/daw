#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Clip.h"
#include "TempoMap.h"

namespace zenith {

/**
 * @class AudioAsMidiBridge
 * @brief Transcribes audio transients/pitches into MIDI note specifications.
 * 
 * This fulfills Pillar C of the strategic feature set, providing 
 * "Google Docs-style" intelligence for audio editing.
 */
class AudioAsMidiBridge {
public:
    AudioAsMidiBridge(double sampleRate);
    ~AudioAsMidiBridge();

    /**
     * @brief Detects transients in an audio buffer and returns MIDI note specs.
     * @param audio The input audio buffer.
     * @param tempoMap The project tempo map for beat conversion.
     * @param threshold Sensitivity threshold (0.0 - 1.0).
     * @return Array of MidiNoteSpec for the detected hits.
     */
    juce::Array<MidiNoteSpec> transcribeTransients(const juce::AudioBuffer<float>& audio, const TempoMap& tempoMap, float threshold = 0.5f);

    /**
     * @brief Performs pitch detection and returns a melodic MIDI sequence.
     * @param audio The input audio buffer.
     * @return Array of MidiNoteSpec representing the melody.
     */
    juce::Array<MidiNoteSpec> transcribeMelody(const juce::AudioBuffer<float>& audio);

private:
    double sampleRate_;
    std::unique_ptr<juce::dsp::FFT> fft_;
    juce::dsp::WindowingFunction<float> window_;

    // Robust onset detection state
    float lastEnergy_ = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAsMidiBridge)
};

} // namespace zenith
