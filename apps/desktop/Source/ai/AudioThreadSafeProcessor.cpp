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

void AudioThreadSafeProcessor::processAudioBlock(const juce::AudioBuffer<float>& buffer,
                                                double sampleRate,
                                                int numSamples) {
    // Copy buffer to lock-free queue for background analysis
    juce::AudioBuffer<float> bufferCopy(buffer.getNumChannels(), buffer.getNumSamples());
    bufferCopy.makeCopyOf(buffer);
    
    if (!audioBuffer.push(bufferCopy)) {
        // Buffer full, drop oldest
        juce::AudioBuffer<float> dropped;
        audioBuffer.pop(dropped);
        audioBuffer.push(bufferCopy);
    }
    
    // Perform real-time analysis at specified rate
    samplesSinceAnalysis.fetch_add(numSamples);
    
    if (samplesSinceAnalysis.load() >= samplesPerAnalysis.load()) {
        performAnalysis(buffer);
        samplesSinceAnalysis.store(0);
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
    
    // Timestamp
    analysis.timestamp = juce::Time::getCurrentTime().toMilliseconds();
    analysis.isValid = true;
    
    // Push to analysis buffer
    if (!analysisBuffer.push(analysis)) {
        // Buffer full, remove oldest
        AudioAnalysisData dropped;
        analysisBuffer.pop(dropped);
        analysisBuffer.push(analysis);
    }
    
    lastAnalysis = analysis;
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
    
    // We can't pop in a const context without making the buffer mutable.
    // However, for real-time safety and convenience, we should provide the latest
    // known data. A better approach would be an atomic snapshot, but for now
    // we'll at least return a valid structure if available (hacked via const_cast 
    // or by returning an empty one if we don't want to change the buffer state).
    // Given the previous empty return, let's at least document the limitation.
    return lastAnalysis;
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
    
    // Add to history
    size_t writePos = historyWritePos.fetch_add(1) % HISTORY_SIZE;
    analysisHistory[writePos] = analysis;
    
    // Generate suggestions
    generateSuggestions(analysis);
    
    // Remove old suggestions
    removeOldSuggestions();
}

void RealTimeSuggestionEngine::generateSuggestions(const AudioAnalysisData& current) {
    // Check for various issues and generate suggestions
    
    // Loudness issues
    if (detectLoudnessTrend(current)) {
        Suggestion suggestion;
        suggestion.id = "loudness_" + juce::String(current.timestamp);
        suggestion.type = "loudness";
        suggestion.message = "Loudness is trending " + juce::String(current.loudness < -20.0f ? "low" : "high");
        suggestion.parameters = juce::var(current.loudness);
        suggestion.confidence = 0.8f;
        suggestion.timestamp = current.timestamp;
        suggestion.isActive = true;
        
        addSuggestion(suggestion);
    }
    
    // Dynamics issues
    if (detectDynamicsIssue(current)) {
        Suggestion suggestion;
        suggestion.id = "dynamics_" + juce::String(current.timestamp);
        suggestion.type = "dynamics";
        suggestion.message = "Dynamic range is " + juce::String(current.dynamics < 6.0f ? "low" : "high");
        suggestion.parameters = juce::var(current.dynamics);
        suggestion.confidence = 0.7f;
        suggestion.timestamp = current.timestamp;
        suggestion.isActive = true;
        
        addSuggestion(suggestion);
    }
    
    // Stereo issues
    if (detectStereoIssue(current)) {
        Suggestion suggestion;
        suggestion.id = "stereo_" + juce::String(current.timestamp);
        suggestion.type = "stereo";
        suggestion.message = "Stereo width is " + juce::String(current.stereoWidth < 0.5f ? "narrow" : "wide");
        suggestion.parameters = juce::var(current.stereoWidth);
        suggestion.confidence = 0.6f;
        suggestion.timestamp = current.timestamp;
        suggestion.isActive = true;
        
        addSuggestion(suggestion);
    }
}

bool RealTimeSuggestionEngine::detectLoudnessTrend(const AudioAnalysisData& current) {
    // Check last 10 analyses for trend
    int historyCount = std::min(10, static_cast<int>(historyWritePos.load()));
    if (historyCount < 3) return false;
    
    float sum = 0.0f;
    int count = 0;
    
    for (int i = 0; i < historyCount; ++i) {
        size_t pos = (historyWritePos.load() - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
        const auto& analysis = analysisHistory[pos];
        
        if (analysis.isValid) {
            sum += analysis.loudness;
            count++;
        }
    }
    
    if (count < 3) return false;
    
    float avg = sum / count;
    float diff = std::abs(current.loudness - avg);
    
    return diff > (3.0f * sensitivity.load());  // Threshold based on sensitivity
}

bool RealTimeSuggestionEngine::detectDynamicsIssue(const AudioAnalysisData& current) {
    return current.dynamics < (4.0f * sensitivity.load()) || 
           current.dynamics > (15.0f / sensitivity.load());
}

bool RealTimeSuggestionEngine::detectStereoIssue(const AudioAnalysisData& current) {
    return current.stereoWidth < (0.3f * sensitivity.load()) || 
           current.stereoWidth > (2.0f / sensitivity.load());
}

void RealTimeSuggestionEngine::addSuggestion(const Suggestion& suggestion) {
    // Find inactive slot
    for (auto& slot : suggestions) {
        if (!slot.isActive) {
            slot = suggestion;
            suggestionCount.fetch_add(1);
            return;
        }
    }
    
    // Replace oldest suggestion if at capacity
    if (suggestionCount.load() >= maxSuggestions.load()) {
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
    uint64_t currentTime = juce::Time::getCurrentTime().toMilliseconds();
    const uint64_t MAX_AGE = 30000;  // 30 seconds
    
    for (auto& suggestion : suggestions) {
        if (suggestion.isActive && (currentTime - suggestion.timestamp) > MAX_AGE) {
            suggestion.isActive = false;
            suggestionCount.fetch_sub(1);
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

void RealTimeSuggestionEngine::dismissSuggestion(const juce::String& id) {
    for (auto& suggestion : suggestions) {
        if (suggestion.isActive && suggestion.id == id) {
            suggestion.isActive = false;
            suggestionCount.fetch_sub(1);
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
