/*
  ==============================================================================

    AudioExporter.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Handles offline bouncing/rendering of the project.
    Features:
    - 2-Pass Normalization
    - Dithering
    - Multi-format support (WAV, FLAC, OGG)
    - 8-bit WAV support

  ==============================================================================
*/

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <functional>
#include <vector>

namespace zenith {

class Engine;

/// Export format enumeration
enum class ExportFormat { WAV, FLAC, OGG, AIFF };

/// Progress callback type for export operations
/// @param progress Progress value from 0.0 to 1.0
/// @param status Human-readable status message
using ExportProgressCallback = std::function<void(float progress, const juce::String& status)>;

/// Export options for offline audio rendering
struct ExportOptions {
  juce::File outputFile;
  double sampleRate = 44100.0;
  int bitDepth = 24; // 8, 16, 24, 32
  ExportFormat format = ExportFormat::WAV;
  bool enableDither = true;
  bool normalize = false;
  double normalizeDb = -0.1;
  double duration = 0.0;
  
  // Stem export options
  bool exportStems = false;
  std::vector<int> stemTrackIndices; // Empty = all tracks
  
  // Progress callback (optional)
  ExportProgressCallback progressCallback = nullptr;
};

class AudioExporter {
public:
  AudioExporter(Engine &engine);
  ~AudioExporter() = default;

  /**
   * @brief Perform project export
   * This is a BLOCKING operation (should run on a background thread ideally,
   * but runs on message thread for MVP simplicity unless async is requested).
   */
  bool exportProject(const ExportOptions &options);

private:
  Engine &engine_;
  juce::AudioFormatManager formatManager;

  // Helpers
  void registerFormats();

  // Normalization Helpers
  // Pass 1: Render to temporary float32 file and find peak
  bool renderToTempFile(const juce::File &tempFile, double duration,
                        double sampleRate, float &outMaxPeak);

  // Pass 2: Read temp file, apply gain/dither, write to final file
  bool writeFinalFile(const juce::File &tempFile, const ExportOptions &options,
                      float maxPeak);

  /**
   * @brief Export individual track stems
   * @param options Export options (stemTrackIndices specifies which tracks)
   * @return true if all stems exported successfully
   */
  bool exportStems(const ExportOptions &options);
  
  /**
   * @brief Export a single track as a stem
   * @param trackIndex Index of track to export
   * @param options Export options
   * @return true if successful
   */
  bool exportSingleStem(int trackIndex, const ExportOptions &options);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExporter)
};

} // namespace zenith
