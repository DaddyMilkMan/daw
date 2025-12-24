/*
  ==============================================================================

    AudioExporter.h
    Created: 2025-12-23
    Author:  Zenith DAW

    High-quality offline project renderer.
    Supports bouncing projects to disk in non-realtime with normalization,
    stem export, and multiple formats.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include <functional>

namespace zenith {

class Engine;
class ProjectState;
class AudioRenderer;

/**
    Handles high-quality offline rendering of Zenith projects.
*/
class AudioExporter
{
public:
    enum class Format { WAV, AIFF, FLAC, MP3, OGG };

    struct ExportSettings {
        juce::File outputFile;
        double sampleRate = 44100.0;
        int bitDepth = 24;
        Format format = Format::WAV;
        bool normalize = false;
        float normalizeDb = -0.1f;
        bool exportStems = false;
        double startTimeSeconds = 0.0;
        double endTimeSeconds = -1.0; // -1 = end of project
        bool useDither = true;
    };

    explicit AudioExporter(Engine& engine);
    ~AudioExporter();

    /**
     * @brief Render the project to disk based on settings.
     * @param settings Configuration for the export
     * @param progressCallback Progress updates (0.0 to 1.0)
     * @return true if successful, false otherwise
     */
    bool renderProject(const ExportSettings& settings, 
                      std::function<void(float progress, const juce::String& status)> progressCallback);

    /**
     * @brief Cancel an active export operation.
     */
    void cancel();

    /**
     * @brief Check if an export is currently running.
     */
    bool isExporting() const { return isExporting_.load(); }

private:
    bool renderMixdown(const ExportSettings& settings, 
                      std::function<void(float, const juce::String&)> progressCallback);
    
    bool renderStems(const ExportSettings& settings, 
                    std::function<void(float, const juce::String&)> progressCallback);

    Engine& engine_;
    std::atomic<bool> isExporting_{ false };
    std::atomic<bool> shouldCancel_{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExporter)
};

} // namespace zenith
