/*
  ==============================================================================
    AudioThreadSafeProcessor.h
    Lock-free audio processing for real-time collaboration
    Phase 3: Autonomous Intelligence (10/10) - Fixed Implementation
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <array>

namespace zenith {
namespace ai {

// Lock-free circular buffer for audio data
template<typename T, size_t Size>
class LockFreeCircularBuffer {
public:
    LockFreeCircularBuffer() : writePos(0), readPos(0) {}
    
    bool push(const T& item) {
        size_t nextWrite = (writePos.load() + 1) % Size;
        if (nextWrite == readPos.load()) {
            return false;  // Buffer full
        }
        
        buffer[writePos.load()] = item;
        writePos.store(nextWrite);
        return true;
    }
    
    bool pop(T& item) {
        if (readPos.load() == writePos.load()) {
            return false;  // Buffer empty
        }
        
        item = buffer[readPos.load()];
        readPos.store((readPos.load() + 1) % Size);
        return true;
    }
    
    bool isEmpty() const {
        return readPos.load() == writePos.load();
    }
    
    size_t size() const {
        size_t w = writePos.load();
        size_t r = readPos.load();
        return (w >= r) ? (w - r) : (Size - r + w);
    }
    
    void clear() {
        writePos.store(0);
        readPos.store(0);
    }
    
private:
    std::array<T, Size> buffer;
    std::atomic<size_t> writePos;
    std::atomic<size_t> readPos;
};

// Audio analysis data structure
struct AudioAnalysisData {
    float loudness = 0.0f;
    float dynamics = 0.0f;
    float stereoWidth = 0.0f;
    float frequencyBalance = 0.0f;
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    uint64_t timestamp = 0;
    bool isValid = false;
};

// Thread-safe audio processor
class AudioThreadSafeProcessor {
public:
    AudioThreadSafeProcessor();
    ~AudioThreadSafeProcessor();
    
    // Audio thread processing (must be real-time safe)
    void processAudioBlock(const juce::AudioBuffer<float>& buffer,
                          double sampleRate,
                          int numSamples);
    
    // Background thread analysis (can block)
    AudioAnalysisData getLatestAnalysis() const;
    bool hasNewAnalysis() const;
    void markAnalysisRead();
    
    // Configuration
    void setAnalysisRate(float Hz);  // How often to analyze
    void setBufferSize(int samples);
    
private:
    // Lock-free buffers
    static constexpr size_t AUDIO_BUFFER_SIZE = 8192;
    static constexpr size_t ANALYSIS_BUFFER_SIZE = 64;
    
    LockFreeCircularBuffer<juce::AudioBuffer<float>, AUDIO_BUFFER_SIZE> audioBuffer;
    LockFreeCircularBuffer<AudioAnalysisData, ANALYSIS_BUFFER_SIZE> analysisBuffer;
    
    // Analysis state
    std::atomic<float> analysisRate{10.0f};  // 10 Hz default
    std::atomic<int> samplesSinceAnalysis{0};
    std::atomic<int> samplesPerAnalysis{0};
    std::atomic<bool> newAnalysisAvailable{false};
    AudioAnalysisData lastAnalysis;
    
    // Analysis methods (real-time safe)
    float calculateLoudness(const juce::AudioBuffer<float>& buffer);
    float calculateDynamics(const juce::AudioBuffer<float>& buffer);
    float calculateStereoWidth(const juce::AudioBuffer<float>& buffer);
    float calculateFrequencyBalance(const juce::AudioBuffer<float>& buffer);
    
    // Background processing
    void performAnalysis(const juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioThreadSafeProcessor)
};

// Real-time suggestion system
class RealTimeSuggestionEngine {
public:
    struct Suggestion {
        juce::String id;
        juce::String type;
        juce::String message;
        juce::var parameters;
        float confidence;
        uint64_t timestamp;
        bool isActive;
    };
    
    RealTimeSuggestionEngine();
    ~RealTimeSuggestionEngine();
    
    // Audio thread interface (real-time safe)
    void processAudio(const AudioAnalysisData& analysis);
    
    // Background thread interface
    std::vector<Suggestion> getActiveSuggestions() const;
    void dismissSuggestion(const juce::String& id);
    
    // Configuration
    void setSensitivity(float sensitivity);
    void setMaxSuggestions(int maxSuggestions);
    
private:
    // Lock-free suggestion storage
    static constexpr size_t MAX_SUGGESTIONS = 16;
    std::array<Suggestion, MAX_SUGGESTIONS> suggestions;
    std::atomic<size_t> suggestionCount{0};
    
    // Analysis history for trend detection
    static constexpr size_t HISTORY_SIZE = 100;
    std::array<AudioAnalysisData, HISTORY_SIZE> analysisHistory;
    std::atomic<size_t> historyWritePos{0};
    
    // Configuration
    std::atomic<float> sensitivity{0.5f};
    std::atomic<int> maxSuggestions{5};
    
    // Suggestion generation (real-time safe)
    void generateSuggestions(const AudioAnalysisData& current);
    void addSuggestion(const Suggestion& suggestion);
    void removeOldSuggestions();
    
    // Trend detection
    bool detectLoudnessTrend(const AudioAnalysisData& current);
    bool detectDynamicsIssue(const AudioAnalysisData& current);
    bool detectStereoIssue(const AudioAnalysisData& current);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeSuggestionEngine)
};

// Combined real-time processor
class RealTimeAudioProcessor {
public:
    RealTimeAudioProcessor();
    ~RealTimeAudioProcessor();
    
    // Main audio processing (real-time safe)
    void processAudio(const juce::AudioBuffer<float>& buffer,
                     double sampleRate);
    
    // Get results (background thread safe)
    std::vector<RealTimeSuggestionEngine::Suggestion> getSuggestions() const;
    AudioAnalysisData getAnalysis() const;
    
    // Configuration
    void setSensitivity(float sensitivity);
    void setAnalysisRate(float Hz);
    
private:
    std::unique_ptr<AudioThreadSafeProcessor> audioProcessor;
    std::unique_ptr<RealTimeSuggestionEngine> suggestionEngine;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeAudioProcessor)
};

} // namespace ai
} // namespace zenith
