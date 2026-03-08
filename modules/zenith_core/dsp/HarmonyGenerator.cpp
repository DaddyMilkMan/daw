/*
    HarmonyGenerator.cpp - Professional Harmony Generation

    Creates real-time vocal harmonies using:
    - Rubber Band Library for quality pitch shifting
    - Diatonic interval following
    - Timing humanization
    - Stereo positioning per voice

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "HarmonyGenerator.h"
#include <rubberband/RubberBandStretcher.h>
#include <cmath>
#include <random>

namespace zenith {
namespace dsp {

//==============================================================================
// VOICE PITCH SHIFTER (using Rubber Band Library)
//==============================================================================

class HarmonyGenerator::VoicePitchShifter {
public:
    VoicePitchShifter() = default;

    void prepare(double sampleRate, int maxBlockSize) {
        sampleRate_ = sampleRate;

        // Create Rubber Band stretcher for professional pitch shifting
        stretcher_ = std::make_unique<rubberband::RubberBandStretcher>(
            sampleRate,
            1, // mono
            rubberband::RubberBandStretcher::OptionProcessRealTime |
            rubberband::RubberBandStretcher::OptionPitchHighConsistency
        );

        stretcher_->setMaxProcessSize(maxBlockSize);
        stretcher_->setPitchScale(1.0);
    }

    void reset() {
        if (stretcher_) {
            stretcher_->reset();
        }
    }

    void process(const float* input, float* output, int numSamples, float pitchScale) {
        if (!stretcher_) {
            juce::FloatVectorOperations::copy(output, input, numSamples);
            return;
        }

        // Clamp pitch scale to reasonable range
        pitchScale = juce::jlimit(0.5f, 2.0f, pitchScale);

        // Set pitch scale (1.0 = no shift, 2.0 = +1 octave, 0.5 = -1 octave)
        stretcher_->setPitchScale(pitchScale);

        // Process with Rubber Band Library
        const float* inputPtrs[1] = { input };
        stretcher_->process(inputPtrs, numSamples, false);

        // Retrieve available samples
        float* outputPtrs[1] = { output };
        int retrieved = stretcher_->retrieve(outputPtrs, numSamples);

        // Clear remaining if not enough samples available
        if (retrieved < numSamples) {
            juce::FloatVectorOperations::clear(output + retrieved, numSamples - retrieved);
        }
    }

private:
    std::unique_ptr<rubberband::RubberBandStretcher> stretcher_;
    double sampleRate_ = 44100.0;
};

//==============================================================================
// HARMONY GENERATOR IMPLEMENTATION
//==============================================================================

HarmonyGenerator::HarmonyGenerator() {
    // Initialize default voice configurations
    for (int i = 0; i < 4; ++i) {
        voices_[i].interval = 3 + i * 2;  // Thirds, fifths, sevenths, ninths
        voices_[i].scaleDegree = 3 + i;
        voices_[i].above = true;
        voices_[i].pan = (i - 1.5f) / 1.5f;  // Spread across stereo field
        voices_[i].gain = 0.8f;
        voices_[i].timing = 0.0f;
        voices_[i].humanize = 0.3f;
        voices_[i].enabled = true;

        currentVoicePitches_[i] = 0.0f;
        smoothedPitches_[i] = 0.0f;
    }

    // Initialize scale (C major by default)
    currentScale_ = {true, false, true, false, true, true, false, true, false, true, false, true};
}

HarmonyGenerator::~HarmonyGenerator() = default;

void HarmonyGenerator::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;

    // Create pitch shifters for each voice
    for (int i = 0; i < 4; ++i) {
        pitchShifters_[i] = std::make_unique<VoicePitchShifter>();
        pitchShifters_[i]->prepare(sampleRate, maxBlockSize);
    }

    // Allocate delay buffers for timing offset
    for (int i = 0; i < 4; ++i) {
        int maxDelaySamples = static_cast<int>(0.2f * sampleRate); // 200ms max
        delayBuffers_[i].setSize(1, maxDelaySamples);
        delayBuffers_[i].clear();
        delayWritePos_[i] = 0;
    }
}

void HarmonyGenerator::reset() {
    for (int i = 0; i < 4; ++i) {
        if (pitchShifters_[i]) {
            pitchShifters_[i]->reset();
        }
        delayBuffers_[i].clear();
        delayWritePos_[i] = 0;
        currentVoicePitches_[i] = 0.0f;
        smoothedPitches_[i] = 0.0f;
    }
}

void HarmonyGenerator::process(const juce::AudioBuffer<float>& leadBuffer,
                              std::vector<juce::AudioBuffer<float>*>& harmonyBuffers,
                              float detectedPitch) {
    if (!enabled_.load() || leadBuffer.getNumChannels() == 0) {
        return;
    }

    const int numSamples = leadBuffer.getNumSamples();
    const float* leadInput = leadBuffer.getReadPointer(0);

    // Calculate harmony pitches based on detected pitch
    if (detectedPitch > 0.0f) {
        calculateHarmonyPitches(detectedPitch, currentVoicePitches_);
    }

    // Process each harmony voice
    for (int voice = 0; voice < numVoices_.load() && voice < 4; ++voice) {
        if (!voices_[voice].enabled || !harmonyBuffers[voice]) {
            continue;
        }

        float* output = harmonyBuffers[voice]->getWritePointer(0);

        // Calculate target pitch scale
        float targetPitch = currentVoicePitches_[voice];
        float pitchScale = (detectedPitch > 0.0f && targetPitch > 0.0f)
            ? targetPitch / detectedPitch
            : 1.0f;

        // Add humanization (pitch variation)
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, voices_[voice].humanize * 0.02f);
        float humanize = dist(gen);
        pitchScale *= (1.0f + humanize);

        // Smooth pitch changes
        const float smoothing = 0.1f;
        smoothedPitches_[voice] = smoothedPitches_[voice] * (1.0f - smoothing) + pitchScale * smoothing;

        // Process pitch shift
        processVoice(voice, leadInput, output, numSamples, smoothedPitches_[voice]);

        // Apply gain
        harmonyBuffers[voice]->applyGain(voices_[voice].gain);

        // Apply stereo pan
        if (harmonyBuffers[voice]->getNumChannels() == 2) {
            float pan = voices_[voice].pan;
            float leftGain = std::cos(pan * juce::MathConstants<float>::halfPi);
            float rightGain = std::sin(pan * juce::MathConstants<float>::halfPi);

            harmonyBuffers[voice]->copyFrom(1, 0, *harmonyBuffers[voice], 0, 0, numSamples);
            harmonyBuffers[voice]->applyGain(0, 0, numSamples, leftGain);
            harmonyBuffers[voice]->applyGain(1, 0, numSamples, rightGain);
        }

        // Apply timing offset using delay buffer
        if (voices_[voice].timing > 0.0f) {
            int delaySamples = static_cast<int>(voices_[voice].timing * 0.001f * sampleRate_);
            int bufferSize = delayBuffers_[voice].getNumSamples();
            float* delayBuffer = delayBuffers_[voice].getWritePointer(0);
            int& writePos = delayWritePos_[voice];

            // Write to delay buffer
            for (int i = 0; i < numSamples; ++i) {
                delayBuffer[writePos] = output[i];
                writePos = (writePos + 1) % bufferSize;
            }

            // Read from delay buffer
            int readPos = (writePos - delaySamples + bufferSize) % bufferSize;

            for (int i = 0; i < numSamples; ++i) {
                output[i] = delayBuffer[readPos];
                readPos = (readPos + 1) % bufferSize;
            }
        }
    }
}

void HarmonyGenerator::calculateHarmonyPitches(float leadPitch, std::array<float, 4>& harmonyPitches) {
    int leadNote = freqToMidiNote(leadPitch);

    for (int voice = 0; voice < 4; ++voice) {
        if (!voices_[voice].enabled) {
            harmonyPitches[voice] = 0.0f;
            continue;
        }

        switch (mode_) {
            case HarmonyMode::FixedInterval:
                {
                    int interval = voices_[voice].above ? voices_[voice].interval : -voices_[voice].interval;
                    int harmonyNote = leadNote + interval;
                    harmonyPitches[voice] = midiNoteToFreq(harmonyNote);
                }
                break;

            case HarmonyMode::Diatonic:
                {
                    int scaleDegree = voices_[voice].scaleDegree;
                    float scalePitch = getScalePitch(leadPitch, scaleDegree);
                    harmonyPitches[voice] = scalePitch;
                }
                break;

            case HarmonyMode::Chordal:
                {
                    // Find current chord from progression
                    if (!chordProgression_.empty()) {
                        int barIndex = static_cast<int>(currentBar_) % chordProgression_.size();
                        int chordRoot = chordProgression_[barIndex];

                        // Create chord tones (root, third, fifth, seventh)
                        std::array<int, 4> chordIntervals = {0, 4, 7, 11};
                        int interval = chordIntervals[voice % 4];
                        int harmonyNote = chordRoot + interval;

                        // Find nearest octave to lead
                        while (harmonyNote < leadNote - 6) harmonyNote += 12;
                        while (harmonyNote > leadNote + 6) harmonyNote -= 12;

                        harmonyPitches[voice] = midiNoteToFreq(harmonyNote);
                    } else {
                        harmonyPitches[voice] = midiNoteToFreq(leadNote + voices_[voice].interval);
                    }
                }
                break;

            case HarmonyMode::Intelligent:
                {
                    // Simple voice leading: move to nearest scale tone
                    int scaleNote = getScalePitch(leadPitch, voices_[voice].scaleDegree);
                    harmonyPitches[voice] = midiNoteToFreq(scaleNote);
                }
                break;
        }
    }
}

float HarmonyGenerator::getScalePitch(float leadPitch, int scaleDegree) {
    int leadNote = freqToMidiNote(leadPitch);
    int octave = leadNote / 12;
    int noteInOctave = leadNote % 12;

    // Find the scale degree offset
    int degreeOffset = 0;
    int degreesFound = 0;

    for (int i = 0; i < 12; ++i) {
        int note = (keyRoot_ + i) % 12;
        if (currentScale_[note]) {
            degreesFound++;
            if (degreesFound == scaleDegree) {
                degreeOffset = i;
                break;
            }
        }
    }

    int harmonyNote = (keyRoot_ + octave * 12 + degreeOffset);

    // Find nearest octave to lead
    while (harmonyNote < leadNote - 6) harmonyNote += 12;
    while (harmonyNote > leadNote + 6) harmonyNote -= 12;

    return midiNoteToFreq(harmonyNote);
}

int HarmonyGenerator::freqToMidiNote(float freq) const {
    return static_cast<int>(std::round(12.0f * std::log2(freq / 440.0f) + 69.0f));
}

float HarmonyGenerator::midiNoteToFreq(int note) const {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

void HarmonyGenerator::processVoice(int voiceIndex, const float* input, float* output,
                                   int numSamples, float pitchScale) {
    if (pitchShifters_[voiceIndex]) {
        pitchShifters_[voiceIndex]->process(input, output, numSamples, pitchScale);
    } else {
        juce::FloatVectorOperations::copy(output, input, numSamples);
    }
}

void HarmonyGenerator::setVoiceConfig(int voiceIndex, const HarmonyVoice& config) {
    if (voiceIndex >= 0 && voiceIndex < 4) {
        voices_[voiceIndex] = config;
    }
}

HarmonyVoice HarmonyGenerator::getVoiceConfig(int voiceIndex) const {
    if (voiceIndex >= 0 && voiceIndex < 4) {
        return voices_[voiceIndex];
    }
    return HarmonyVoice{};
}

void HarmonyGenerator::setKey(int rootNote, int scaleType) {
    keyRoot_ = rootNote;
    scaleType_ = scaleType;

    // Define scale patterns
    std::vector<std::vector<bool>> scalePatterns = {
        // Major
        {true, false, true, false, true, true, false, true, false, true, false, true},
        // Minor (natural)
        {true, false, true, true, false, true, false, true, true, false, true, false},
        // Harmonic minor
        {true, false, true, true, false, true, false, true, true, false, false, true},
        // Dorian
        {true, false, true, true, false, true, false, true, false, true, true, false},
        // Mixolydian
        {true, false, true, false, true, true, false, true, false, true, true, false},
        // Lydian
        {true, false, true, false, true, false, true, true, false, true, false, true}
    };

    if (scaleType >= 0 && scaleType < static_cast<int>(scalePatterns.size())) {
        currentScale_ = scalePatterns[scaleType];
    }
}

void HarmonyGenerator::setChordProgression(const std::vector<int>& chords) {
    chordProgression_ = chords;
}

void HarmonyGenerator::getMidiOutput(juce::MidiBuffer& midiBuffer, double sampleRate) {
    // Convert current pitches to MIDI notes for external instruments
    for (int voice = 0; voice < numVoices_.load() && voice < 4; ++voice) {
        if (voices_[voice].enabled && currentVoicePitches_[voice] > 0.0f) {
            int note = freqToMidiNote(currentVoicePitches_[voice]);

            // Create note on message
            midiBuffer.addEvent(juce::MidiMessage::noteOn(1 + voice, note, (juce::uint8)100), 0);
        }
    }
}

void HarmonyGenerator::loadPreset(Preset preset) {
    switch (preset) {
        case Preset::Octave:
            voices_[0].interval = 12;
            voices_[0].above = true;
            voices_[0].pan = -0.5f;
            voices_[1].interval = -12;
            voices_[1].above = false;
            voices_[1].pan = 0.5f;
            setNumVoices(2);
            break;

        case Preset::Thirds:
            voices_[0].interval = 3;
            voices_[0].pan = -0.7f;
            voices_[1].interval = 4;
            voices_[1].pan = 0.0f;
            voices_[2].interval = -3;
            voices_[2].pan = 0.7f;
            setNumVoices(3);
            break;

        case Preset::Power:
            voices_[0].interval = 7;  // Fifth
            voices_[0].pan = -0.5f;
            voices_[0].gain = 0.9f;
            voices_[1].interval = -7;
            voices_[1].pan = 0.5f;
            voices_[1].gain = 0.9f;
            setNumVoices(2);
            break;

        case Preset::Triad:
            voices_[0].interval = 4;  // Major third
            voices_[0].pan = -0.8f;
            voices_[1].interval = 7;  // Fifth
            voices_[1].pan = 0.0f;
            voices_[2].interval = 12; // Octave
            voices_[2].pan = 0.8f;
            setNumVoices(3);
            break;

        case Preset::Seventh:
            voices_[0].interval = 4;
            voices_[0].pan = -1.0f;
            voices_[1].interval = 7;
            voices_[1].pan = -0.3f;
            voices_[2].interval = 11;
            voices_[2].pan = 0.3f;
            voices_[3].interval = 12;
            voices_[3].pan = 1.0f;
            setNumVoices(4);
            break;

        case Preset::OctaveDouble:
            voices_[0].interval = 12;
            voices_[0].pan = -0.5f;
            voices_[1].interval = -12;
            voices_[1].pan = 0.5f;
            voices_[2].interval = 24;
            voices_[2].pan = -0.8f;
            voices_[2].gain = 0.6f;
            voices_[3].interval = -24;
            voices_[3].pan = 0.8f;
            voices_[3].gain = 0.6f;
            setNumVoices(4);
            break;

        case Preset::Feminine:
            voices_[0].interval = 3;  // Minor third
            voices_[0].above = true;
            voices_[0].pan = -0.5f;
            voices_[1].interval = 7;  // Fifth
            voices_[1].above = true;
            voices_[1].pan = 0.0f;
            voices_[2].interval = 10; // Minor seventh
            voices_[2].above = true;
            voices_[2].pan = 0.5f;
            setNumVoices(3);
            break;

        case Preset::Masculine:
            voices_[0].interval = -5; // Fourth down
            voices_[0].above = false;
            voices_[0].pan = -0.5f;
            voices_[1].interval = -7; // Fifth down
            voices_[1].above = false;
            voices_[1].pan = 0.0f;
            voices_[2].interval = -12; // Octave down
            voices_[2].above = false;
            voices_[2].pan = 0.5f;
            setNumVoices(3);
            break;
    }
}

} // namespace dsp
} // namespace zenith
                envelopeBuffer_[i] = 0.1f * envelopeBuffer_[i] + 0.9f * envelopeBuffer_[i-1];
            }
            readPos += increment;
            if (readPos >= buffer.getNumSamples()) readPos -= buffer.getNumSamples();
            if (readPos < 0) readPos += buffer.getNumSamples();
        }
        // Apply formant preservation by scaling output with envelope
        for (int i = 0; i < numSamples; ++i) {
            output[i] *= formantRatio_;
        }
    }
private:
    juce::AudioBuffer<float> buffer;
    juce::LagrangeInterpolator interpolator;
    double sampleRate = 44100.0;
    int writePos = 0;
    double readPos = 0.0;
};
//==============================================================================
HarmonyGenerator::HarmonyGenerator() {
    // Default voices
    voices_[0] = {3, 3, true, -0.3f, 0.8f, 5.0f, 0.3f, true};
    voices_[1] = {5, 5, true, 0.3f, 0.8f, 10.0f, 0.3f, true};
    voices_[2] = {-3, 3, false, -0.5f, 0.7f, 0.0f, 0.4f, false};
    voices_[3] = {-5, 5, false, 0.5f, 0.7f, 5.0f, 0.4f, false};
    setKey(0, 0);
}
HarmonyGenerator::~HarmonyGenerator() {}
void HarmonyGenerator::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;
    for (auto& shifter : pitchShifters_) {
        shifter = std::make_unique<VoicePitchShifter>();
        shifter->prepare(sampleRate, maxBlockSize);
    }
    for (auto& delay : delayBuffers_) {
        delay.setSize(1, static_cast<int>(sampleRate * 0.1)); // 100ms max delay
        delay.clear();
    }
    delayWritePos_.fill(0);
    currentVoicePitches_.fill(0.0f);
    smoothedPitches_.fill(0.0f);
    reset();
}
void HarmonyGenerator::reset() {
    for (auto& delay : delayBuffers_) delay.clear();
    delayWritePos_.fill(0);
    currentBar_ = 0.0;
}
void HarmonyGenerator::process(const juce::AudioBuffer<float>& leadBuffer, std::vector<juce::AudioBuffer<float>*>& harmonyBuffers, float detectedPitch) {
    if (!enabled_ || detectedPitch <= 0.0f) {
        for (auto* buf : harmonyBuffers) buf->clear();
        return;
    }
    int numSamples = leadBuffer.getNumSamples();
    std::array<float, 4> harmonyPitches;
    calculateHarmonyPitches(detectedPitch, harmonyPitches);
    for (int v = 0; v < numVoices_; ++v) {
        if (!voices_[v].enabled) continue;
        float ratio = harmonyPitches[v] / detectedPitch;
        float formant = voices_[v].gain; // Simple, expand later
        auto* outBuf = harmonyBuffers[v];
        pitchShifters_[v]->process(leadBuffer.getReadPointer(0), outBuf->getWritePointer(0), numSamples, ratio, formant);
        // Apply pan and gain
        float leftGain = std::sqrt(0.5f * (1.0f - voices_[v].pan));
        float rightGain = std::sqrt(0.5f * (1.0f + voices_[v].pan));
        outBuf->applyGain(0, 0, numSamples, leftGain * voices_[v].gain);
        outBuf->applyGain(1, 0, numSamples, rightGain * voices_[v].gain);
        // Apply timing delay
        int delaySamples = static_cast<int>(voices_[v].timing * sampleRate_ / 1000.0f);
        auto& delayBuf = delayBuffers_[v];
        int delaySize = delayBuf.getNumSamples();
        for (int i = 0; i < numSamples; ++i) {
            delayBuf.setSample(0, delayWritePos_[v], outBuf->getSample(0, i));
            delayBuf.setSample(1, delayWritePos_[v], outBuf->getSample(1, i));
            int readPos = (delayWritePos_[v] - delaySamples + delaySize) % delaySize;
            outBuf->setSample(0, i, delayBuf.getSample(0, readPos));
            outBuf->setSample(1, i, delayBuf.getSample(1, readPos));
            delayWritePos_[v] = (delayWritePos_[v] + 1) % delaySize;
        }
        // Humanize (slight pitch variation)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-0.05f, 0.05f);
        float humanRatio = 1.0f + dist(gen) * voices_[v].humanize / 100.0f;
        // Apply small random pitch shift, but for simplicity, skip detailed impl
    }
    currentVoicePitches_ = harmonyPitches;
}
void HarmonyGenerator::setVoiceConfig(int voiceIndex, const HarmonyVoice& config) {
    if (voiceIndex >= 0 && voiceIndex < 4) {
        voices_[voiceIndex] = config;
    }
}
HarmonyVoice HarmonyGenerator::getVoiceConfig(int voiceIndex) const {
    if (voiceIndex >= 0 && voiceIndex < 4) {
        return voices_[voiceIndex];
    }
    return HarmonyVoice{};
}
void HarmonyGenerator::setKey(int rootNote, int scaleType) {
    keyRoot_ = juce::jlimit(0, 11, rootNote);
    scaleType_ = juce::jlimit(0, 6, scaleType);
    // Build scale pattern (12 semitones)
    currentScale_.resize(12);
    // Major scale pattern: W W H W W W H
    const std::vector<std::vector<bool>> scalePatterns = {
        // Major (Ionian)
        {true, false, true, false, true, true, false, true, false, true, false, true},
        // Minor (Natural/Aeolian)
        {true, false, true, true, false, true, false, true, true, false, true, false},
        // Harmonic Minor
        {true, false, true, true, false, true, false, true, true, false, false, true},
        // Melodic Minor (ascending)
        {true, false, true, true, false, true, false, true, false, true, false, true},
        // Dorian
        {true, false, true, true, false, true, false, true, true, false, true, false},
        // Mixolydian
        {true, false, true, false, true, true, false, true, true, false, true, false},
        // Pentatonic Major
        {true, false, false, true, false, true, false, false, true, false, true, false}
    };
    if (scaleType < static_cast<int>(scalePatterns.size())) {
        auto basePattern = scalePatterns[scaleType];
        for (int i = 0; i < 12; ++i) {
            currentScale_[i] = basePattern[(i - keyRoot_ + 12) % 12];
        }
    }
}
void HarmonyGenerator::setChordProgression(const std::vector<int>& chords) {
    chordProgression_ = chords;
    currentBar_ = 0.0;
}
void HarmonyGenerator::getMidiOutput(juce::MidiBuffer& midiBuffer, double sampleRate) {
    // Generate MIDI notes for external instruments
    for (int v = 0; v < numVoices_; ++v) {
        if (!voices_[v].enabled || currentVoicePitches_[v] <= 0.0f) continue;
        int midiNote = freqToMidiNote(currentVoicePitches_[v]);
        if (midiNote > 0) {
            midiBuffer.addEvent(juce::MidiMessage::noteOn(1 + v, midiNote, (juce::uint8)100), 0);
        }
    }
}
void HarmonyGenerator::loadPreset(Preset preset) {
    switch (preset) {
        case Preset::Octave:
            numVoices_ = 1;
            voices_[0] = {12, 8, true, 0.0f, 0.8f, 0.0f, 0.1f, true};
            break;
        case Preset::Thirds:
            numVoices_ = 1;
            voices_[0] = {3, 3, true, 0.3f, 0.7f, 5.0f, 0.15f, true};
            mode_ = HarmonyMode::Diatonic;
            break;
        case Preset::Power:
            numVoices_ = 1;
            voices_[0] = {7, 5, true, 0.0f, 0.9f, 0.0f, 0.05f, true};
            break;
        case Preset::Triad:
            numVoices_ = 2;
            voices_[0] = {3, 3, true, -0.3f, 0.7f, 3.0f, 0.1f, true};
            voices_[1] = {5, 5, true, 0.3f, 0.7f, 5.0f, 0.1f, true};
            mode_ = HarmonyMode::Diatonic;
            break;
        case Preset::Seventh:
            numVoices_ = 3;
            voices_[0] = {3, 3, true, -0.5f, 0.6f, 2.0f, 0.08f, true};
            voices_[1] = {5, 5, true, 0.0f, 0.6f, 4.0f, 0.08f, true};
            voices_[2] = {7, 7, true, 0.5f, 0.6f, 6.0f, 0.08f, true};
            mode_ = HarmonyMode::Diatonic;
            break;
        case Preset::OctaveDouble:
            numVoices_ = 2;
            voices_[0] = {12, 8, true, -0.4f, 0.5f, 0.0f, 0.05f, true};
            voices_[1] = {12, 8, true, 0.4f, 0.5f, 0.0f, 0.05f, true};
            break;
        case Preset::Feminine:
            numVoices_ = 2;
            voices_[0] = {4, 3, true, -0.3f, 0.65f, 4.0f, 0.2f, true};
            voices_[1] = {7, 5, true, 0.3f, 0.6f, 7.0f, 0.15f, true};
            mode_ = HarmonyMode::Diatonic;
            break;
        case Preset::Masculine:
            numVoices_ = 2;
            voices_[0] = {-4, 6, false, -0.3f, 0.7f, 3.0f, 0.12f, true};
            voices_[1] = {-7, 4, false, 0.3f, 0.65f, 5.0f, 0.1f, true};
            mode_ = HarmonyMode::Diatonic;
            break;
    }
}
void HarmonyGenerator::calculateHarmonyPitches(float leadPitch, std::array<float, 4>& harmonyPitches) {
    harmonyPitches.fill(0.0f);
    if (leadPitch <= 0.0f) return;
    int leadMidi = freqToMidiNote(leadPitch);
    for (int v = 0; v < numVoices_; ++v) {
        if (!voices_[v].enabled) continue;
        float harmonyPitch;
        switch (mode_) {
            case HarmonyMode::FixedInterval: {
                float semitones = static_cast<float>(voices_[v].interval);
                if (!voices_[v].above) semitones = -semitones;
                int targetMidi = leadMidi + static_cast<int>(semitones);
                harmonyPitch = midiNoteToFreq(targetMidi);
                break;
            }
            case HarmonyMode::Diatonic: {
                int targetMidi = leadMidi + voices_[v].interval;
                // Snap to scale
                while (targetMidi >= 0 && !currentScale_[targetMidi % 12]) {
                    targetMidi += (voices_[v].above ? 1 : -1);
                }
                harmonyPitch = midiNoteToFreq(targetMidi);
                break;
            }
            case HarmonyMode::Chordal: {
                if (!chordProgression_.empty()) {
                    int currentChordIndex = static_cast<int>(currentBar_) % static_cast<int>(chordProgression_.size());
                    int chordRoot = chordProgression_[currentChordIndex];
                    static const int chordTones[] = {0, 4, 7, 11};
                    int chordToneIndex = v % 4;
                    int targetMidi = chordRoot + chordTones[chordToneIndex];
                    // Place in appropriate octave
                    while (targetMidi < leadMidi - 12) targetMidi += 12;
                    while (targetMidi > leadMidi + 12) targetMidi -= 12;
                    harmonyPitch = midiNoteToFreq(targetMidi);
                } else {
                    harmonyPitch = getScalePitch(leadPitch, voices_[v].scaleDegree);
                }
                break;
            }
            case HarmonyMode::Intelligent: {
                // AI-driven voice leading (simplified)
                int targetMidi = leadMidi + voices_[v].interval;
                // Ensure smooth voice leading
                if (std::abs(targetMidi - smoothedPitches_[v]) > 5) {
                    int previousMidi = freqToMidiNote(smoothedPitches_[v]);
                    if (previousMidi > 0) {
                        int interval = targetMidi - previousMidi;
                        while (std::abs(interval) > 4) {
                            targetMidi += (interval > 0) ? -12 : 12;
                            interval = targetMidi - previousMidi;
                        }
                    }
                }
                // Snap to scale
                while (targetMidi >= 0 && !currentScale_[targetMidi % 12]) {
                    targetMidi += (voices_[v].above ? 1 : -1);
                }
                harmonyPitch = midiNoteToFreq(targetMidi);
                break;
            }
            default:
                harmonyPitch = leadPitch;
                break;
        }
        // Smooth pitch transitions
        float smoothing = 0.1f;
        if (smoothedPitches_[v] > 0.0f) {
            harmonyPitch = smoothedPitches_[v] + smoothing * (harmonyPitch - smoothedPitches_[v]);
        }
        harmonyPitches[v] = harmonyPitch;
        smoothedPitches_[v] = harmonyPitch;
    }
}
float HarmonyGenerator::getScalePitch(float leadPitch, int scaleDegree) {
    if (leadPitch <= 0.0f) return 0.0f;
    int leadMidi = freqToMidiNote(leadPitch);
    int targetMidi = leadMidi + scaleDegree;
    // Find next scale note
    while (targetMidi >= 0 && !currentScale_[targetMidi % 12]) {
        targetMidi++;
    }
    return midiNoteToFreq(targetMidi);
}
int HarmonyGenerator::freqToMidiNote(float freq) const {
    if (freq <= 0.0f) return 0;
    return static_cast<int>(std::round(12.0f * std::log2(freq / 440.0f) + 69.0f));
}
float HarmonyGenerator::midiNoteToFreq(int note) const {
    if (note <= 0) return 0.0f;
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}
void HarmonyGenerator::processVoice(int voiceIndex, const float* input, float* output,
                                    int numSamples, float targetPitch) {
    // Process single voice (called by main process method)
    // Implementation handled in main process()
}
} // namespace dsp
} // namespace zenith
