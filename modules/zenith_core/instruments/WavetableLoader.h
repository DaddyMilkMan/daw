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

#include "WavetableData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

//==============================================================================
// WAVETABLE FILE LOADER
//==============================================================================
/**
 * Professional wavetable file loader supporting multiple formats
 *
 * FEATURES:
 * - Support for 16-bit, 24-bit, 32-bit float .wav files
 * - Auto-detection of wavetable format (Serum-style single vs multi-cycle)
 * - Cross-fade loop handling for seamless frame transitions
 * - RMS normalization for consistent levels
 * - Metadata extraction (name, author, etc.)
 *
 * FILE FORMATS SUPPORTED:
 * - Serum .wav (single-cycle wavetable, 2048 samples, 16-bit)
 * - Vital .wav (multi-cycle wavetable, 2048 samples, 32-bit)
 * - Standard .wav (any wavetable file)
 */
class WavetableLoader {
public:
    WavetableLoader() = default;

    /**
     * @brief Load a wavetable file from disk
     * @param file Path to .wav file
     * @return Result containing WavetableData on success, or error message
     */
    juce::Result loadWavetable(const juce::File& file, WavetableData& outData);

    /**
     * @brief Get list of available wavetable files in a directory
     * @param directoryPath Directory to scan
     * @return Array of wavetable file paths
     */
    static juce::StringArray getAvailableWavetables(const juce::File& directoryPath);

private:
    // Constants
    static constexpr int SERUM_HEADER_SIZE = 100;
    static constexpr int MAX_WAVETABLE_SIZE = 2048 * 256; // Max samples to load
    static constexpr int MAX_WAVETABLE_FRAMES = 256;

    /**
     * @brief Detect wavetable format from file header
     * @param file File to check
     * @return Format type
     */
    WavetableFormat detectFormat(const juce::File& file);

    /**
     * @brief Detect format from memory buffer
     */
    WavetableFormat detectFormatFromBuffer(const void* data, size_t size);

    /**
     * @brief Parse 16-bit PCM sample from data
     */
    static float parseSample16(const uint8_t* data);

    /**
     * @brief Parse 24-bit PCM sample from data
     */
    static float parseSample24(const uint8_t* data);

    /**
     * @brief Parse 32-bit float sample from data
     */
    static float parseSample32(const uint8_t* data);

    /**
     * @brief Calculate RMS level for normalization
     */
    static float calculateRMS(const std::vector<float>& samples);

    // Temporary buffer for file loading
    std::vector<float> tempBuffer_;
};

} // namespace zenith
