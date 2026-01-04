/*
  ==============================================================================
    AudioThreadSafeProcessor.cpp
    Lock-free audio processing implementation
  ==============================================================================
*/

#include "AudioThreadSafeProcessor.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace zenith {
namespace ai {

// AudioThreadSafeProcessor Implementation
AudioThreadSafeProcessor::AudioThreadSafeProcessor() {
    samplesPerAnalysis.store(static_cast<int>(44100.0f / analysisRate.load()));
}

AudioThreadSafeProcessor::~AudioThreadSafeProcessor() = default;

// RT-SAFE: This function is called from the audio thread and must not allocate
void AudioThreadSafeProcessor::processAudioBlock(const juce::AudioBuffer<float>& buffer,
                                                double sampleRate,
                                                int numSamples) {
    // WARNING: Copying AudioBuffer allocates memory - NOT RT-SAFE!
    // This should be refactored to use a lock-free FIFO or pointer queue
    // For now, we skip the buffer copy to maintain RT-safety and only perform
    // in-place analysis on the provided buffer reference
    
    // Perform real-time analysis at specified rate
    samplesSinceAnalysis.fetch_add(numSamples, std::memory_order_relaxed);
    
    if (samplesSinceAnalysis.load(std::memory_order_relaxed) >= samplesPerAnalysis.load(std::memory_order_relaxed)) {
        performAnalysis(buffer);
        samplesSinceAnalysis.store(0, std::memory_order_relaxed);
    }
}

void AudioThreadSafeProcessor::performAnalysis(const juce::AudioBuffer<float>& buffer) {
    AudioAnalysisData analysis;
    
    // Calculate metrics (real-time safe algorithms)
    analysis.loudness = calculateLoudness(buffer);
    analysis.dynamics = calculateDynamics(buffer);
    analysis.stereoWidth = calculateStereoWidth(buffer);
    analysis.frequencyBalance = calculateFrequencyBalance(buffer);
    
    // Calculate peak and RMS
    float peak = 0.0f;
    float rms = 0.0f;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = buffer.getSample(ch, i);
            peak = std::max(peak, std::abs(sample));
            rms += sample * sample;
        }
    }
    
    rms = std::sqrt(rms / (numSamples * numChannels));
    analysis.peakLevel = peak;
    analysis.rmsLevel = rms;
    
    // RT-SAFE: Use atomic counter instead of system time call
    // juce::Time::getCurrentTime() may allocate or make system calls - NOT RT-SAFE!
    static std::atomic<uint64_t> rtSafeTimestamp{0};
    analysis.timestamp = rtSafeTimestamp.fetch_add(1, std::memory_order_relaxed);
    analysis.isValid = true;
    
    // 1. Push to analysis buffer
    if (!analysisBuffer.push(analysis)) {
        AudioAnalysisData dropped;
        analysisBuffer.pop(dropped);
        analysisBuffer.push(analysis);
    }
    
    // 2. Update lastAnalysis using Seqlock (RT-safe)
    analysisSequence.fetch_add(1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    
    lastAnalysis = analysis;
    
    std::atomic_thread_fence(std::memory_order_release);
    analysisSequence.fetch_add(1, std::memory_order_relaxed);
    
    newAnalysisAvailable.store(true);
}

float AudioThreadSafeProcessor::calculateLoudness(const juce::AudioBuffer<float>& buffer) {
    // Simplified loudness calculation (LUFS approximation)
    float sum = 0.0f;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = buffer.getSample(ch, i);
            sum += sample * sample;
        }
    }
    
    float rms = std::sqrt(sum / (numSamples * numChannels));
    return juce::Decibels::gainToDecibels(rms + 1e-10f);
}

float AudioThreadSafeProcessor::calculateDynamics(const juce::AudioBuffer<float>& buffer) {
    // Calculate crest factor as dynamics measure
    float peak = 0.0f;
    float rms = 0.0f;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = buffer.getSample(ch, i);
            peak = std::max(peak, std::abs(sample));
            rms += sample * sample;
        }
    }
    
    rms = std::sqrt(rms / (numSamples * numChannels));
    return 20.0f * std::log10((peak + 1e-10f) / (rms + 1e-10f));
}

float AudioThreadSafeProcessor::calculateStereoWidth(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return 0.0f;
    
    float correlation = 0.0f;
    float leftPower = 0.0f;
    float rightPower = 0.0f;
    int numSamples = buffer.getNumSamples();
    
    for (int i = 0; i < numSamples; ++i) {
        float left = buffer.getSample(0, i);
        float right = buffer.getSample(1, i);
        
        correlation += left * right;
        leftPower += left * left;
        rightPower += right * right;
    }
    
    if (leftPower > 0.0f && rightPower > 0.0f) {
        correlation /= std::sqrt(leftPower * rightPower);
        return std::sqrt(2.0f * (1.0f - correlation));  // Stereo width from correlation
    }
    
    return 0.0f;
}

float AudioThreadSafeProcessor::calculateFrequencyBalance(const juce::AudioBuffer<float>& buffer) {
    // Simplified frequency balance (low vs high energy)
    // In practice, would use FFT
    float lowEnergy = 0.0f;
    float highEnergy = 0.0f;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    // Simple high-pass/low-pass approximation using differences
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 1; i < numSamples; ++i) {
            float sample = buffer.getSample(ch, i);
            float prevSample = buffer.getSample(ch, i - 1);
            float diff = sample - prevSample;
            
            // High frequency content (differences)
            highEnergy += diff * diff;
            
            // Low frequency content (absolute values)
            lowEnergy += sample * sample;
        }
    }
    
    if (lowEnergy > 0.0f) {
        return highEnergy / lowEnergy;  // Higher ratio = brighter
    }
    
    return 0.5f;  // Neutral
}

AudioAnalysisData AudioThreadSafeProcessor::getLatestAnalysis() const {
    AudioAnalysisData analysis;
    uint32_t s1, s2;
    
    // Seqlock read (Lock-free)
    do {
        s1 = analysisSequence.load(std::memory_order_acquire);
        analysis = lastAnalysis;
        s2 = analysisSequence.load(std::memory_order_acquire);
    } while ((s1 & 1) != 0 || s1 != s2);
    
    return analysis;
}

bool AudioThreadSafeProcessor::hasNewAnalysis() const {
    return newAnalysisAvailable.load();
}

void AudioThreadSafeProcessor::markAnalysisRead() {
    newAnalysisAvailable.store(false);
}

void AudioThreadSafeProcessor::setAnalysisRate(float Hz) {
    analysisRate.store(Hz);
    samplesPerAnalysis.store(static_cast<int>(44100.0f / Hz));
}

void AudioThreadSafeProcessor::setBufferSize(int samples) {
    // Buffer size is fixed by template, but we can validate
    juce::ignoreUnused(samples);
}

// RealTimeSuggestionEngine Implementation
RealTimeSuggestionEngine::RealTimeSuggestionEngine() {
    // Initialize suggestions array
    for (auto& suggestion : suggestions) {
        suggestion.isActive = false;
    }
}

RealTimeSuggestionEngine::~RealTimeSuggestionEngine() = default;

void RealTimeSuggestionEngine::processAudio(const AudioAnalysisData& analysis) {
    if (!analysis.isValid) return;
    
    // Add to history (RT-safe with relaxed memory ordering)
    size_t writePos = historyWritePos.fetch_add(1, std::memory_order_relaxed) % HISTORY_SIZE;
    analysisHistory[writePos] = analysis;
    
    // Generate suggestions (RT-safe)
    generateSuggestions(analysis);
    
    // Remove old suggestions (RT-safe)
    removeOldSuggestions();
}

void RealTimeSuggestionEngine::generateSuggestions(const AudioAnalysisData& current) {
    // RT-SAFE: All string operations now use fixed-size buffers with no allocations
    
    // Loudness issues
    if (detectLoudnessTrend(current)) {
        Suggestion suggestion;
        // Use snprintf for RT-safe string formatting (no allocation)
        char idBuf[32];
        std::snprintf(idBuf, sizeof(idBuf), "loud_%llu", 
                     static_cast<unsigned long long>(current.timestamp));
        suggestion.setId(idBuf);
        suggestion.setType("loudness");
        suggestion.setMessage(current.loudness < -20.0f ? 
                             "Loudness is trending low" : 
                             "Loudness is trending high");
        suggestion.parameterValue = current.loudness;
        suggestion.confidence = 0.8f;
        suggestion.timestamp = current.timestamp;
        suggestion.isActive = true;
        
        addSuggestion(suggestion);
    }
    
    // Dynamics issues
    if (detectDynamicsIssue(current)) {
        Suggestion suggestion;
        char idBuf[32];
        std::snprintf(idBuf, sizeof(idBuf), "dyn_%llu", 
                     static_cast<unsigned long long>(current.timestamp));
        suggestion.setId(idBuf);
        suggestion.setType("dynamics");
        suggestion.setMessage(current.dynamics < 6.0f ? 
                             "Dynamic range is low" : 
                             "Dynamic range is high");
        suggestion.parameterValue = current.dynamics;
        suggestion.confidence = 0.7f;
        suggestion.timestamp = current.timestamp;
        suggestion.isActive = true;
        
        addSuggestion(suggestion);
    }
    
    // Stereo issues
    if (detectStereoIssue(current)) {
        Suggestion suggestion;
        char idBuf[32];
        std::snprintf(idBuf, sizeof(idBuf), "stereo_%llu", 
                     static_cast<unsigned long long>(current.timestamp));
        suggestion.setId(idBuf);
        suggestion.setType("stereo");
        suggestion.setMessage(current.stereoWidth < 0.5f ? 
                             "Stereo width is narrow" : 
                             "Stereo width is wide");
        suggestion.parameterValue = current.stereoWidth;
        suggestion.confidence = 0.6f;
        suggestion.timestamp = current.timestamp;
        suggestion.isActive = true;
        
        addSuggestion(suggestion);
    }
}

bool RealTimeSuggestionEngine::detectLoudnessTrend(const AudioAnalysisData& current) {
    // RT-SAFE: Check last 10 analyses for trend
    int historyCount = std::min(10, static_cast<int>(historyWritePos.load(std::memory_order_relaxed)));
    if (historyCount < 3) return false;
    
    float sum = 0.0f;
    int count = 0;
    
    for (int i = 0; i < historyCount; ++i) {
        size_t pos = (historyWritePos.load(std::memory_order_relaxed) - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
        const auto& analysis = analysisHistory[pos];
        
        if (analysis.isValid) {
            sum += analysis.loudness;
            count++;
        }
    }
    
    if (count < 3) return false;
    
    float avg = sum / count;
    float diff = std::abs(current.loudness - avg);
    
    return diff > (3.0f * sensitivity.load(std::memory_order_relaxed));
}

bool RealTimeSuggestionEngine::detectDynamicsIssue(const AudioAnalysisData& current) {
    float sens = sensitivity.load(std::memory_order_relaxed);
    return current.dynamics < (4.0f * sens) || 
           current.dynamics > (15.0f / sens);
}

bool RealTimeSuggestionEngine::detectStereoIssue(const AudioAnalysisData& current) {
    float sens = sensitivity.load(std::memory_order_relaxed);
    return current.stereoWidth < (0.3f * sens) || 
           current.stereoWidth > (2.0f / sens);
}

void RealTimeSuggestionEngine::addSuggestion(const Suggestion& suggestion) {
    // RT-SAFE: Find inactive slot
    for (auto& slot : suggestions) {
        if (!slot.isActive) {
            slot = suggestion;
            suggestionCount.fetch_add(1, std::memory_order_acq_rel);
            return;
        }
    }
    
    // Replace oldest suggestion if at capacity
    if (suggestionCount.load(std::memory_order_acquire) >= static_cast<size_t>(maxSuggestions.load(std::memory_order_acquire))) {
        uint64_t oldestTime = UINT64_MAX;
        int oldestIndex = 0;
        
        for (int i = 0; i < MAX_SUGGESTIONS; ++i) {
            if (suggestions[i].isActive && suggestions[i].timestamp < oldestTime) {
                oldestTime = suggestions[i].timestamp;
                oldestIndex = i;
            }
        }
        
        suggestions[oldestIndex] = suggestion;
    }
}

void RealTimeSuggestionEngine::removeOldSuggestions() {
    // RT-SAFE: Use our atomic timestamp counter instead of system time
    // Suggestions older than MAX_AGE counter increments are removed
    static std::atomic<uint64_t> rtCurrentTime{0};
    uint64_t currentTime = rtCurrentTime.fetch_add(1, std::memory_order_relaxed);
    const uint64_t MAX_AGE = 30000;  // Arbitrary counter threshold
    
    for (auto& suggestion : suggestions) {
        if (suggestion.isActive && (currentTime - suggestion.timestamp) > MAX_AGE) {
            suggestion.isActive = false;
            suggestionCount.fetch_sub(1, std::memory_order_relaxed);
        }
    }
}

std::vector<RealTimeSuggestionEngine::Suggestion> RealTimeSuggestionEngine::getActiveSuggestions() const {
    std::vector<Suggestion> active;
    
    for (const auto& suggestion : suggestions) {
        if (suggestion.isActive) {
            active.push_back(suggestion);
        }
    }
    
    return active;
}

void RealTimeSuggestionEngine::dismissSuggestion(const char* id) {
    for (auto& suggestion : suggestions) {
        if (suggestion.isActive && std::strcmp(suggestion.id, id) == 0) {
            suggestion.isActive = false;
            suggestionCount.fetch_sub(1, std::memory_order_acq_rel);
            break;
        }
    }
}

void RealTimeSuggestionEngine::setSensitivity(float sensitivity) {
    this->sensitivity.store(juce::jlimit(0.1f, 2.0f, sensitivity));
}

void RealTimeSuggestionEngine::setMaxSuggestions(int maxSuggestions) {
    this->maxSuggestions.store(juce::jlimit(1, static_cast<int>(MAX_SUGGESTIONS), maxSuggestions));
}

// RealTimeAudioProcessor Implementation
RealTimeAudioProcessor::RealTimeAudioProcessor() {
    audioProcessor = std::make_unique<AudioThreadSafeProcessor>();
    suggestionEngine = std::make_unique<RealTimeSuggestionEngine>();
}

RealTimeAudioProcessor::~RealTimeAudioProcessor() = default;

void RealTimeAudioProcessor::processAudio(const juce::AudioBuffer<float>& buffer,
                                         double sampleRate) {
    // Process audio through thread-safe processor
    audioProcessor->processAudioBlock(buffer, sampleRate, buffer.getNumSamples());
    
    // Get latest analysis and feed to suggestion engine
    if (audioProcessor->hasNewAnalysis()) {
        auto analysis = audioProcessor->getLatestAnalysis();
        suggestionEngine->processAudio(analysis);
        audioProcessor->markAnalysisRead();
    }
}

std::vector<RealTimeSuggestionEngine::Suggestion> RealTimeAudioProcessor::getSuggestions() const {
    return suggestionEngine->getActiveSuggestions();
}

AudioAnalysisData RealTimeAudioProcessor::getAnalysis() const {
    return audioProcessor->getLatestAnalysis();
}

void RealTimeAudioProcessor::setSensitivity(float sensitivity) {
    suggestionEngine->setSensitivity(sensitivity);
}

void RealTimeAudioProcessor::setAnalysisRate(float Hz) {
    audioProcessor->setAnalysisRate(Hz);
}

} // namespace ai
} // namespace zenith
