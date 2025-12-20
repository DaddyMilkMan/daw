#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>

namespace zenith {

class Engine;
class ProjectState;

class AudioExporter
{
public:
    enum class Format { WAV, AIFF, FLAC, MP3, OGG };

    struct ExportSettings {
        juce::File outputFile;
        int sampleRate = 48000;
        int bitDepth = 24;
        Format format = Format::WAV;
        bool normalize = true;
        double startTime = 0.0;
        double endTime = -1.0; // -1 = end of project
    };

    /**
     * @brief Exports the project to an audio file.
     * 
     * @param state The project state (used for duration calculation if needed).
     * @param engine The audio engine (used for rendering).
     * @param settings Export configuration.
     * @param progressCallback Callback for progress updates (0.0 to 1.0).
     */
    static void exportProject(ProjectState& state, Engine& engine, 
                            const ExportSettings& settings,
                            std::function<void(float)> progressCallback);

private:
    static juce::AudioFormat* getFormat(Format format, juce::AudioFormatManager& manager);
};

} // namespace zenith
