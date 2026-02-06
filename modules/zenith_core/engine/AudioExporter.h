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

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include "../dsp/Dither.h"
#include <functional>
#include <vector>
#include <atomic>
#include <memory>

namespace zenith {

class Engine;

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

/// Progress tracking for individual stem exports
struct StemExportProgress {
  int trackIndex = -1;
  juce::String trackName;
  float progress = 0.0f;
  bool completed = false;
  bool success = false;
};

/**
 * @class AudioExporter
 * @brief Professional audio export engine with normalization, dithering, and stem export
 */
class AudioExporter {
public:
  AudioExporter(Engine &engine);
  ~AudioExporter();

  /// Perform project export (blocking unless async stems)
  bool exportProject(const ExportOptions &options);

  /// Cancel any ongoing export
  void cancelExport();

  /// Check if export is in progress
  bool isExporting() const { return isExporting_.load(); }

  /// Get stem progress (for async exports)
  std::vector<StemExportProgress> getStemProgress() const;

  /// Export a single track as a stem file (uses outputFile from options)
  bool exportSingleStem(int trackIndex, const ExportOptions &options);

private:
  Engine &engine_;
  juce::AudioFormatManager formatManager_;
  
  std::atomic<bool> shouldCancel_{false};
  std::atomic<bool> isExporting_{false};
  
  mutable juce::CriticalSection stemProgressLock_;
  std::vector<StemExportProgress> stemProgress_;
  std::atomic<int> completedStemCount_{0};

  void registerFormats();
  juce::AudioFormat* getFormatForType(ExportFormat format);

  bool renderToTempFile(const juce::File &tempFile, double duration,
                        double sampleRate, double startTime, float &outMaxPeak);
  bool analyzeProjectPeak(double duration, double sampleRate, double startTime, float &outMaxPeak);
  bool writeFinalFile(const juce::File &tempFile, const ExportOptions &options,
                      float maxPeak);

  bool exportStems(const ExportOptions &options);
  bool exportStemsAsync(const ExportOptions &options);
  bool exportSingleStemInternal(int trackIndex, const ExportOptions &options, const juce::File& stemOutputFile, float normalizationGain = 1.0f);

  void updateStemProgress(int trackIndex, float progress);
  void markStemComplete(int trackIndex, bool success);
  void reportAggregateProgress(const ExportOptions& options);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExporter)
};

} // namespace zenith
