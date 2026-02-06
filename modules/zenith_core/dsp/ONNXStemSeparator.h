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

#pragma once

#include <cstddef>
#include <initializer_list>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @brief Handles AI-based stem separation using ONNX Runtime
 * 

 * This class provides an interface for running Demucs or similar models
 * to separate audio into stems (Vocals, Drums, Bass, Other).
 */
class ONNXStemSeparator {
public:
    ONNXStemSeparator();
    ~ONNXStemSeparator();

    /**
     * @brief Check if the ONNX runtime is available and supported
     */
    bool isAvailable() const;

    /**
     * @brief Initialize the engine with a specific model file
     */
    bool initialize(const juce::File& modelPath);
    
    struct SeparationResult {
        juce::AudioBuffer<float> vocals;
        juce::AudioBuffer<float> drums;
        juce::AudioBuffer<float> bass;
        juce::AudioBuffer<float> other;
        bool success = false;
        bool usedONNX = false; // True if ONNX inference was used, false if DSP fallback
        juce::String error;
    };

    /**
     * @brief Perform separation on an audio buffer
     * @param input Stereo input buffer
     * @param sampleRate Sample rate of the audio
     * @return SeparationResult containing the stems
     */
    SeparationResult separate(const juce::AudioBuffer<float>& input, double sampleRate);
    
    /**
     * @brief Get information about the loaded model
     * @return String with model details
     */
    juce::String getModelInfo() const;

    /**
     * @brief Find the default model file using standard system paths
     * @return File object pointing to the model if found, or an empty non-existent file if not
     */
    static juce::File findDefaultModel();

    /**
     * @brief Cleanup static resources (ONNX environment). 
     * Call on application shutdown to avoid false positive leaks.
     */
    static void shutdown();


private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
#ifdef ZENITH_USE_ONNX_RUNTIME
    /**
     * @brief Helper to copy ONNX tensor data to JUCE AudioBuffer
     */
    void copyTensorToBuffer(const float* tensorData, juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
#endif

};

} // namespace zenith
