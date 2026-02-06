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

#include <juce_audio_formats/juce_audio_formats.h> // For juce::AudioFormat
#include <juce_core/juce_core.h>
#include "../dsp/Dither.h"
#include <functional>
#include <vector>

namespace zenith {

/// Export format enumeration
enum class ExportFormat { WAV, FLAC, OGG, AIFF };

/// Progress callback type for export operations
using ExportProgressCallback = std::function<void(float progress, const juce::String& status)>;

/// Export options for offline audio rendering
struct ExportOptions {
  juce::File outputFile;
  double sampleRate = 44100.0;
  int bitDepth = 24; // 8, 16, 24, 32
  ExportFormat format = ExportFormat::WAV;
  
  // Dithering options
  bool enableDither = true;
  dsp::DitherType ditherType = dsp::DitherType::ShapedTPDF;
  
  // Normalization options
  bool normalize = false;
  double normalizeDb = -0.1;
  
  // Time range
  double startTime = 0.0;
  double duration = 0.0;  // 0 = auto-detect
  
  // Stem export options
  bool exportStems = false;
  bool exportStemsAsync = true;  // Use thread pool for parallel stem export
  std::vector<int> stemTrackIndices; // Empty = all tracks
  
  // Progress callback (optional)
  ExportProgressCallback progressCallback = nullptr;
};

} // namespace zenith
