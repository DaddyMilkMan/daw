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

==============================================================================
// PROFESSIONAL WAVETABLE LOADER IMPLEMENTATION
//==============================================================================
*/

#include "WavetableLoader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstring>
#include <cmath>

namespace zenith {

//==============================================================================
// PUBLIC METHODS
//==============================================================================

juce::StringArray WavetableLoader::getAvailableWavetables(const juce::File& directoryPath) {
    juce::StringArray wavetables;

    if (!directoryPath.exists() || !directoryPath.isDirectory()) {
        return wavetables;
    }

    // Get all .wav files in directory (non-recursive)
    juce::Array<juce::File> files;
    directoryPath.findChildFiles(files, juce::File::findFiles, false, "*.wav");

    for (const auto& file : files) {
        wavetables.add(file.getFullPathName());
    }

    return wavetables;
}

juce::Result WavetableLoader::loadWavetable(const juce::File& file, WavetableData& outData) {
    // Validate file exists
    if (!file.existsAsFile()) {
        return juce::Result::fail("File not found: " + file.getFullPathName());
    }

    // Detect format
    WavetableFormat format = detectFormat(file);
    if (format == WavetableFormat::Unknown) {
        return juce::Result::fail("Unable to detect wavetable format");
    }

    // Use JUCE's audio format manager for reliable WAV reading
    static juce::AudioFormatManager formatManager;
    static bool initialized = false;

    if (!initialized) {
        formatManager.registerBasicFormats();
        initialized = true;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file)
    );

    if (!reader) {
        return juce::Result::fail("Failed to create audio reader for: " + file.getFileName());
    }

    // Validate wavetable properties
    const int numChannels = reader->numChannels;
    const int bitsPerSample = reader->bitsPerSample;
    const int sampleRate = reader->sampleRate;

    // Wavetables should be mono or stereo, at reasonable sample rates
    if (numChannels > 2) {
        return juce::Result::fail("Unsupported channel count: " + juce::String(numChannels));
    }

    if (bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) {
        return juce::Result::fail("Unsupported bit depth: " + juce::String(bitsPerSample));
    }

    // Calculate frame size - standard is 2048 samples per frame
    const int totalSamples = static_cast<int>(reader->lengthInSamples);
    const int samplesPerFrame = 2048;  // Serum standard
    const int numFrames = totalSamples / samplesPerFrame;

    if (numFrames < 1) {
        return juce::Result::fail("File too short for wavetable (minimum 2048 samples)");
    }

    if (numFrames > MAX_WAVETABLE_FRAMES) {
        return juce::Result::fail("Too many frames: " + juce::String(numFrames) +
                                   " (max: " + juce::String(MAX_WAVETABLE_FRAMES) + ")");
    }

    // Read entire file into memory
    tempBuffer_.resize(static_cast<size_t>(totalSamples) * numChannels);

    // Use JUCE's buffer for reading
    juce::AudioBuffer<float> readBuffer(numChannels, totalSamples);

    if (!reader->read(&readBuffer, 0, totalSamples, 0, true, true)) {
        return juce::Result::fail("Failed to read audio data");
    }

    // Convert to mono if stereo (average channels)
    std::vector<float> monoSamples;
    monoSamples.reserve(totalSamples);

    for (int i = 0; i < totalSamples; ++i) {
        if (numChannels == 1) {
            monoSamples.push_back(readBuffer.getSample(0, i));
        } else {
            // Average stereo channels
            float left = readBuffer.getSample(0, i);
            float right = readBuffer.getSample(1, i);
            monoSamples.push_back((left + right) * 0.5f);
        }
    }

    // Calculate RMS for normalization
    float rmsLevel = calculateRMS(monoSamples);
    const float targetRMS = 0.707f;  // -3dBFS target
    float normalizer = (rmsLevel > 0.0f) ? (targetRMS / rmsLevel) : 1.0f;

    // Parse samples into frames
    for (int frame = 0; frame < numFrames; ++frame) {
        WavetableFrame wtFrame;

        for (int sample = 0; sample < samplesPerFrame; ++sample) {
            int srcIndex = frame * samplesPerFrame + sample;
            if (srcIndex < totalSamples) {
                wtFrame[sample] = monoSamples[srcIndex] * normalizer;
            }
        }

        outData.addFrame(wtFrame);
    }

    // Set metadata
    outData.setName(file.getFileNameWithoutExtension());
    outData.setNumFrames(numFrames);
    outData.setBitDepth(bitsPerSample);

    return juce::Result::success();
}

//==============================================================================
// PRIVATE METHODS
//==============================================================================

WavetableFormat WavetableLoader::detectFormat(const juce::File& file) {
    if (!file.existsAsFile()) {
        return WavetableFormat::Unknown;
    }

    // Read first 100 bytes for format detection
    juce::FileInputStream stream(file);
    if (!stream.openedOk() || stream.getTotalLength() < 12) {
        return WavetableFormat::Unknown;
    }

    std::vector<uint8_t> header(SERUM_HEADER_SIZE);
    int bytesRead = stream.read(header.data(), SERUM_HEADER_SIZE);

    if (bytesRead < 12) {
        return WavetableFormat::Unknown;
    }

    // Check from buffer
    return detectFormatFromBuffer(header.data(), bytesRead);
}

WavetableFormat WavetableLoader::detectFormatFromBuffer(const void* data, size_t size) {
    if (size < 12) {
        return WavetableFormat::Unknown;
    }

    const uint8_t* bytes = static_cast<const uint8_t*>(data);

    // Check for RIFF/WAVE header
    if (bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
        bytes[8] == 'W' && bytes[9] == 'A' && bytes[10] == 'V' && bytes[11] == 'E') {
        return WavetableFormat::RIFF;
    }

    // Check for raw data (no RIFF header but valid sample data)
    // Could be a RAW wavetable file
    bool hasValidData = false;
    for (size_t i = 0; i < juce::jmin(size_t(32), size); ++i) {
        if (bytes[i] != 0) {
            hasValidData = true;
            break;
        }
    }

    if (hasValidData) {
        return WavetableFormat::RAW;
    }

    return WavetableFormat::Unknown;
}

float WavetableLoader::parseSample16(const uint8_t* data) {
    // Little-endian 16-bit signed PCM
    int16_t sample = static_cast<int16_t>(data[0] | (data[1] << 8));
    return static_cast<float>(sample) / 32768.0f;
}

float WavetableLoader::parseSample24(const uint8_t* data) {
    // Little-endian 24-bit signed PCM
    int32_t sample = static_cast<int32_t>(
        data[0] | (data[1] << 8) | (data[2] << 16)
    );

    // Sign extend from 24-bit to 32-bit
    if (sample & 0x800000) {
        sample |= 0xFF000000;
    }

    return static_cast<float>(sample) / 8388608.0f;  // 2^23
}

float WavetableLoader::parseSample32(const uint8_t* data) {
    // 32-bit float
    union {
        float f;
        uint32_t u;
    } converter;

    converter.u = (static_cast<uint32_t>(data[0])) |
                   (static_cast<uint32_t>(data[1]) << 8) |
                   (static_cast<uint32_t>(data[2]) << 16) |
                   (static_cast<uint32_t>(data[3]) << 24);

    return converter.f;
}

float WavetableLoader::calculateRMS(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0f;
    }

    double sum = 0.0;
    for (float sample : samples) {
        sum += static_cast<double>(sample) * static_cast<double>(sample);
    }

    return static_cast<float>(std::sqrt(sum / static_cast<double>(samples.size())));
}

} // namespace zenith
