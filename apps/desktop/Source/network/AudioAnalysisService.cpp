/*
  ==============================================================================

    AudioAnalysisService.cpp
    Created: 2025-11-29


    Implementation of AudioAnalysisService
    Runs Python script via ChildProcess to analyze audio

  ==============================================================================
*/

#include "AudioAnalysisService.h"
#include <juce_events/juce_events.h>

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

// #include <unistd.h>  <-- Removed POSIX headers
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <poll.h>

//==============================================================================
/**
    A persistent worker process for audio analysis using juce::ChildProcess
*/
class AnalysisWorker : private juce::Thread
{
public:
    AnalysisWorker(const juce::File& script)
        : juce::Thread("AnalysisWorker"), scriptFile(script)
    {
        startThread();
    }
    
    ~AnalysisWorker()
    {
        stop();
    }
    
    bool isBusy() const { return busy; }
    
    void analyze(const juce::File& file, 
                 std::function<void(AudioAnalysisResults)> onComplete,
                 std::function<void(juce::String)> onError)
    {
        busy = true;
        targetFile = file;
        completeCallback = onComplete;
        errorCallback = onError;
        
        // Notify the thread to send the file path to Python
        workSignal.signal();
    }
    
    bool isAlive() const { return process.isRunning(); }
    
    void stop()
    {
        shouldExit = true;
        workSignal.signal();
        
        if (isAlive())
        {
            // Send quit command to python
            process.start("echo quit");
            
            // Wait a bit for graceful exit
            if (!process.waitForProcessToFinish(2000))
            {
                process.kill();
            }
        }
        
        stopThread(2000);
    }

private:
    void run() override
    {
        while (!threadShouldExit() && !shouldExit)
        {
            if (!isAlive())
            {
                if (!startProcess())
                {
                    wait(2000);
                    continue;
                }
            }
            
            // Wait for work
            if (workSignal.wait(1000))
            {
                if (shouldExit) break;
                
                if (targetFile.existsAsFile())
                {
                    // Send path to Python
                    juce::String path = targetFile.getFullPathName() + "\n";
                    if (process.start("echo " + path))
                    {
                        // Read response
                        juce::String response = readResponse();
                        
                        if (response.isNotEmpty())
                            handleResponse(response);
                        else
                            notifyError("Empty response from analysis worker");
                    }
                    else
                    {
                        notifyError("Failed to write to worker process");
                    }
                }
                else
                {
                    notifyError("File not found: " + targetFile.getFullPathName());
                }
                
                busy = false;
            }
        }
    }
    
    bool startProcess()
    {
        juce::StringArray args;
        args.add("python3"); // Try python3 first
        args.add(scriptFile.getFullPathName());
        
        if (process.start(args))
        {
             return waitForReady();
        }
        
        // Fallback to "python"
        args.set(0, "python");
        if (process.start(args))
        {
             return waitForReady();
        }
        
        return false;
    }
    
    bool waitForReady()
    {
        // Read "ready" message
        juce::String readyMsg = readResponse();
        if (readyMsg.isEmpty()) return false;
        
        auto json = juce::JSON::parse(readyMsg);
        return json["status"].toString() == "ready";
    }

    juce::String readResponse()
    {
        juce::MemoryBlock buffer;
        char c;
        
        auto startTime = juce::Time::getMillisecondCounter();
        
        // Read until newline or timeout
        while (!threadShouldExit())
        {
            int numRead = process.readProcessOutput(&c, 1);
            
            if (numRead > 0)
            {
                if (c == '\n') break;
                buffer.append(&c, 1);
            }
            else
            {
                // Small sleep to avoid spinning if no data yet
                juce::Thread::sleep(10);
                
                if (!process.isRunning()) return {};
            }
            
            if (juce::Time::getMillisecondCounter() - startTime > 10000) // 10s timeout
                break;
        }
        
        return buffer.toString();
    }
    
    void handleResponse(const juce::String& response)
    {
        auto json = juce::JSON::parse(response);
        
        if (!json.isObject())
        {
            notifyError("Invalid JSON response from worker");
            return;
        }
        
        bool success = json.getProperty("success", false);
        if (!success)
        {
            juce::String error = json.getProperty("error", "Unknown error");
            notifyError("Analysis error: " + error);
            return;
        }
        
        AudioAnalysisResults results;
        results.success = true;
        
        // We'll call the parser from AudioAnalysisService::Impl
        if (parserFunc)
            parserFunc(json, results);
            
        if (completeCallback)
        {
            auto cb = std::move(completeCallback);
            juce::MessageManager::callAsync([cb, results]() { if (cb) cb(results); });
        }
    }
    
    void notifyError(const juce::String& error)
    {
        if (errorCallback)
        {
            auto cb = std::move(errorCallback);
            juce::MessageManager::callAsync([cb, error]() { if (cb) cb(error); });
        }
    }

    juce::File scriptFile;
    juce::File targetFile;
    juce::ChildProcess process;
    
    std::atomic<bool> busy { false };
    std::atomic<bool> shouldExit { false };
    juce::WaitableEvent workSignal;
    
    std::function<void(AudioAnalysisResults)> completeCallback;
    std::function<void(juce::String)> errorCallback;
    
public:
    std::function<void(const juce::var&, AudioAnalysisResults&)> parserFunc;
};

//==============================================================================

class AudioAnalysisService::Impl
{
public:
    Impl() {}
    
    ~Impl()
    {
        cancelAnalysis();
    }
    
    void startAnalysis(
        const juce::File& file,
        std::function<void(AudioAnalysisResults)> onComplete,
        std::function<void(juce::String)> onError)
    {
        const juce::ScopedLock sl(lock);
        
        juce::File scriptFile = findScriptFile();
        if (!scriptFile.existsAsFile())
        {
            if (onError) onError("Analysis script not found (audio_analyzer.py)");
            return;
        }
        
        // Find an idle worker or create a new one (max 4 workers)
        AnalysisWorker* worker = nullptr;
        for (auto& w : workers)
        {
            if (!w->isBusy())
            {
                worker = w.get();
                break;
            }
        }
        
        if (worker == nullptr && workers.size() < 4)
        {
            auto newWorker = std::make_unique<AnalysisWorker>(scriptFile);
            newWorker->parserFunc = [this](const juce::var& v, AudioAnalysisResults& r) { parseResults(v, r); };
            worker = newWorker.get();
            workers.push_back(std::move(newWorker));
        }
        
        if (worker != nullptr)
        {
            worker->analyze(file, onComplete, onError);
        }
        else
        {
            // All workers busy, use the first one (it will eventually get to it if we had a queue)
            // For now, simpler: round robin
            static int lastWorker = 0;
            lastWorker = (lastWorker + 1) % workers.size();
            workers[lastWorker]->analyze(file, onComplete, onError);
        }
    }
    
    void cancelAnalysis()
    {
        const juce::ScopedLock sl(lock);
        for (auto& w : workers)
            w->stop();
        workers.clear();
    }

    void parseResults(const juce::var& json, AudioAnalysisResults& results)
    {
        try {
            // File info
            if (json.hasProperty("file_info"))
            {
                auto fileInfo = json["file_info"];
                results.durationSeconds = fileInfo.getProperty("duration_seconds", 0.0);
                results.sampleRate = (int)fileInfo.getProperty("sample_rate", 0);
                results.isStereo = fileInfo.getProperty("is_stereo", false);
            }
            
            // Loudness
            if (json.hasProperty("loudness"))
            {
                auto loudness = json["loudness"];
                results.averageRmsDb = loudness.getProperty("average_rms_db", 0.0);
                results.peakRmsDb = loudness.getProperty("peak_rms_db", 0.0);
                results.peakAmplitudeDb = loudness.getProperty("peak_amplitude_db", 0.0);
                results.dynamicRangeDb = loudness.getProperty("dynamic_range_db", 0.0);
                results.headroomDb = loudness.getProperty("headroom_db", 0.0);
            }
            
            // Spectral
            if (json.hasProperty("spectral"))
            {
                auto spectral = json["spectral"];
                results.spectralCentroidHz = spectral.getProperty("centroid_hz", 0.0);
                results.spectralRolloffHz = spectral.getProperty("rolloff_hz", 0.0);
                results.spectralBandwidthHz = spectral.getProperty("bandwidth_hz", 0.0);
                results.brightness = spectral.getProperty("brightness", "unknown").toString();
            }
            
            // Bands
            if (json.hasProperty("frequency_bands"))
            {
                auto bands = json["frequency_bands"];
                results.subBassDb = bands.getProperty("sub_bass", 0.0);
                results.bassDb = bands.getProperty("bass", 0.0);
                results.lowMidsDb = bands.getProperty("low_mids", 0.0);
                results.midsDb = bands.getProperty("mids", 0.0);
                results.highMidsDb = bands.getProperty("high_mids", 0.0);
                results.presenceDb = bands.getProperty("presence", 0.0);
                results.brillianceDb = bands.getProperty("brilliance", 0.0);
            }
            
            results.dominantFrequencyHz = json.getProperty("dominant_frequency_hz", 0.0);
            
            // Rhythm
            if (json.hasProperty("rhythm"))
            {
                auto rhythm = json["rhythm"];
                results.tempoBpm = rhythm.getProperty("tempo_bpm", 0.0);
                results.beatStrength = rhythm.getProperty("beat_strength", 0.0);
            }
            
            // Stereo
            if (results.isStereo && json.hasProperty("stereo"))
            {
                auto stereo = json["stereo"];
                results.stereoCorrelation = stereo.getProperty("correlation", 0.0);
                results.stereoWidth = stereo.getProperty("width", "unknown").toString();
                results.stereoBalance = stereo.getProperty("balance", 0.0);
            }
        }
        catch (const std::exception& e)
        {
            results.success = false;
            results.errorMessage = juce::String("Error parsing analysis results: ") + e.what();
        }
    }

private:
    juce::File findScriptFile()
    {
        // 1. Check relative to executable (Production layout: Resources/Scripts/)
        auto exeFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto script = exeFile.getSiblingFile("Resources").getChildFile("Scripts").getChildFile("audio_analyzer.py");
        if (script.existsAsFile()) return script;

        // 2. Check sibling to executable (Flat layout)
        script = exeFile.getSiblingFile("audio_analyzer.py");
        if (script.existsAsFile()) return script;

        // 3. Check in a scripts subfolder relative to executable
        script = exeFile.getSiblingFile("scripts").getChildFile("audio_analyzer.py");
        if (script.existsAsFile()) return script;
        
        // 4. Check various development locations
        juce::File current = juce::File::getCurrentWorkingDirectory();
        
        // Try zenith-core/scripts/
        script = current.getChildFile("zenith-core/scripts/audio_analyzer.py");
        if (script.existsAsFile()) return script;
        
        // Try apps/desktop/scripts/ (from build dir)
        script = exeFile.getParentDirectory().getParentDirectory().getParentDirectory().getChildFile("scripts/audio_analyzer.py");
        if (script.existsAsFile()) return script;

        return {}; // Not found
    }

    std::vector<std::unique_ptr<AnalysisWorker>> workers;
    juce::CriticalSection lock;
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
    static bool hasChecked = false;
    static bool isReady = false;
    
    if (hasChecked)
        return isReady;
        
    // Check if python is available
    juce::ChildProcess process;
    if (process.start("python3 --version") || process.start("python --version"))
    {
        isReady = true;
    }
    
    hasChecked = true;
    return isReady;
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
