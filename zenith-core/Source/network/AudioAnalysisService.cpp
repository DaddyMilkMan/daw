/*
  ==============================================================================

    AudioAnalysisService.cpp
    Created: 2025-11-29
    Author:  Dr. Maya Rodriguez (Lead Integration)

    Implementation of AudioAnalysisService
    Runs Python script via ChildProcess to analyze audio

  ==============================================================================
*/

#include "AudioAnalysisService.h"

namespace zenith {

//==============================================================================
// AudioAnalysisResults Implementation
//==============================================================================

juce::var AudioAnalysisResults::toJSON() const
{
    auto* obj = new juce::DynamicObject();
    
    // File info
    auto* fileInfo = new juce::DynamicObject();
    fileInfo->setProperty("duration_seconds", durationSeconds);
    fileInfo->setProperty("sample_rate", sampleRate);
    fileInfo->setProperty("is_stereo", isStereo);
    obj->setProperty("file_info", fileInfo);
    
    // Loudness
    auto* loudness = new juce::DynamicObject();
    loudness->setProperty("average_rms_db", averageRmsDb);
    loudness->setProperty("peak_rms_db", peakRmsDb);
    loudness->setProperty("peak_amplitude_db", peakAmplitudeDb);
    loudness->setProperty("dynamic_range_db", dynamicRangeDb);
    loudness->setProperty("headroom_db", headroomDb);
    obj->setProperty("loudness", loudness);
    
    // Spectral
    auto* spectral = new juce::DynamicObject();
    spectral->setProperty("centroid_hz", spectralCentroidHz);
    spectral->setProperty("rolloff_hz", spectralRolloffHz);
    spectral->setProperty("bandwidth_hz", spectralBandwidthHz);
    spectral->setProperty("brightness", brightness);
    obj->setProperty("spectral", spectral);
    
    // Frequency bands
    auto* bands = new juce::DynamicObject();
    bands->setProperty("sub_bass", subBassDb);
    bands->setProperty("bass", bassDb);
    bands->setProperty("low_mids", lowMidsDb);
    bands->setProperty("mids", midsDb);
    bands->setProperty("high_mids", highMidsDb);
    bands->setProperty("presence", presenceDb);
    bands->setProperty("brilliance", brillianceDb);
    obj->setProperty("frequency_bands", bands);
    
    // Rhythm
    auto* rhythm = new juce::DynamicObject();
    rhythm->setProperty("tempo_bpm", tempoBpm);
    rhythm->setProperty("beat_strength", beatStrength);
    obj->setProperty("rhythm", rhythm);
    
    // Stereo
    if (isStereo)
    {
        auto* stereo = new juce::DynamicObject();
        stereo->setProperty("correlation", stereoCorrelation);
        stereo->setProperty("width", stereoWidth);
        stereo->setProperty("balance", stereoBalance);
        obj->setProperty("stereo", stereo);
    }
    
    return juce::var(obj);
}

juce::String AudioAnalysisResults::toSummary() const
{
    juce::String summary;
    summary << "Audio Analysis Summary:\n";
    summary << "- Duration: " << juce::String(durationSeconds, 1) << "s\n";
    summary << "- Loudness: Avg " << juce::String(averageRmsDb, 1) << " dB, Peak " << juce::String(peakAmplitudeDb, 1) << " dB\n";
    summary << "- Dynamic Range: " << juce::String(dynamicRangeDb, 1) << " dB\n";
    summary << "- Tempo: " << juce::String(tempoBpm, 1) << " BPM\n";
    summary << "- Tonal Balance: " << brightness << "\n";
    
    if (isStereo)
        summary << "- Stereo Width: " << stereoWidth << "\n";
        
    return summary;
}

//==============================================================================
// AudioAnalysisService Implementation
//==============================================================================

class AudioAnalysisService::Impl : public juce::Thread
{
public:
    Impl() : juce::Thread("AudioAnalysisThread") {}
    
    ~Impl()
    {
        cancelAnalysis();
    }
    
    //==========================================================================
    void startAnalysis(
        const juce::File& file,
        std::function<void(AudioAnalysisResults)> onComplete,
        std::function<void(juce::String)> onError)
    {
        // Cancel any existing analysis
        cancelAnalysis();
        
        targetFile = file;
        completeCallback = onComplete;
        errorCallback = onError;
        
        startThread();
    }
    
    void cancelAnalysis()
    {
        signalThreadShouldExit();
        stopThread(2000);
    }
    
    //==========================================================================
    void run() override
    {
        if (!targetFile.existsAsFile())
        {
            notifyError("Audio file not found: " + targetFile.getFullPathName());
            return;
        }
        
        // Locate python script
        // Assuming script is in scripts/audio_analyzer.py relative to executable or project root
        // For development, we'll look in the source directory structure
        juce::File scriptFile = findScriptFile();
        
        if (!scriptFile.existsAsFile())
        {
            notifyError("Analysis script not found (audio_analyzer.py)");
            return;
        }
        
        // Run python script
        juce::ChildProcess process;
        juce::StringArray args;
        args.add("python3"); // Or "python" depending on system
        args.add(scriptFile.getFullPathName());
        args.add(targetFile.getFullPathName());
        
        if (!process.start(args))
        {
            // Try "python" if "python3" failed
            args.set(0, "python");
            if (!process.start(args))
            {
                notifyError("Failed to start Python process. Is Python installed?");
                return;
            }
        }
        
        // Read output
        juce::String output = process.readAllProcessOutput();
        int exitCode = process.getExitCode();
        
        if (exitCode != 0)
        {
            notifyError("Analysis failed (Exit code " + juce::String(exitCode) + ")");
            return;
        }
        
        // Parse JSON
        juce::var jsonResult = juce::JSON::parse(output);
        
        if (!jsonResult.isObject())
        {
            notifyError("Invalid output from analysis script");
            return;
        }
        
        bool success = jsonResult.getProperty("success", false);
        if (!success)
        {
            juce::String error = jsonResult.getProperty("error", "Unknown error");
            notifyError("Analysis error: " + error);
            return;
        }
        
        // Convert to results struct
        AudioAnalysisResults results;
        results.success = true;
        
        parseResults(jsonResult, results);
        
        // Notify success
        if (completeCallback)
        {
            juce::MessageManager::callAsync([this, results]()
            {
                if (completeCallback)
                    completeCallback(results);
            });
        }
    }
    
private:
    juce::File targetFile;
    std::function<void(AudioAnalysisResults)> completeCallback;
    std::function<void(juce::String)> errorCallback;
    
    juce::File findScriptFile()
    {
        // Try various locations
        juce::File current = juce::File::getCurrentWorkingDirectory();
        
        // 1. Check direct path (dev environment)
        juce::File script = current.getChildFile("zenith-core/scripts/audio_analyzer.py");
        if (script.existsAsFile()) return script;
        
        // 2. Check relative to executable
        juce::File exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        script = exe.getParentDirectory().getChildFile("scripts/audio_analyzer.py");
        if (script.existsAsFile()) return script;
        
        // 3. Hardcoded path for this session (fallback)
        return juce::File("c:\\zenith\\daw\\zenith-core\\scripts\\audio_analyzer.py");
    }
    
    void notifyError(const juce::String& error)
    {
        if (errorCallback)
        {
            juce::MessageManager::callAsync([this, error]()
            {
                if (errorCallback)
                    errorCallback(error);
            });
        }
    }
    
    void parseResults(const juce::var& json, AudioAnalysisResults& results)
    {
        // File info
        auto fileInfo = json["file_info"];
        results.durationSeconds = fileInfo["duration_seconds"];
        results.sampleRate = (int)fileInfo["sample_rate"];
        results.isStereo = fileInfo["is_stereo"];
        
        // Loudness
        auto loudness = json["loudness"];
        results.averageRmsDb = loudness["average_rms_db"];
        results.peakRmsDb = loudness["peak_rms_db"];
        results.peakAmplitudeDb = loudness["peak_amplitude_db"];
        results.dynamicRangeDb = loudness["dynamic_range_db"];
        results.headroomDb = loudness["headroom_db"];
        
        // Spectral
        auto spectral = json["spectral"];
        results.spectralCentroidHz = spectral["centroid_hz"];
        results.spectralRolloffHz = spectral["rolloff_hz"];
        results.spectralBandwidthHz = spectral["bandwidth_hz"];
        results.brightness = spectral["brightness"].toString();
        
        // Bands
        auto bands = json["frequency_bands"];
        results.subBassDb = bands["sub_bass"];
        results.bassDb = bands["bass"];
        results.lowMidsDb = bands["low_mids"];
        results.midsDb = bands["mids"];
        results.highMidsDb = bands["high_mids"];
        results.presenceDb = bands["presence"];
        results.brillianceDb = bands["brilliance"];
        
        results.dominantFrequencyHz = json["dominant_frequency_hz"];
        
        // Rhythm
        auto rhythm = json["rhythm"];
        results.tempoBpm = rhythm["tempo_bpm"];
        results.beatStrength = rhythm["beat_strength"];
        
        // Stereo
        if (results.isStereo && json.hasProperty("stereo"))
        {
            auto stereo = json["stereo"];
            results.stereoCorrelation = stereo["correlation"];
            results.stereoWidth = stereo["width"].toString();
            results.stereoBalance = stereo["balance"];
        }
    }
};

//==============================================================================
// Public Interface
//==============================================================================

AudioAnalysisService::AudioAnalysisService()
    : pImpl(std::make_unique<Impl>())
{
}

AudioAnalysisService::~AudioAnalysisService()
{
    pImpl->cancelAnalysis();
}

bool AudioAnalysisService::isAvailable() const
{
    // Check if python is available
    juce::ChildProcess process;
    if (process.start("python3 --version") || process.start("python --version"))
    {
        return true;
    }
    return false;
}

juce::String AudioAnalysisService::getAvailabilityError() const
{
    if (!isAvailable())
        return "Python environment not found. Please install Python to use audio analysis.";
    return "";
}

void AudioAnalysisService::analyzeAudioFile(
    const juce::File& audioFile,
    std::function<void(AudioAnalysisResults)> onComplete,
    std::function<void(juce::String)> onError)
{
    pImpl->startAnalysis(audioFile, onComplete, onError);
}

void AudioAnalysisService::cancelAnalysis()
{
    pImpl->cancelAnalysis();
}

} // namespace zenith
