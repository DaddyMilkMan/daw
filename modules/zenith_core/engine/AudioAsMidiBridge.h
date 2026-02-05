/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Clip.h"

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
     * @param threshold Sensitivity threshold (0.0 - 1.0).
     * @return Array of MidiNoteSpec for the detected hits.
     */
    juce::Array<MidiNoteSpec> transcribeTransients(const juce::AudioBuffer<float>& audio, float threshold = 0.5f);

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
