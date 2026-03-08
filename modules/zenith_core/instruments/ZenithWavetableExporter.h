/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include "WavetableData.h"
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>

namespace zenith {

//==============================================================================
// WAVETABLE EXPORT FORMAT
//==============================================================================
enum class WavetableExportFormat {
    WAV,            ///< Standard WAV files (single cycle)
    SERUM,          ///< Serum format (.wav)
    VITAL,           ///< Vital format (.wav)
    WAVETABLE,       ///< Wavetable format (.wt)
    JSON,            ///< JSON format
    FL_STUDIO,        ///< FL Studio format
    BITWIGGLE        ///< Bitwig format
};

//==============================================================================
// WAVETABLE EXPORT OPTIONS
//==============================================================================
struct WavetableExportOptions {
    //==========================================================================
    // Format Options
    //==========================================================================

    WavetableExportFormat format = WavetableExportFormat::WAV;

    //==========================================================================
    // Quality Options
    //==========================================================================

    int bitDepth = 32;                  ///< Bit depth (16, 24, 32)
    int sampleRate = 44100;             ///< Export sample rate
    int numCycles = 256;                ///< Number of cycles to export

    //==========================================================================
    // Processing Options
    //==========================================================================

    bool normalize = true;                ///< Normalize to -1 to 1
    bool dither = true;                   ///< Add dither for 16-bit

    //==========================================================================
    // Metadata
    //==========================================================================

    juce::String name = "Untitled";
    juce::String author = "Unknown";
    juce::String category = "Bass";
    juce::String description = "";

    //==========================================================================
    // Crossfade Options
    //==========================================================================

    bool crossfadeCycles = true;          ///< Smooth between cycles
    float crossfadeLength = 0.01f;        ///< Crossfade duration
};

//==============================================================================
// WAVETABLE EXPORTER
//==============================================================================
/**
 * Professional wavetable exporter matching Serum 2
 *
 * FEATURES:
 * - Multiple format support (WAV, Serum, Vital, etc.)
 * - Per-cycle quality control
 * - Metadata embedding
 * - Crossfade between cycles
 * - Batch export
 */
class ZenithWavetableExporter {
public:
    ZenithWavetableExporter();
    ~ZenithWavetableExporter() = default;

    //==========================================================================
    // Export Functions
    //==========================================================================

    /**
     * @brief Export wavetable data to file
     */
    bool exportToFile(const WavetableData& wavetable,
                     const juce::File& file,
                     const WavetableExportOptions& options);

    /**
     * @brief Export to memory block
     */
    juce::MemoryBlock exportToMemory(const WavetableData& wavetable,
                                       const WavetableExportOptions& options);

    /**
     * @brief Export multiple wavetables to directory
     */
    bool exportBatch(const std::vector<WavetableData>& wavetables,
                    const juce::File& targetDirectory,
                    const WavetableExportOptions& options);

    //==========================================================================
    // Format-Specific Export
    //==========================================================================

    /**
     * @brief Export as single-cycle WAV
     */
    bool exportWAV(const WavetableData& wavetable,
                  const juce::File& file,
                  const WavetableExportOptions& options);

    /**
     * @brief Export as Serum wavetable
     */
    bool exportSerum(const WavetableData& wavetable,
                    const juce::File& file,
                    const WavetableExportOptions& options);

    /**
     * @brief Export as Vital wavetable
     */
    bool exportVital(const WavetableData& wavetable,
                    const juce::File& file,
                    const WavetableExportOptions& options);

    /**
     * @brief Export as .wt wavetable
     */
    bool exportWavetable(const WavetableData& wavetable,
                       const juce::File& file,
                       const WavetableExportOptions& options);

    //==========================================================================
    // Progress Callback
    //==========================================================================

    using ProgressCallback = std::function<void(float progress)>;
    void setProgressCallback(ProgressCallback callback) {
        progressCallback_ = callback;
    }

private:
    ProgressCallback progressCallback_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void writeMetadata(juce::OutputStream& stream,
                     const WavetableExportOptions& options);
    void crossfadeFrames(float* dest, const float* src1,
                      const float* src2, int numSamples,
                      float crossfadeLength);
    void normalizeFrame(float* data, int numSamples);

    //==========================================================================
    // Format Writers
    //==========================================================================

    bool writeSerumChunk(juce::OutputStream& stream,
                       const WavetableData& wavetable,
                       const WavetableExportOptions& options);

    bool writeVitalChunk(juce::OutputStream& stream,
                       const WavetableData& wavetable,
                       const WavetableExportOptions& options);

    bool writeWavetableChunk(juce::OutputStream& stream,
                           const WavetableData& wavetable,
                           const WavetableExportOptions& options);
};

} // namespace zenith
