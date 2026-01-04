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
#include <cstring>

namespace zenith {
namespace ai {

// RT-SAFE Lock-free circular buffer for audio data
// Uses proper memory ordering for thread-safe single-producer/single-consumer
template<typename T, size_t Size>
class LockFreeCircularBuffer {
public:
    LockFreeCircularBuffer() : writePos(0), readPos(0) {}
    
    // Producer thread: push data
    bool push(const T& item) {
        size_t currentWrite = writePos.load(std::memory_order_relaxed);
        size_t nextWrite = (currentWrite + 1) % Size;
        
        // Check if buffer is full (acquire to sync with consumer)
        if (nextWrite == readPos.load(std::memory_order_acquire)) {
            return false;  // Buffer full
        }
        
        // Write data
        buffer[currentWrite] = item;
        
        // Publish write position (release to make data visible to consumer)
        writePos.store(nextWrite, std::memory_order_release);
        return true;
    }
    
    // Consumer thread: pop data
    bool pop(T& item) {
        size_t currentRead = readPos.load(std::memory_order_relaxed);
        
        // Check if buffer is empty (acquire to sync with producer)
        if (currentRead == writePos.load(std::memory_order_acquire)) {
            return false;  // Buffer empty
        }
        
        // Read data
        item = buffer[currentRead];
        
        // Publish read position (release to make space visible to producer)
        readPos.store((currentRead + 1) % Size, std::memory_order_release);
        return true;
    }
    
    bool isEmpty() const {
        return readPos.load(std::memory_order_relaxed) == 
               writePos.load(std::memory_order_relaxed);
    }
    
    size_t size() const {
        size_t w = writePos.load(std::memory_order_relaxed);
        size_t r = readPos.load(std::memory_order_relaxed);
        return (w >= r) ? (w - r) : (Size - r + w);
    }
    
    void clear() {
        writePos.store(0, std::memory_order_relaxed);
        readPos.store(0, std::memory_order_relaxed);
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
    mutable std::atomic<uint32_t> analysisSequence{0}; // For Seqlock
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
    // RT-SAFE suggestion structure with fixed-size buffers
    struct Suggestion {
        char id[32];           // Fixed-size buffer instead of juce::String
        char type[16];         // Fixed-size buffer
        char message[128];     // Fixed-size buffer
        float parameterValue;  // Single float instead of juce::var
        float confidence;
        uint64_t timestamp;
        bool isActive;
        
        // Helper to set string fields safely
        void setId(const char* str) {
            std::strncpy(id, str, sizeof(id) - 1);
            id[sizeof(id) - 1] = '\0';
        }
        
        void setType(const char* str) {
            std::strncpy(type, str, sizeof(type) - 1);
            type[sizeof(type) - 1] = '\0';
        }
        
        void setMessage(const char* str) {
            std::strncpy(message, str, sizeof(message) - 1);
            message[sizeof(message) - 1] = '\0';
        }
    };
    
    RealTimeSuggestionEngine();
    ~RealTimeSuggestionEngine();
    
    // Audio thread interface (real-time safe)
    void processAudio(const AudioAnalysisData& analysis);
    
    // Background thread interface
    std::vector<Suggestion> getActiveSuggestions() const;
    void dismissSuggestion(const char* id);
    
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
    void removeOldSuggestions(uint64_t currentTime);
    
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
