/*
  ==============================================================================

    AudioExporter.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Handles offline bouncing/rendering of the project.
    Features:
    - 2-Pass Normalization (Analyze peaks → Apply gain)
    - Professional TPDF Dithering with optional noise shaping
    - Multi-format support (WAV, FLAC, OGG, AIFF)
    - Asynchronous multi-file stem export

  ==============================================================================
*/

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include "../dsp/Dither.h"
#include "ExportCommon.h"
#include <functional>
#include <vector>
#include <atomic>
#include <memory>

namespace zenith {

class Engine;
class ExportJob;



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

  /// Completion callback type for async exports
  using ExportCompletionCallback = std::function<void(juce::Result)>;

  /**
   * @brief Perform project export asynchronously (non-blocking)
   * 
   * This is the preferred method for UI integration as it does not block
   * the message thread. Progress and completion are reported via callbacks.
   * 
   * @param options Export options (format, sample rate, output file, etc.)
   * @param completion Callback when export finishes (success or error with message)
   * @return true if export was started, false if another export is already running
   * 
   * @code
   * exporter.exportProjectAsync(options, [](juce::Result result) {
   *     if (result.wasOk())
   *         showMessage("Export complete!");
   *     else
   *         showError(result.getErrorMessage());
   * });
   * @endcode
   */
  bool exportProjectAsync(const ExportOptions& options, 
                          ExportCompletionCallback completion = nullptr);

  /// Cancel any ongoing export
  void cancelExport();

  /// Check if export is in progress
  bool isExporting() const { return isExporting_.load(); }

  /// Get stem progress (for async exports)
  std::vector<StemExportProgress> getStemProgress() const;

  /// Export a single track as a stem file (uses outputFile from options)
  bool exportSingleStem(int trackIndex, const ExportOptions &options);

private:
  /// Export processing block size - used consistently across all export methods
  static constexpr int kExportBlockSize = 4096;

  Engine &engine_;
  juce::AudioFormatManager formatManager_;
  
  std::atomic<bool> shouldCancel_{false};
  std::atomic<bool> isExporting_{false};
  
  mutable juce::CriticalSection stemProgressLock_;
  std::vector<StemExportProgress> stemProgress_;
  std::atomic<int> completedStemCount_{0};
  
  /// Current async export job (for cancellation)
  ExportJob* currentExportJob_{nullptr};
  mutable juce::CriticalSection exportJobLock_;

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
