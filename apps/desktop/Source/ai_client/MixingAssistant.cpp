/*
  ==============================================================================
    MixingAssistant.cpp
    AI-powered mixing assistant implementation
  ==============================================================================
*/

#include "MixingAssistant.h"
#include "../ui/design-system/ZenithDesignSystem.h"
#include <algorithm>
#include <numeric>
#include <string>

namespace zenith {
namespace ai {

// MixingAssistant Implementation
MixingAssistant::MixingAssistant() {
    // Initialize with default config
    config = MixingAssistantFactory::getDefaultConfig();
    
    // Initialize FFT for analysis
    fft = juce::dsp::FFT(config.fftOrder);
    window = juce::dsp::WindowingFunction<float>(fft.getSize(), config.windowMethod);
    
    // Initialize analysis buffers
    fftBuffer.resize(fft.getSize() * 2);
    magnitudeBuffer.resize(fft.getSize() / 2 + 1);
    phaseBuffer.resize(fft.getSize() / 2 + 1);
    
    // Start timer for real-time analysis
    startTimer(config.updateInterval);
}

MixingAssistant::~MixingAssistant() {
    stopTimer();
}

void MixingAssistant::setConfig(const MixingAssistantConfig& newConfig) {
    config = newConfig;
    
    // Reinitialize FFT if size changed
    if (fft.getSize() != (1 << config.fftOrder)) {
        fft = juce::dsp::FFT(config.fftOrder);
        window = juce::dsp::WindowingFunction<float>(fft.getSize(), config.windowMethod);
        fftBuffer.resize(fft.getSize() * 2);
        magnitudeBuffer.resize(fft.getSize() / 2 + 1);
        phaseBuffer.resize(fft.getSize() / 2 + 1);
    }
    
    // Restart timer with new interval
    startTimer(config.updateInterval);
}

MixingAssistantConfig MixingAssistant::getConfig() const {
    return config;
}

void MixingAssistant::setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    std::lock_guard<std::mutex> lock(audioMutex);
    mainAudioBuffer = buffer;
    mainSampleRate = sampleRate;
    needsAnalysis.store(true);
}

void MixingAssistant::addTrackAudio(const juce::String& trackId, 
                                   const juce::AudioBuffer<float>& buffer, 
                                   double sampleRate) {
    std::lock_guard<std::mutex> lock(audioMutex);
    trackBuffers[trackId] = buffer;
    trackSampleRates[trackId] = sampleRate;
    needsAnalysis.store(true);
}

void MixingAssistant::removeTrack(const juce::String& trackId) {
    std::lock_guard<std::mutex> lock(audioMutex);
    trackBuffers.erase(trackId);
    trackSampleRates.erase(trackId);
    trackAnalyses.erase(trackId);
}

void MixingAssistant::analyzeMix() {
    if (isAnalyzing.load()) return;
    
    isAnalyzing.store(true);
    
    // Analyze main mix
    analyzeMainMix();
    
    // Analyze individual tracks
    analyzeAllTracks();
    
    // Detect issues
    detectMixIssues();
    
    // Generate suggestions
    if (config.enableAISuggestions) {
        generateSuggestions();
    }
    
    isAnalyzing.store(false);
    notifyAnalysisUpdated(currentAnalysis);
}

void MixingAssistant::analyzeTrack(const juce::String& trackId) {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    auto it = trackBuffers.find(trackId);
    if (it == trackBuffers.end()) return;
    
    const auto& buffer = it->second;
    double sampleRate = trackSampleRates[trackId];
    
    MixAnalysis::TrackAnalysis trackAnalysis;
    trackAnalysis.trackId = trackId;
    
    // Analyze track level
    trackAnalysis.loudness = calculateLUFS(buffer);
    trackAnalysis.peakLevel = calculatePeak(buffer);
    trackAnalysis.hasClipping = trackAnalysis.peakLevel > -0.1f;
    
    // Analyze frequency content
    analyzeTrackFrequency(trackId, trackAnalysis);
    
    // Analyze masking
    analyzeTrackMasking(trackId, trackAnalysis);
    
    // Determine track type
    trackAnalysis.trackType = detectTrackType(trackAnalysis);
    
    // Generate suggestion
    trackAnalysis.suggestedAction = generateTrackSuggestion(trackAnalysis);
    
    trackAnalyses[trackId] = trackAnalysis;
    currentAnalysis.trackAnalyses.clear();
    for (const auto& pair : trackAnalyses) {
        currentAnalysis.trackAnalyses.push_back(pair.second);
    }
}

MixAnalysis MixingAssistant::getCurrentAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return currentAnalysis;
}

MixAnalysis MixingAssistant::getTrackAnalysis(const juce::String& trackId) const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    auto it = trackAnalyses.find(trackId);
    if (it != trackAnalyses.end()) {
        return it->second;
    }
    return MixAnalysis::TrackAnalysis{};
}

void MixingAssistant::generateSuggestions() {
    std::lock_guard<std::mutex> lock(suggestionsMutex);
    suggestions.clear();
    
    // Generate volume suggestions
    generateVolumeSuggestions();
    
    // Generate EQ suggestions
    generateEQSuggestions();
    
    // Generate compression suggestions
    generateCompressionSuggestions();
    
    // Generate reverb suggestions
    generateReverbSuggestions();
    
    // Generate stereo suggestions
    generateStereoSuggestions();
    
    // Sort by priority
    std::sort(suggestions.begin(), suggestions.end(),
              [](const MixSuggestion& a, const MixSuggestion& b) {
                  return a.priority > b.priority;
              });
    
    // Limit suggestions per track
    std::unordered_map<juce::String, int> trackCounts;
    std::vector<MixSuggestion> filtered;
    
    for (const auto& suggestion : suggestions) {
        if (trackCounts[suggestion.targetTrack] < config.maxSuggestionsPerTrack) {
            filtered.push_back(suggestion);
            trackCounts[suggestion.targetTrack]++;
        }
    }
    
    suggestions = filtered;
}

std::vector<MixSuggestion> MixingAssistant::getSuggestions() const {
    std::lock_guard<std::mutex> lock(suggestionsMutex);
    return suggestions;
}

std::vector<MixSuggestion> MixingAssistant::getTrackSuggestions(const juce::String& trackId) const {
    std::lock_guard<std::mutex> lock(suggestionsMutex);
    std::vector<MixSuggestion> trackSuggestions;
    
    for (const auto& suggestion : suggestions) {
        if (suggestion.targetTrack == trackId) {
            trackSuggestions.push_back(suggestion);
        }
    }
    
    return trackSuggestions;
}

void MixingAssistant::applySuggestion(const juce::String& suggestionId) {
    std::lock_guard<std::mutex> lock(suggestionsMutex);
    
    for (auto& suggestion : suggestions) {
        if (suggestion.id == suggestionId) {
            suggestion.isApplied = true;
            
            // Apply the suggestion (implementation depends on DAW architecture)
            // This would interface with the audio engine to apply parameters
            
            notifySuggestionApplied(suggestion);
            break;
        }
    }
}

void MixingAssistant::rejectSuggestion(const juce::String& suggestionId) {
    std::lock_guard<std::mutex> lock(suggestionsMutex);
    
    for (auto& suggestion : suggestions) {
        if (suggestion.id == suggestionId) {
            // Learn from rejection if enabled
            if (config.learnFromUserActions && grokClient) {
                learnFromUserAction(suggestion.targetTrack, "reject_suggestion", suggestion.parameters);
            }
            
            notifySuggestionRejected(suggestion);
            break;
        }
    }
}

void MixingAssistant::clearSuggestions() {
    std::lock_guard<std::mutex> lock(suggestionsMutex);
    suggestions.clear();
}

void MixingAssistant::setGrokClient(std::shared_ptr<GrokAPIClient> client) {
    grokClient = client;
}

void MixingAssistant::requestAISuggestion(const juce::String& trackId, const juce::String& type) {
    if (!grokClient || !config.enableAISuggestions) return;
    
    juce::String prompt = "Generate mixing suggestion for track " + trackId + " of type " + type;
    
    grokClient->sendQuery(prompt, [this, trackId, type](const juce::String& response) {
        // Parse AI response and create suggestion
        MixSuggestion suggestion;
        suggestion.id = juce::Uuid().toString();
        suggestion.type = type;
        suggestion.targetTrack = trackId;
        suggestion.description = response;
        suggestion.confidence = 0.7f; // Default confidence
        suggestion.timestamp = juce::Time::getCurrentTime();
        
        std::lock_guard<std::mutex> lock(suggestionsMutex);
        suggestions.push_back(suggestion);
        notifySuggestionGenerated(suggestion);
    });
}

void MixingAssistant::learnFromUserAction(const juce::String& trackId, 
                                         const juce::String& action, 
                                         const juce::var& parameters) {
    if (!grokClient || !config.learnFromUserActions) return;
    
    juce::String prompt = "User performed action " + action + " on track " + trackId;
    prompt += " with parameters: " + juce::JSON::toString(parameters);
    prompt += ". Learn from this preference for future suggestions.";
    
    grokClient->sendQuery(prompt, [](const juce::String& response) {
        // Learning response - could update internal models
    });
}

void MixingAssistant::startAutoMix() {
    autoMixActive.store(true);
    notifyAutoMixStarted();
}

void MixingAssistant::stopAutoMix() {
    autoMixActive.store(false);
    notifyAutoMixStopped();
}

bool MixingAssistant::isAutoMixActive() const {
    return autoMixActive.load();
}

void MixingAssistant::setAutoMixStrength(float strength) {
    autoMixStrength = juce::jlimit(0.0f, 1.0f, strength);
}

juce::String MixingAssistant::detectGenre() {
    // Analyze audio characteristics to detect genre
    std::lock_guard<std::mutex> lock(audioMutex);
    
    if (mainAudioBuffer.getNumSamples() == 0) return "Unknown";
    
    // Simple genre detection based on spectral characteristics
    performFFT(mainAudioBuffer);
    
    float spectralCentroid = calculateFrequencyCenter(mainAudioBuffer);
    float spectralSpread = calculateSpectralSpread();
    
    // Basic genre classification
    if (spectralCentroid < 1000) {
        detectedGenre = "Electronic";
    } else if (spectralCentroid < 2000) {
        detectedGenre = "Rock";
    } else if (spectralCentroid < 3000) {
        detectedGenre = "Pop";
    } else {
        detectedGenre = "Classical";
    }
    
    notifyGenreDetected(detectedGenre);
    return detectedGenre;
}

void MixingAssistant::setTargetGenre(const juce::String& genre) {
    targetGenre = genre;
    applyGenreTargets();
}

std::vector<juce::String> MixingAssistant::getSupportedGenres() const {
    return {"Rock", "Pop", "Jazz", "Electronic", "Classical", "Hip Hop", "Country"};
}

void MixingAssistant::loadReferenceTrack(const juce::File& file) {
    if (!file.exists()) return;
    
    // Load reference track
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return;
    
    referenceBuffer.setSize(reader->numChannels, static_cast<int>(reader->lengthInSamples));
    reader->read(&referenceBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    referenceSampleRate = reader->sampleRate;
    hasReferenceTrack = true;
    
    // Analyze reference track
    analyzeReferenceTrack();
    
    notifyReferenceTrackLoaded();
}

void MixingAssistant::analyzeReferenceTrack() {
    if (!hasReferenceTrack) return;
    
    // Analyze loudness
    referenceAnalysis.overallLoudness = calculateLUFS(referenceBuffer);
    referenceAnalysis.dynamicRange = calculateDynamicRange(referenceBuffer);
    
    // Analyze frequency balance
    analyzeFrequencyBalance(referenceAnalysis, referenceBuffer);
    
    // Analyze stereo image
    analyzeStereoImage(referenceAnalysis, referenceBuffer);
    
    notifyReferenceTrackMatched();
}

void MixingAssistant::matchToReference() {
    if (!hasReferenceTrack) return;
    
    matchLoudnessToReference();
    matchFrequencyToReference();
    matchStereoToReference();
    
    notifyReferenceTrackMatched();
}

void MixingAssistant::clearReferenceTrack() {
    hasReferenceTrack = false;
    referenceBuffer.setSize(0, 0);
    referenceAnalysis = MixAnalysis{};
}

void MixingAssistant::timerCallback() {
    if (config.enableRealTimeAnalysis && needsAnalysis.load()) {
        analyzeMix();
        needsAnalysis.store(false);
    }
}

void MixingAssistant::handleAsyncUpdate() {
    // Handle async updates if needed
}

void MixingAssistant::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void MixingAssistant::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void MixingAssistant::analyzeLoudness(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer) {
    analysis.overallLoudness = calculateLUFS(buffer);
    analysis.momentaryLUFS = calculateMomentaryLUFS(buffer);
    analysis.shortTermLUFS = calculateShortTermLUFS(buffer);
    analysis.loudnessRange = calculateLRA(buffer);
    analysis.truePeak = calculateTruePeak(buffer);
    analysis.samplePeak = calculatePeak(buffer);
    analysis.crestFactor = analysis.truePeak - analysis.overallLoudness;
    analysis.dynamicRange = calculateDynamicRange(buffer);
    
    // Channel analysis
    analysis.channelLUFS.clear();
    analysis.channelPeaks.clear();
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        juce::AudioBuffer<float> channelBuffer(1, buffer.getNumSamples());
        channelBuffer.copyFrom(0, 0, buffer, ch, 0, buffer.getNumSamples());
        
        analysis.channelLUFS.push_back(calculateLUFS(channelBuffer));
        analysis.channelPeaks.push_back(calculatePeak(channelBuffer));
    }
}

void MixingAssistant::analyzeFrequencyBalance(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer) {
    performFFT(buffer);
    
    // Calculate frequency band levels
    float sampleRate = mainSampleRate;
    int fftSize = fft.getSize();
    float binWidth = sampleRate / fftSize;
    
    // Sub-bass (20-60 Hz)
    int subBassStart = static_cast<int>(20.0f / binWidth);
    int subBassEnd = static_cast<int>(60.0f / binWidth);
    analysis.subBassLevel = calculateBandLevel(magnitudeBuffer, subBassStart, subBassEnd);
    
    // Bass (60-250 Hz)
    int bassStart = static_cast<int>(60.0f / binWidth);
    int bassEnd = static_cast<int>(250.0f / binWidth);
    analysis.bassLevel = calculateBandLevel(magnitudeBuffer, bassStart, bassEnd);
    
    // Low-mid (250-500 Hz)
    int lowMidStart = static_cast<int>(250.0f / binWidth);
    int lowMidEnd = static_cast<int>(500.0f / binWidth);
    analysis.lowMidLevel = calculateBandLevel(magnitudeBuffer, lowMidStart, lowMidEnd);
    
    // Mid (500-2000 Hz)
    int midStart = static_cast<int>(500.0f / binWidth);
    int midEnd = static_cast<int>(2000.0f / binWidth);
    analysis.midLevel = calculateBandLevel(magnitudeBuffer, midStart, midEnd);
    
    // High-mid (2000-4000 Hz)
    int highMidStart = static_cast<int>(2000.0f / binWidth);
    int highMidEnd = static_cast<int>(4000.0f / binWidth);
    analysis.highMidLevel = calculateBandLevel(magnitudeBuffer, highMidStart, highMidEnd);
    
    // Presence (4000-6000 Hz)
    int presenceStart = static_cast<int>(4000.0f / binWidth);
    int presenceEnd = static_cast<int>(6000.0f / binWidth);
    analysis.presenceLevel = calculateBandLevel(magnitudeBuffer, presenceStart, presenceEnd);
    
    // Brilliance (6000-20000 Hz)
    int brillianceStart = static_cast<int>(6000.0f / binWidth);
    int brillianceEnd = static_cast<int>(20000.0f / binWidth);
    analysis.brillianceLevel = calculateBandLevel(magnitudeBuffer, brillianceStart, brillianceEnd);
    
    // Calculate spectral features
    calculateSpectralFeatures(analysis);
}

void MixingAssistant::analyzeStereoImage(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) {
        analysis.stereoWidth = 0.0f;
        analysis.phaseCorrelation = 1.0f;
        return;
    }
    
    // Calculate stereo width
    analysis.stereoWidth = calculateStereoWidth(buffer);
    
    // Calculate phase correlation
    analysis.phaseCorrelation = calculatePhaseCorrelation(buffer);
    
    // Calculate mid-side ratio
    float midLevel = 0.0f;
    float sideLevel = 0.0f;
    
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float mid = (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f;
        float side = (buffer.getSample(0, i) - buffer.getSample(1, i)) * 0.5f;
        
        midLevel += mid * mid;
        sideLevel += side * side;
    }
    
    midLevel = std::sqrt(midLevel / buffer.getNumSamples());
    sideLevel = std::sqrt(sideLevel / buffer.getNumSamples());
    
    analysis.midSideRatio = 20.0f * std::log10(sideLevel / midLevel);
}

void MixingAssistant::analyzeDynamics(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer) {
    // Calculate RMS history
    int windowSize = 1024;
    int numWindows = buffer.getNumSamples() / windowSize;
    
    analysis.rmsHistory.clear();
    float totalRMS = 0.0f;
    
    for (int w = 0; w < numWindows; ++w) {
        float rms = 0.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < windowSize; ++i) {
                float sample = buffer.getSample(ch, w * windowSize + i);
                rms += sample * sample;
            }
        }
        
        rms = std::sqrt(rms / (windowSize * buffer.getNumChannels()));
        rms = 20.0f * std::log10(rms);
        
        analysis.rmsHistory.push_back(rms);
        totalRMS += rms;
    }
    
    analysis.averageRMS = totalRMS / numWindows;
    
    // Calculate RMS variation
    float variance = 0.0f;
    for (float rms : analysis.rmsHistory) {
        variance += (rms - analysis.averageRMS) * (rms - analysis.averageRMS);
    }
    analysis.rmsVariation = std::sqrt(variance / numWindows);
    
    // Calculate peak-to-RMS ratio
    float peak = calculatePeak(buffer);
    analysis.peakToRMSRatio = peak - analysis.averageRMS;
}

void MixingAssistant::analyzePitch(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer) {
    // Detect fundamental frequency
    analysis.fundamentalFrequency = detectFundamental(magnitudeBuffer);
    analysis.fundamentalConfidence = calculatePitchConfidence(buffer);
    
    // Analyze harmony
    analyzeHarmony(analysis);
    
    // Detect key
    detectKey(analysis);
}

void MixingAssistant::analyzeRhythm(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer) {
    // Detect tempo
    analysis.tempo = detectTempo();
    
    // Detect time signature
    detectTimeSignature(analysis);
    
    // Track beats
    trackBeats(analysis);
    
    // Detect onsets
    detectOnsets(analysis);
}

void MixingAssistant::analyzeTimbre(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer) {
    // Calculate MFCC coefficients
    calculateMFCC(analysis);
    
    // Calculate timbre descriptors
    calculateTimbreDescriptors(analysis);
    
    // Classify instrument
    classifyInstrument(analysis);
}

void MixingAssistant::analyzeQuality(MixAnalysis& analysis) {
    // Assess clarity
    assessClarity(analysis);
    
    // Assess punch
    assessPunch(analysis);
    
    // Assess warmth
    assessWarmth(analysis);
    
    // Detect issues
    detectIssues(analysis);
    
    // Calculate overall score
    calculateOverallScore(analysis);
    
    // Generate recommendations
    generateRecommendations(analysis);
}

void MixingAssistant::performFFT(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumSamples() < fft.getSize()) return;
    
    // Copy samples to FFT buffer
    int numSamples = juce::jmin(buffer.getNumSamples(), fft.getSize());
    
    for (int i = 0; i < numSamples; ++i) {
        float sample = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            sample += buffer.getSample(ch, i);
        }
        sample /= buffer.getNumChannels();
        
        fftBuffer[i] = std::complex<float>(sample, 0.0f);
    }
    
    // Zero pad if necessary
    for (int i = numSamples; i < fft.getSize(); ++i) {
        fftBuffer[i] = std::complex<float>(0.0f, 0.0f);
    }
    
    // Apply window
    std::vector<float> windowedSamples(fft.getSize());
    for (int i = 0; i < fft.getSize(); ++i) {
        windowedSamples[i] = fftBuffer[i].real();
    }
    window.multiplyWithWindowingTable(windowedSamples.data(), fft.getSize());
    for (int i = 0; i < fft.getSize(); ++i) {
        fftBuffer[i] = std::complex<float>(windowedSamples[i], 0.0f);
    }
    
    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(reinterpret_cast<float*>(fftBuffer.data()));
    
    // Convert to magnitude and phase
    for (int i = 0; i < fft.getSize() / 2 + 1; ++i) {
        magnitudeBuffer[i] = std::abs(fftBuffer[i]);
        phaseBuffer[i] = std::arg(fftBuffer[i]);
    }
}

float MixingAssistant::calculateLUFS(const juce::AudioBuffer<float>& buffer) {
    // Simplified LUFS calculation
    float sum = 0.0f;
    int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = buffer.getSample(ch, i);
            sum += sample * sample;
        }
    }
    
    float rms = std::sqrt(sum / (numSamples * buffer.getNumChannels()));
    return -0.691f + 10.0f * std::log10(rms);
}

float MixingAssistant::calculateTruePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(buffer.getSample(ch, i));
            peak = juce::jmax(peak, sample);
        }
    }
    
    return 20.0f * std::log10(peak);
}

float MixingAssistant::calculateLRA(const juce::AudioBuffer<float>& buffer) {
    // Simplified loudness range calculation
    std::vector<float> shortTermLoudness;
    
    int windowSize = static_cast<int>(3.0f * mainSampleRate); // 3 second windows
    int hopSize = windowSize / 2;
    
    for (int start = 0; start < buffer.getNumSamples() - windowSize; start += hopSize) {
        juce::AudioBuffer<float> window(buffer.getNumChannels(), windowSize);
        window.copyFrom(0, 0, buffer, 0, start, windowSize);
        
        shortTermLoudness.push_back(calculateLUFS(window));
    }
    
    if (shortTermLoudness.empty()) return 0.0f;
    
    // Calculate loudness range
    std::sort(shortTermLoudness.begin(), shortTermLoudness.end());
    
    int lowIndex = static_cast<int>(shortTermLoudness.size() * 0.1);
    int highIndex = static_cast<int>(shortTermLoudness.size() * 0.95);
    
    return shortTermLoudness[highIndex] - shortTermLoudness[lowIndex];
}

float MixingAssistant::calculateStereoWidth(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return 0.0f;
    
    float correlation = 0.0f;
    float leftPower = 0.0f;
    float rightPower = 0.0f;
    
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float left = buffer.getSample(0, i);
        float right = buffer.getSample(1, i);
        
        correlation += left * right;
        leftPower += left * left;
        rightPower += right * right;
    }
    
    if (leftPower == 0.0f || rightPower == 0.0f) return 0.0f;
    
    correlation /= std::sqrt(leftPower * rightPower);
    
    // Convert correlation to width (0 = mono, 1 = maximum width)
    return std::sqrt(1.0f - correlation);
}

float MixingAssistant::calculatePhaseCorrelation(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return 1.0f;
    
    float correlation = 0.0f;
    float leftPower = 0.0f;
    float rightPower = 0.0f;
    
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float left = buffer.getSample(0, i);
        float right = buffer.getSample(1, i);
        
        correlation += left * right;
        leftPower += left * left;
        rightPower += right * right;
    }
    
    if (leftPower == 0.0f || rightPower == 0.0f) return 1.0f;
    
    return correlation / std::sqrt(leftPower * rightPower);
}

float MixingAssistant::calculateRMS(const juce::AudioBuffer<float>& buffer) {
    float sum = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = buffer.getSample(ch, i);
            sum += sample * sample;
        }
    }
    
    return std::sqrt(sum / (buffer.getNumSamples() * buffer.getNumChannels()));
}

float MixingAssistant::calculatePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(buffer.getSample(ch, i));
            peak = juce::jmax(peak, sample);
        }
    }
    
    return 20.0f * std::log10(peak);
}

float MixingAssistant::calculateFrequencyCenter(const juce::AudioBuffer<float>& buffer) {
    performFFT(buffer);
    
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    float binWidth = mainSampleRate / fft.getSize();
    
    for (int i = 1; i < magnitudeBuffer.size(); ++i) {
        float frequency = i * binWidth;
        float magnitude = magnitudeBuffer[i];
        
        weightedSum += frequency * magnitude;
        magnitudeSum += magnitude;
    }
    
    return magnitudeSum > 0.0f ? weightedSum / magnitudeSum : 0.0f;
}

float MixingAssistant::calculateSpectralSpread() {
    float center = calculateFrequencyCenter(mainAudioBuffer);
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    float binWidth = mainSampleRate / fft.getSize();
    
    for (int i = 1; i < magnitudeBuffer.size(); ++i) {
        float frequency = i * binWidth;
        float magnitude = magnitudeBuffer[i];
        float deviation = frequency - center;
        
        weightedSum += deviation * deviation * magnitude;
        magnitudeSum += magnitude;
    }
    
    return magnitudeSum > 0.0f ? std::sqrt(weightedSum / magnitudeSum) : 0.0f;
}

float MixingAssistant::calculateMaskingIndex(const juce::AudioBuffer<float>& track1, 
                                            const juce::AudioBuffer<float>& track2) {
    // Simplified masking calculation
    float track1RMS = calculateRMS(track1);
    float track2RMS = calculateRMS(track2);
    
    return track1RMS / (track1RMS + track2RMS + 1e-6f);
}

void MixingAssistant::analyzeMainMix() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    if (mainAudioBuffer.getNumSamples() == 0) return;
    
    // Analyze loudness
    analyzeLoudness(currentAnalysis, mainAudioBuffer);
    
    // Analyze frequency balance
    analyzeFrequencyBalance(currentAnalysis, mainAudioBuffer);
    
    // Analyze stereo image
    analyzeStereoImage(currentAnalysis, mainAudioBuffer);
    
    // Analyze dynamics
    analyzeDynamics(currentAnalysis, mainAudioBuffer);
    
    // Analyze pitch
    analyzePitch(currentAnalysis, mainAudioBuffer);
    
    // Analyze rhythm
    analyzeRhythm(currentAnalysis, mainAudioBuffer);
    
    // Analyze timbre
    analyzeTimbre(currentAnalysis, mainAudioBuffer);
    
    // Analyze quality
    analyzeQuality(currentAnalysis);
}

void MixingAssistant::analyzeAllTracks() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    for (const auto& pair : trackBuffers) {
        analyzeTrack(pair.first);
    }
}

void MixingAssistant::detectMixIssues() {
    currentAnalysis.issues.clear();
    
    // Check for clipping
    if (currentAnalysis.truePeak > -0.1f) {
        MixAnalysis::MixIssue issue;
        issue.description = "Mix is clipping - peak level exceeds -0.1 dBFS";
        issue.severity = "critical";
        issue.category = "clipping";
        issue.confidence = 1.0f;
        currentAnalysis.issues.push_back(issue);
    }
    
    // Check for low loudness
    if (currentAnalysis.overallLoudness < -16.0f) {
        MixAnalysis::MixIssue issue;
        issue.description = "Mix is too quiet - loudness below -16 LUFS";
        issue.severity = "high";
        issue.category = "loudness";
        issue.confidence = 0.8f;
        currentAnalysis.issues.push_back(issue);
    }
    
    // Check for excessive bass
    if (currentAnalysis.bassLevel > currentAnalysis.midLevel + 6.0f) {
        MixAnalysis::MixIssue issue;
        issue.description = "Excessive bass - bass level significantly higher than midrange";
        issue.severity = "medium";
        issue.category = "frequency";
        issue.confidence = 0.7f;
        currentAnalysis.issues.push_back(issue);
    }
    
    // Check for narrow stereo image
    if (currentAnalysis.stereoWidth < 0.3f) {
        MixAnalysis::MixIssue issue;
        issue.description = "Narrow stereo image - width below 30%";
        issue.severity = "medium";
        issue.category = "stereo";
        issue.confidence = 0.6f;
        currentAnalysis.issues.push_back(issue);
    }
    
    // Check for phase issues
    if (currentAnalysis.phaseCorrelation < 0.5f) {
        MixAnalysis::MixIssue issue;
        issue.description = "Phase correlation issues - potential mono compatibility problems";
        issue.severity = "high";
        issue.category = "phase";
        issue.confidence = 0.8f;
        currentAnalysis.issues.push_back(issue);
    }
}

void MixingAssistant::generateVolumeSuggestions() {
    for (const auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        // Check if track is too loud
        if (trackAnalysis.loudness > -12.0f) {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "volume";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Reduce track volume by " + 
                                    juce::String(trackAnalysis.loudness + 12.0f, 1) + " dB";
            suggestion.confidence = 0.8f;
            suggestion.priority = 0.7f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("parameter", "volume");
            params->setProperty("value", -12.0f);
            suggestion.parameters = params;
            
            suggestions.push_back(suggestion);
        }
        
        // Check if track is too quiet
        if (trackAnalysis.loudness < -24.0f) {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "volume";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Increase track volume by " + 
                                    juce::String(-24.0f - trackAnalysis.loudness, 1) + " dB";
            suggestion.confidence = 0.7f;
            suggestion.priority = 0.6f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("parameter", "volume");
            params->setProperty("value", -24.0f);
            suggestion.parameters = params;
            
            suggestions.push_back(suggestion);
        }
    }
}

void MixingAssistant::generateEQSuggestions() {
    for (const auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        // Check for muddy low-mids
        if (trackAnalysis.frequencyCenter < 500 && trackAnalysis.frequencyCenter > 250) {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "eq";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Cut low-mids around 300-400 Hz to reduce mud";
            suggestion.confidence = 0.6f;
            suggestion.priority = 0.5f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("frequency", 350.0f);
            params->setProperty("gain", -3.0f);
            params->setProperty("q", 1.0f);
            suggestion.parameters = params;
            
            suggestions.push_back(suggestion);
        }
        
        // Check for harsh highs
        if (trackAnalysis.frequencyCenter > 4000) {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "eq";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Cut highs around 4-6 kHz to reduce harshness";
            suggestion.confidence = 0.5f;
            suggestion.priority = 0.4f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("frequency", 5000.0f);
            params->setProperty("gain", -2.0f);
            params->setProperty("q", 1.5f);
            suggestion.parameters = params;
            
            suggestions.push_back(suggestion);
        }
    }
}

void MixingAssistant::generateCompressionSuggestions() {
    for (const auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        // Check for dynamic tracks that need compression
        if (trackAnalysis.trackType == "vocals" || trackAnalysis.trackType == "bass") {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "compression";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Add compression to control dynamics";
            suggestion.confidence = 0.7f;
            suggestion.priority = 0.6f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("ratio", 4.0f);
            params->setProperty("threshold", -12.0f);
            params->setProperty("attack", 5.0f);
            params->setProperty("release", 50.0f);
            suggestion.parameters = params;
            
            suggestions.push_back(suggestion);
        }
    }
}

void MixingAssistant::generateReverbSuggestions() {
    for (const auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        // Suggest reverb for dry tracks
        if (trackAnalysis.trackType == "vocals" || trackAnalysis.trackType == "guitar") {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "reverb";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Add reverb for spatial depth";
            suggestion.confidence = 0.5f;
            suggestion.priority = 0.3f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("roomSize", 0.3f);
            params->setProperty("damping", 0.5f);
            params->setProperty("wetLevel", -12.0f);
            params->setProperty("dryLevel", -6.0f);
            suggestion.parameters = params;
            
            suggestions.push_back(suggestion);
        }
    }
}

void MixingAssistant::generateStereoSuggestions() {
    // Check overall stereo width
    if (currentAnalysis.stereoWidth < 0.5f) {
        MixSuggestion suggestion;
        suggestion.id = juce::Uuid().toString();
        suggestion.type = "stereo";
        suggestion.targetTrack = "mix";
        suggestion.description = "Increase stereo width for better spatial image";
        suggestion.confidence = 0.6f;
        suggestion.priority = 0.4f;
        
        juce::DynamicObject::Ptr params = new juce::DynamicObject();
        params->setProperty("width", 1.2f);
        suggestion.parameters = params;
        
        suggestions.push_back(suggestion);
    }
}

void MixingAssistant::generateClippingSuggestions() {
    for (const auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        if (trackAnalysis.hasClipping) {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "volume";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Reduce volume to prevent clipping";
            suggestion.confidence = 0.9f;
            suggestion.priority = 0.9f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("parameter", "volume");
            params->setProperty("value", trackAnalysis.peakLevel - 1.0f);
            suggestion.parameters = params;
            
            suggestions.push_back(suggestion);
        }
    }
}

void MixingAssistant::requestVolumeAISuggestion(const juce::String& trackId) {
    requestAISuggestion(trackId, "volume");
}

void MixingAssistant::requestEQAISuggestion(const juce::String& trackId) {
    requestAISuggestion(trackId, "eq");
}

void MixingAssistant::requestCompressionAISuggestion(const juce::String& trackId) {
    requestAISuggestion(trackId, "compression");
}

void Juce::MixingAssistant::requestReverbAISuggestion(const juce::String& trackId) {
    requestAISuggestion(trackId, "reverb");
}

void MixingAssistant::performAutoMix() {
    if (!autoMixActive.load()) return;
    
    // Apply suggested adjustments based on strength
    for (const auto& suggestion : suggestions) {
        if (suggestion.confidence * autoMixStrength > 0.5f) {
            applySuggestion(suggestion.id);
        }
    }
}

void MixingAssistant::autoMixLevels() {
    // Balance track levels
    float targetLoudness = -18.0f; // Target LUFS for individual tracks
    
    for (auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        if (trackAnalysis.loudness < targetLoudness - 3.0f) {
            // Track is too quiet, increase volume
            float adjustment = (targetLoudness - trackAnalysis.loudness) * autoMixStrength;
            
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "volume";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Auto-adjust volume by " + juce::String(adjustment, 1) + " dB";
            suggestion.confidence = 0.8f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("parameter", "volume");
            params->setProperty("value", trackAnalysis.loudness + adjustment);
            suggestion.parameters = params;
            
            applySuggestion(suggestion.id);
        }
    }
}

void MixingAssistant::autoMixEQ() {
    // Apply basic EQ adjustments
    for (auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        // Reduce mud in low-mids
        if (trackAnalysis.frequencyCenter < 400 && trackAnalysis.frequencyCenter > 200) {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "eq";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Auto-EQ: Cut mud at 300 Hz";
            suggestion.confidence = 0.6f;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("frequency", 300.0f);
            params->setProperty("gain", -2.0f * autoMixStrength);
            params->setProperty("q", 1.0f);
            suggestion.parameters = params;
            
            applySuggestion(suggestion.id);
        }
    }
}

void MixingAssistant::autoMixCompression() {
    // Add compression to tracks that need it
    for (auto& trackAnalysis : currentAnalysis.trackAnalyses) {
        if (trackAnalysis.trackType == "vocals" || trackAnalysis.trackType == "bass") {
            MixSuggestion suggestion;
            suggestion.id = juce::Uuid().toString();
            suggestion.type = "compression";
            suggestion.targetTrack = trackAnalysis.trackId;
            suggestion.description = "Auto-compression: Light compression";
            suggestion.confidence = 0.7f;
            
            float ratio = 2.0f + (4.0f - 2.0f) * autoMixStrength;
            
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            params->setProperty("ratio", ratio);
            params->setProperty("threshold", -15.0f);
            params->setProperty("attack", 10.0f);
            params->setProperty("release", 100.0f);
            suggestion.parameters = params;
            
            applySuggestion(suggestion.id);
        }
    }
}

void MixingAssistant::autoMixStereo() {
    // Adjust stereo width if needed
    if (currentAnalysis.stereoWidth < 0.5f) {
        MixSuggestion suggestion;
        suggestion.id = juce::Uuid().toString();
        suggestion.type = "stereo";
        suggestion.targetTrack = "mix";
        suggestion.description = "Auto-stereo: Increase width";
        suggestion.confidence = 0.6f;
        
        float width = 1.0f + (1.5f - 1.0f) * autoMixStrength;
        
        juce::DynamicObject::Ptr params = new juce::DynamicObject();
        params->setProperty("width", width);
        suggestion.parameters = params;
        
        applySuggestion(suggestion.id);
    }
}

std::unordered_map<juce::String, float> MixingAssistant::getGenreTargets(const juce::String& genre) {
    if (genre == "Rock") {
        return MixingAssistantFactory::getRockTargets();
    } else if (genre == "Pop") {
        return MixingAssistantFactory::getPopTargets();
    } else if (genre == "Jazz") {
        return MixingAssistantFactory::getJazzTargets();
    } else if (genre == "Electronic") {
        return MixingAssistantFactory::getElectronicTargets();
    } else if (genre == "Classical") {
        return MixingAssistantFactory::getOrchestralTargets();
    }
    
    return {};
}

void MixingAssistant::applyGenreTargets() {
    if (targetGenre.isEmpty()) return;
    
    auto targets = getGenreTargets(targetGenre);
    
    // Apply genre-specific adjustments
    for (const auto& target : targets) {
        juce::String parameter = target.first;
        float value = target.second;
        
        // Apply to current mix
        // Implementation depends on DAW architecture
    }
}

void MixingAssistant::matchLoudnessToReference() {
    if (!hasReferenceTrack) return;
    
    float targetLoudness = referenceAnalysis.overallLoudness;
    float currentLoudness = currentAnalysis.overallLoudness;
    float adjustment = targetLoudness - currentLoudness;
    
    // Apply adjustment to master volume
    MixSuggestion suggestion;
    suggestion.id = juce::Uuid().toString();
    suggestion.type = "volume";
    suggestion.targetTrack = "master";
    suggestion.description = "Match loudness to reference: " + juce::String(adjustment, 1) + " dB";
    suggestion.confidence = 0.9f;
    
    juce::DynamicObject::Ptr params = new juce::DynamicObject();
    params->setProperty("parameter", "volume");
    params->setProperty("value", currentLoudness + adjustment);
    suggestion.parameters = params;
    
    applySuggestion(suggestion.id);
}

void MixingAssistant::matchFrequencyToReference() {
    if (!hasReferenceTrack) return;
    
    // Compare frequency balance
    float bassDiff = referenceAnalysis.bassLevel - currentAnalysis.bassLevel;
    float midDiff = referenceAnalysis.midLevel - currentAnalysis.midLevel;
    float highDiff = referenceAnalysis.brillianceLevel - currentAnalysis.brillianceLevel;
    
    // Apply EQ adjustments
    if (std::abs(bassDiff) > 3.0f) {
        MixSuggestion suggestion;
        suggestion.id = juce::Uuid().toString();
        suggestion.type = "eq";
        suggestion.targetTrack = "master";
        suggestion.description = "Match bass to reference: " + juce::String(bassDiff, 1) + " dB";
        suggestion.confidence = 0.7f;
        
        juce::DynamicObject::Ptr params = new juce::DynamicObject();
        params->setProperty("frequency", 100.0f);
        params->setProperty("gain", bassDiff);
        params->setProperty("q", 0.7f);
        suggestion.parameters = params;
        
        applySuggestion(suggestion.id);
    }
}

void MixingAssistant::matchStereoToReference() {
    if (!hasReferenceTrack) return;
    
    float targetWidth = referenceAnalysis.stereoWidth;
    float currentWidth = currentAnalysis.stereoWidth;
    float adjustment = (targetWidth - currentWidth) * 0.5f;
    
    if (std::abs(adjustment) > 0.1f) {
        MixSuggestion suggestion;
        suggestion.id = juce::Uuid().toString();
        suggestion.type = "stereo";
        suggestion.targetTrack = "master";
        suggestion.description = "Match stereo width to reference";
        suggestion.confidence = 0.6f;
        
        juce::DynamicObject::Ptr params = new juce::DynamicObject();
        params->setProperty("width", currentWidth + adjustment);
        suggestion.parameters = params;
        
        applySuggestion(suggestion.id);
    }
}

// Helper methods
float MixingAssistant::calculateBandLevel(const std::vector<float>& spectrum, 
                                          int startBin, int endBin) {
    if (startBin >= spectrum.size() || endBin >= spectrum.size()) return 0.0f;
    
    float sum = 0.0f;
    for (int i = startBin; i <= endBin; ++i) {
        sum += spectrum[i] * spectrum[i];
    }
    
    return 10.0f * std::log10(sum / (endBin - startBin + 1) + 1e-10f);
}

void MixingAssistant::calculateSpectralFeatures(MixAnalysis& analysis) {
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    float binWidth = mainSampleRate / fft.getSize();
    
    for (int i = 1; i < magnitudeBuffer.size(); ++i) {
        float frequency = i * binWidth;
        float magnitude = magnitudeBuffer[i];
        
        weightedSum += frequency * magnitude;
        magnitudeSum += magnitude;
    }
    
    analysis.spectralCentroid = magnitudeSum > 0.0f ? weightedSum / magnitudeSum : 0.0f;
    
    // Calculate other spectral features
    // spectralSpread, spectralSkewness, etc.
}

void MixingAssistant::analyzeTrackFrequency(const juce::String& trackId, 
                                           MixAnalysis::TrackAnalysis& trackAnalysis) {
    auto it = trackBuffers.find(trackId);
    if (it == trackBuffers.end()) return;
    
    const auto& buffer = it->second;
    performFFT(buffer);
    
    trackAnalysis.frequencyCenter = calculateFrequencyCenter(buffer);
}

void MixingAssistant::analyzeTrackMasking(const juce::String& trackId, 
                                         MixAnalysis::TrackAnalysis& trackAnalysis) {
    float totalMasking = 0.0f;
    int count = 0;
    
    auto it = trackBuffers.find(trackId);
    if (it == trackBuffers.end()) return;
    
    const auto& buffer = it->second;
    
    for (const auto& pair : trackBuffers) {
        if (pair.first != trackId) {
            float masking = calculateMaskingIndex(buffer, pair.second);
            totalMasking += masking;
            count++;
        }
    }
    
    trackAnalysis.maskingIndex = count > 0 ? totalMasking / count : 0.0f;
}

juce::String MixingAssistant::detectTrackType(const MixAnalysis::TrackAnalysis& trackAnalysis) {
    // Simple track type detection based on frequency content
    if (trackAnalysis.frequencyCenter < 200) {
        return "kick";
    } else if (trackAnalysis.frequencyCenter < 500) {
        return "bass";
    } else if (trackAnalysis.frequencyCenter < 2000) {
        return "vocal";
    } else if (trackAnalysis.frequencyCenter < 4000) {
        return "guitar";
    } else {
        return "hihat";
    }
}

juce::String MixingAssistant::generateTrackSuggestion(const MixAnalysis::TrackAnalysis& trackAnalysis) {
    if (trackAnalysis.hasClipping) {
        return "Reduce volume to prevent clipping";
    }
    
    if (trackAnalysis.loudness < -24.0f) {
        return "Increase volume to improve presence";
    }
    
    if (trackAnalysis.maskingIndex > 0.7f) {
        return "Consider EQ to reduce masking";
    }
    
    return "Track sounds good";
}

float MixingAssistant::calculateMomentaryLUFS(const juce::AudioBuffer<float>& buffer) {
    // Simplified momentary loudness (400ms window)
    int windowSize = static_cast<int>(0.4f * mainSampleRate);
    if (buffer.getNumSamples() < windowSize) return calculateLUFS(buffer);
    
    juce::AudioBuffer<float> window(buffer.getNumChannels(), windowSize);
    window.copyFrom(0, 0, buffer, 0, 0, windowSize);
    
    return calculateLUFS(window);
}

float MixingAssistant::calculateShortTermLUFS(const juce::AudioBuffer<float>& buffer) {
    // Simplified short-term loudness (3 second window)
    int windowSize = static_cast<int>(3.0f * mainSampleRate);
    if (buffer.getNumSamples() < windowSize) return calculateLUFS(buffer);
    
    juce::AudioBuffer<float> window(buffer.getNumChannels(), windowSize);
    window.copyFrom(0, 0, buffer, 0, 0, windowSize);
    
    return calculateLUFS(window);
}

float MixingAssistant::calculateDynamicRange(const juce::AudioBuffer<float>& buffer) {
    // Calculate peak to RMS ratio
    float peak = calculatePeak(buffer);
    float rms = 20.0f * std::log10(calculateRMS(buffer));
    
    return peak - rms;
}

float MixingAssistant::detectFundamental(const std::vector<float>& magnitude) {
    // Find the peak frequency
    int maxBin = 1;
    float maxMagnitude = magnitude[1];
    
    for (int i = 2; i < magnitude.size(); ++i) {
        if (magnitude[i] > maxMagnitude) {
            maxMagnitude = magnitude[i];
            maxBin = i;
        }
    }
    
    return maxBin * mainSampleRate / fft.getSize();
}

float MixingAssistant::calculatePitchConfidence(const juce::AudioBuffer<float>& buffer) {
    // Simplified pitch confidence calculation
    performFFT(buffer);
    
    float maxMagnitude = 0.0f;
    float totalMagnitude = 0.0f;
    
    for (int i = 1; i < magnitudeBuffer.size(); ++i) {
        maxMagnitude = juce::jmax(maxMagnitude, magnitudeBuffer[i]);
        totalMagnitude += magnitudeBuffer[i];
    }
    
    return totalMagnitude > 0.0f ? maxMagnitude / totalMagnitude : 0.0f;
}

void MixingAssistant::analyzeHarmony(MixAnalysis& analysis) {
    // Simplified harmony analysis
    // Would implement chromagram calculation here
}

void MixingAssistant::detectKey(MixAnalysis& analysis) {
    // Simplified key detection
    analysis.detectedKey = "C";
    analysis.detectedMode = "major";
    analysis.keyConfidence = 0.5f;
}

float MixingAssistant::detectTempo() {
    // Simplified tempo detection
    return 120.0f; // Default tempo
}

void MixingAssistant::detectTimeSignature(MixAnalysis& analysis) {
    analysis.timeSignatureNumerator = 4;
    analysis.timeSignatureDenominator = 4;
    analysis.timeSignatureConfidence = 0.8f;
}

void MixingAssistant::trackBeats(MixAnalysis& analysis) {
    // Simplified beat tracking
    float tempo = analysis.tempo;
    float beatInterval = 60.0f / tempo;
    
    analysis.beatPositions.clear();
    for (float t = 0.0f; t < 10.0f; t += beatInterval) {
        analysis.beatPositions.push_back(t);
        analysis.beatStrengths.push_back(0.8f);
    }
}

void MixingAssistant::detectOnsets(MixAnalysis& analysis) {
    // Simplified onset detection
    analysis.onsets.clear();
    // Would implement actual onset detection here
}

void MixingAssistant::calculateMFCC(MixAnalysis& analysis) {
    // Simplified MFCC calculation
    analysis.mfccCoefficients.resize(13, 0.0f);
    // Would implement actual MFCC calculation here
}

void MixingAssistant::calculateTimbreDescriptors(MixAnalysis& analysis) {
    // Calculate timbre descriptors
    analysis.brightness = juce::jmap(analysis.spectralCentroid, 200.0f, 8000.0f, 0.0f, 1.0f);
    analysis.roughness = 0.3f; // Placeholder
    analysis.warmth = juce::jmap(analysis.bassLevel, -60.0f, 0.0f, 0.0f, 1.0f);
}

void MixingAssistant::classifyInstrument(MixAnalysis& analysis) {
    // Simplified instrument classification
    std::vector<std::pair<juce::String, float>> probabilities;
    probabilities.push_back({"piano", 0.3f});
    probabilities.push_back({"guitar", 0.2f});
    probabilities.push_back({"drums", 0.2f});
    probabilities.push_back({"bass", 0.1f});
    probabilities.push_back({"vocal", 0.1f});
    probabilities.push_back({"synth", 0.1f});
    
    analysis.instrumentProbabilities = probabilities;
    analysis.primaryInstrument = "piano";
    analysis.instrumentConfidence = 0.3f;
}

void MixingAssistant::assessClarity(MixAnalysis& analysis) {
    // Assess mix clarity
    float clarity = 1.0f;
    
    // Reduce clarity for muddy mixes
    if (analysis.lowMidLevel > analysis.midLevel + 3.0f) {
        clarity -= 0.2f;
    }
    
    // Reduce clarity for harsh mixes
    if (analysis.brillianceLevel > analysis.presenceLevel + 6.0f) {
        clarity -= 0.2f;
    }
    
    analysis.clarity = juce::jlimit(0.0f, 1.0f, clarity);
}

void MixingAssistant::assessPunch(MixAnalysis& analysis) {
    // Assess punch based on transients and dynamics
    float punch = 0.5f;
    
    // Increase punch for dynamic mixes
    if (analysis.dynamicRange > 12.0f) {
        punch += 0.3f;
    }
    
    // Increase punch for strong transients
    if (analysis.crestFactor > 15.0f) {
        punch += 0.2f;
    }
    
    analysis.punch = juce::jlimit(0.0f, 1.0f, punch);
}

void MixingAssistant::assessWarmth(MixAnalysis& analysis) {
    // Assess warmth based on bass and low-mid balance
    float warmth = juce::jmap(analysis.bassLevel, -60.0f, 0.0f, 0.0f, 0.5f);
    warmth += juce::jmap(analysis.lowMidLevel, -60.0f, 0.0f, 0.0f, 0.5f);
    
    analysis.warmth = juce::jlimit(0.0f, 1.0f, warmth);
}

void MixingAssistant::detectIssues(MixAnalysis& analysis) {
    // Already implemented in detectMixIssues()
}

void MixingAssistant::generateRecommendations(MixAnalysis& analysis) {
    analysis.recommendations.clear();
    
    if (analysis.clarity < 0.5f) {
        analysis.recommendations.push_back("Consider EQ to improve clarity");
    }
    
    if (analysis.punch < 0.5f) {
        analysis.recommendations.push_back("Add compression to increase punch");
    }
    
    if (analysis.warmth < 0.5f) {
        analysis.recommendations.push_back("Boost low-mids for warmth");
    }
    
    if (analysis.stereoWidth < 0.5f) {
        analysis.recommendations.push_back("Increase stereo width");
    }
}

void MixingAssistant::calculateOverallScore(MixAnalysis& analysis) {
    // Calculate overall quality score
    float score = (analysis.clarity + analysis.punch + analysis.warmth + 
                   analysis.presence + analysis.stereoImage + 
                   analysis.dynamicRange / 20.0f) / 6.0f;
    
    score = juce::jlimit(0.0f, 100.0f, score * 100.0f);
    analysis.overallScore = score;
}

void MixingAssistant::notifyAnalysisUpdated(const MixAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->analysisUpdated(analysis);
    }
}

void MixingAssistant::notifySuggestionGenerated(const MixSuggestion& suggestion) {
    for (auto* listener : listeners) {
        listener->suggestionGenerated(suggestion);
    }
}

void MixingAssistant::notifySuggestionApplied(const MixSuggestion& suggestion) {
    for (auto* listener : listeners) {
        listener->suggestionApplied(suggestion);
    }
}

void MixingAssistant::notifySuggestionRejected(const MixSuggestion& suggestion) {
    for (auto* listener : listeners) {
        listener->suggestionRejected(suggestion);
    }
}

void MixingAssistant::notifyGenreDetected(const juce::String& genre) {
    for (auto* listener : listeners) {
        listener->genreDetected(genre);
    }
}

void MixingAssistant::notifyAutoMixStarted() {
    for (auto* listener : listeners) {
        listener->autoMixStarted();
    }
}

void MixingAssistant::notifyAutoMixStopped() {
    for (auto* listener : listeners) {
        listener->autoMixStopped();
    }
}

void MixingAssistant::notifyReferenceTrackLoaded() {
    for (auto* listener : listeners) {
        listener->referenceTrackLoaded();
    }
}

void MixingAssistant::notifyReferenceTrackMatched() {
    for (auto* listener : listeners) {
        listener->referenceTrackMatched();
    }
}

namespace {
constexpr int kPanelPadding = 16;
constexpr int kPanelGap = 12;
constexpr int kAnalysisPanelWidth = 280;
constexpr int kSettingsPanelWidth = 240;
constexpr int kSuggestionsHeaderHeight = 28;
constexpr int kSuggestionsFooterHeight = 40;
constexpr float kPanelRadius = 12.0f;

inline SkRect toSkRect(const juce::Rectangle<int>& rect) {
    return SkRect::MakeXYWH(static_cast<float>(rect.getX()),
                            static_cast<float>(rect.getY()),
                            static_cast<float>(rect.getWidth()),
                            static_cast<float>(rect.getHeight()));
}

inline float normalizeValue(float value, float minValue, float maxValue) {
    if (maxValue <= minValue) return 0.0f;
    return juce::jlimit(0.0f, 1.0f, (value - minValue) / (maxValue - minValue));
}

inline std::string toStdString(const juce::String& text) {
    return text.toStdString();
}
} // namespace

// MixSuggestionComponent Implementation
MixSuggestionComponent::MixSuggestionComponent(const MixSuggestion& suggestion)
    : suggestion_(suggestion) {
    applyButton_ = std::make_unique<zenith::ZenithButton>("Apply");
    applyButton_->setButtonStyle(zenith::ZenithButton::Style::Success);
    applyButton_->setButtonSize(zenith::ZenithButton::Size::Small);
    applyButton_->onClick = [this]() {
        for (auto* listener : listeners_) {
            listener->suggestionApplied(suggestion_);
        }
    };
    addAndMakeVisible(*applyButton_);

    rejectButton_ = std::make_unique<zenith::ZenithButton>("Reject");
    rejectButton_->setButtonStyle(zenith::ZenithButton::Style::Danger);
    rejectButton_->setButtonSize(zenith::ZenithButton::Size::Small);
    rejectButton_->onClick = [this]() {
        for (auto* listener : listeners_) {
            listener->suggestionRejected(suggestion_);
        }
    };
    addAndMakeVisible(*rejectButton_);

    setApplied(suggestion_.isApplied);
}

MixSuggestionComponent::~MixSuggestionComponent() = default;

void MixSuggestionComponent::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
    SkRRect rrect = SkRRect::MakeRectXY(rect, kPanelRadius, kPanelRadius);

    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_02);
    canvas->drawRRect(rrect, bgPaint);

    if (suggestion_.isApplied) {
        SkPaint appliedPaint;
        appliedPaint.setColor(design::colors::GREEN_GLOW);
        canvas->drawRRect(rrect, appliedPaint);
    }

    SkPaint borderPaint;
    borderPaint.setColor(design::colors::BORDER_SUBTLE);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    const float padding = 10.0f;
    float contentRight = bounds.getRight() - padding;
    if (applyButton_) {
        contentRight = static_cast<float>(applyButton_->getX()) - padding;
    }

    SkFont titleFont = design::typography::getSkFont(13.0f, design::FontWeight::SemiBold);
    SkFont metaFont = design::typography::getSkFont(11.0f, design::FontWeight::Regular);

    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);

    std::string description = toStdString(suggestion_.description);
    float textX = bounds.getX() + padding;
    float textY = bounds.getY() + padding + titleFont.getSize();
    canvas->drawString(description.c_str(), textX, textY, titleFont, textPaint);

    SkPaint metaPaint;
    metaPaint.setColor(design::colors::TEXT_SECONDARY);
    metaPaint.setAntiAlias(true);

    juce::String target = suggestion_.targetTrack.isNotEmpty() ? suggestion_.targetTrack : "Mix Bus";
    juce::String metaLine = target + " • " + suggestion_.type;
    std::string metaStr = toStdString(metaLine);
    canvas->drawString(metaStr.c_str(), textX, textY + 16.0f, metaFont, metaPaint);

    float barWidth = juce::jmax(0.0f, contentRight - textX);
    float barY = bounds.getBottom() - 12.0f;

    SkRect barBg = SkRect::MakeXYWH(textX, barY, barWidth, 4.0f);
    SkPaint barBgPaint;
    barBgPaint.setColor(design::colors::BG_04);
    canvas->drawRect(barBg, barBgPaint);

    SkPaint confidencePaint;
    confidencePaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRect(SkRect::MakeXYWH(textX, barY, barWidth * juce::jlimit(0.0f, 1.0f, suggestion_.confidence), 4.0f),
                     confidencePaint);

    SkPaint priorityPaint;
    priorityPaint.setColor(design::colors::MAGENTA);
    canvas->drawRect(SkRect::MakeXYWH(textX, barY + 6.0f, barWidth * juce::jlimit(0.0f, 1.0f, suggestion_.priority), 3.0f),
                     priorityPaint);
}

void MixSuggestionComponent::resized() {
    auto bounds = getLocalBounds().reduced(10);
    const int buttonWidth = 60;
    const int buttonHeight = 22;
    const int gap = 6;

    auto buttonArea = bounds.removeFromRight(buttonWidth * 2 + gap);
    rejectButton_->setBounds(buttonArea.removeFromRight(buttonWidth).withHeight(buttonHeight));
    applyButton_->setBounds(buttonArea.removeFromRight(buttonWidth).withHeight(buttonHeight));
}

void MixSuggestionComponent::setApplied(bool applied) {
    suggestion_.isApplied = applied;
    if (applyButton_) {
        applyButton_->setEnabled(!applied);
    }
    markDirty();
}

void MixSuggestionComponent::addListener(Listener* listener) {
    listeners_.push_back(listener);
}

void MixSuggestionComponent::removeListener(Listener* listener) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
}

// MixingAssistantUI Implementation
MixingAssistantUI::MixingAssistantUI() {
    createControls();
    wireControlCallbacks();
    updateVisibility();
}

MixingAssistantUI::~MixingAssistantUI() {
    if (assistant) {
        assistant->removeListener(this);
    }
}

void MixingAssistantUI::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_01);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);

    if (showAnalysis_ && !analysisBounds_.isEmpty()) {
        drawPanelBackground(canvas, analysisBounds_);
        drawAnalysisPanel(canvas);
    }

    if (showSuggestions_ && !suggestionsBounds_.isEmpty()) {
        drawPanelBackground(canvas, suggestionsBounds_);
        drawSuggestionsHeader(canvas);
    }

    if (!settingsBounds_.isEmpty()) {
        drawPanelBackground(canvas, settingsBounds_);
        drawSettingsHeader(canvas);
    }
}

void MixingAssistantUI::resized() {
    auto content = getLocalBounds().reduced(kPanelPadding);

    int leftWidth = showAnalysis_ ? kAnalysisPanelWidth : 0;
    int rightWidth = kSettingsPanelWidth;

    analysisBounds_ = leftWidth > 0 ? content.removeFromLeft(leftWidth) : juce::Rectangle<int>();
    if (leftWidth > 0) {
        content.removeFromLeft(kPanelGap);
    }

    settingsBounds_ = content.removeFromRight(rightWidth);
    content.removeFromRight(kPanelGap);
    suggestionsBounds_ = content;

    auto suggestionsInner = suggestionsBounds_.reduced(kPanelPadding);
    suggestionsHeaderBounds_ = suggestionsInner.removeFromTop(kSuggestionsHeaderHeight);
    suggestionsFooterBounds_ = suggestionsInner.removeFromBottom(kSuggestionsFooterHeight);
    suggestionsListBounds_ = suggestionsInner;

    if (suggestionsList_) {
        suggestionsList_->setBounds(suggestionsListBounds_);
    }

    if (applySelectedButton_ && rejectSelectedButton_ && applyAllButton_ && clearAllButton_) {
        auto footer = suggestionsFooterBounds_;
        const int buttonWidth = 84;
        const int buttonHeight = 28;
        const int gap = 8;

        applySelectedButton_->setBounds(footer.removeFromLeft(buttonWidth).withHeight(buttonHeight));
        footer.removeFromLeft(gap);
        rejectSelectedButton_->setBounds(footer.removeFromLeft(buttonWidth).withHeight(buttonHeight));
        footer.removeFromLeft(gap);
        applyAllButton_->setBounds(footer.removeFromLeft(buttonWidth).withHeight(buttonHeight));
        footer.removeFromLeft(gap);
        clearAllButton_->setBounds(footer.removeFromLeft(buttonWidth).withHeight(buttonHeight));
    }

    auto settingsInner = settingsBounds_.reduced(kPanelPadding);
    settingsHeaderBounds_ = settingsInner.removeFromTop(24);

    if (settingsButton_) {
        settingsButton_->setBounds(settingsInner.removeFromTop(28));
    }

    settingsInner.removeFromTop(8);

    if (realTimeToggle_) {
        realTimeToggle_->setBounds(settingsInner.removeFromTop(26));
    }
    if (aiToggle_) {
        aiToggle_->setBounds(settingsInner.removeFromTop(26));
    }

    settingsInner.removeFromTop(12);

    if (autoMixToggle_) {
        autoMixToggle_->setBounds(settingsInner.removeFromTop(28));
    }
    if (autoMixStrengthSlider_) {
        autoMixStrengthSlider_->setBounds(settingsInner.removeFromTop(32));
    }
}

void MixingAssistantUI::setAssistant(MixingAssistant* newAssistant) {
    if (assistant == newAssistant) {
        return;
    }

    if (assistant) {
        assistant->removeListener(this);
    }

    assistant = newAssistant;

    if (assistant) {
        assistant->addListener(this);
        updateAnalysisSnapshot(assistant->getCurrentAnalysis());
        refreshSuggestions();

        auto config = assistant->getConfig();
        if (realTimeToggle_) {
            realTimeToggle_->setToggleState(config.enableRealTimeAnalysis, false);
        }
        if (aiToggle_) {
            aiToggle_->setToggleState(config.enableAISuggestions, false);
        }
        if (autoMixToggle_) {
            autoMixToggle_->setToggleState(assistant->isAutoMixActive(), false);
        }
    }
}

void MixingAssistantUI::showAnalysis(bool show) {
    showAnalysis_ = show;
    resized();
    markDirty();
}

void MixingAssistantUI::showSuggestions(bool show) {
    showSuggestions_ = show;
    updateVisibility();
}

void MixingAssistantUI::showAutoMix(bool show) {
    showAutoMix_ = show;
    updateVisibility();
}

void MixingAssistantUI::analysisUpdated(const MixAnalysis& analysis) {
    updateAnalysisSnapshot(analysis);
}

void MixingAssistantUI::suggestionGenerated(const MixSuggestion& suggestion) {
    juce::ignoreUnused(suggestion);
    refreshSuggestions();
}

void MixingAssistantUI::suggestionApplied(const MixSuggestion& suggestion) {
    juce::ignoreUnused(suggestion);
    refreshSuggestions();
}

void MixingAssistantUI::genreDetected(const juce::String& genre) {
    detectedGenre_ = genre;
    markDirty();
}

void MixingAssistantUI::autoMixStarted() {
    updateAutoMixControls();
}

void MixingAssistantUI::autoMixStopped() {
    updateAutoMixControls();
}

int MixingAssistantUI::getNumRows() {
    return static_cast<int>(suggestionsCache_.size());
}

void MixingAssistantUI::paintListBoxItem(int rowNumber, SkCanvas& canvas, int width,
                                        int height, bool rowIsSelected) {
    if (rowNumber < 0 || rowNumber >= static_cast<int>(suggestionsCache_.size())) {
        return;
    }

    const auto& suggestion = suggestionsCache_[rowNumber];

    canvas.save();
    canvas.translate(0.0f, static_cast<float>(rowNumber * height));

    SkPaint textPaint;
    textPaint.setColor(rowIsSelected ? design::colors::TEXT_INVERSE : design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);

    SkPaint metaPaint;
    metaPaint.setColor(rowIsSelected ? design::colors::TEXT_INVERSE : design::colors::TEXT_SECONDARY);
    metaPaint.setAntiAlias(true);

    SkFont titleFont = design::typography::getSkFont(12.0f, design::FontWeight::Medium);
    SkFont metaFont = design::typography::getSkFont(10.0f, design::FontWeight::Regular);

    float textX = 10.0f;
    float textY = 18.0f;

    canvas.drawString(toStdString(suggestion.description).c_str(), textX, textY, titleFont, textPaint);

    juce::String target = suggestion.targetTrack.isNotEmpty() ? suggestion.targetTrack : "Mix Bus";
    juce::String metaLine = target + " • " + suggestion.type;
    canvas.drawString(toStdString(metaLine).c_str(), textX, textY + 14.0f, metaFont, metaPaint);

    float barWidth = static_cast<float>(width) - 20.0f;
    float barY = static_cast<float>(height) - 12.0f;

    SkRect barBg = SkRect::MakeXYWH(textX, barY, barWidth, 3.0f);
    SkPaint barBgPaint;
    barBgPaint.setColor(design::colors::BG_04);
    canvas.drawRect(barBg, barBgPaint);

    SkPaint confidencePaint;
    confidencePaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas.drawRect(SkRect::MakeXYWH(textX, barY, barWidth * juce::jlimit(0.0f, 1.0f, suggestion.confidence), 3.0f),
                     confidencePaint);

    canvas.restore();
}

void MixingAssistantUI::listBoxItemClicked(int rowNumber, const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    selectedSuggestionIndex_ = rowNumber;
    if (applySelectedButton_) {
        applySelectedButton_->setEnabled(selectedSuggestionIndex_ >= 0);
    }
    if (rejectSelectedButton_) {
        rejectSelectedButton_->setEnabled(selectedSuggestionIndex_ >= 0);
    }
}

juce::String MixingAssistantUI::getTooltipForRow(int rowNumber) {
    if (rowNumber < 0 || rowNumber >= static_cast<int>(suggestionsCache_.size())) {
        return {};
    }
    return suggestionsCache_[rowNumber].description;
}

void MixingAssistantUI::createControls() {
    suggestionsList_ = std::make_unique<zenith::SkiaListBox>("MixSuggestions");
    suggestionsList_->setRowHeight(68);
    suggestionsList_->setColour(design::colors::BG_03);
    suggestionsList_->setModel(this);
    suggestionsList_->onRowClicked = [this](int row) {
        selectedSuggestionIndex_ = row;
        if (applySelectedButton_) applySelectedButton_->setEnabled(row >= 0);
        if (rejectSelectedButton_) rejectSelectedButton_->setEnabled(row >= 0);
    };
    addAndMakeVisible(*suggestionsList_);

    applySelectedButton_ = std::make_unique<zenith::ZenithButton>("Apply");
    applySelectedButton_->setButtonStyle(zenith::ZenithButton::Style::Success);
    applySelectedButton_->setButtonSize(zenith::ZenithButton::Size::Small);
    addAndMakeVisible(*applySelectedButton_);

    rejectSelectedButton_ = std::make_unique<zenith::ZenithButton>("Reject");
    rejectSelectedButton_->setButtonStyle(zenith::ZenithButton::Style::Danger);
    rejectSelectedButton_->setButtonSize(zenith::ZenithButton::Size::Small);
    addAndMakeVisible(*rejectSelectedButton_);

    applyAllButton_ = std::make_unique<zenith::ZenithButton>("Apply All");
    applyAllButton_->setButtonStyle(zenith::ZenithButton::Style::Secondary);
    applyAllButton_->setButtonSize(zenith::ZenithButton::Size::Small);
    addAndMakeVisible(*applyAllButton_);

    clearAllButton_ = std::make_unique<zenith::ZenithButton>("Clear");
    clearAllButton_->setButtonStyle(zenith::ZenithButton::Style::Ghost);
    clearAllButton_->setButtonSize(zenith::ZenithButton::Size::Small);
    addAndMakeVisible(*clearAllButton_);

    settingsButton_ = std::make_unique<zenith::ZenithButton>("Advanced");
    settingsButton_->setButtonStyle(zenith::ZenithButton::Style::Ghost);
    settingsButton_->setButtonSize(zenith::ZenithButton::Size::Small);
    addAndMakeVisible(*settingsButton_);

    realTimeToggle_ = std::make_unique<zenith::ZenithToggle>("Real-time Analysis");
    realTimeToggle_->setStyle(zenith::ZenithToggle::Style::Switch);
    realTimeToggle_->setToggleState(true, false);
    addAndMakeVisible(*realTimeToggle_);

    aiToggle_ = std::make_unique<zenith::ZenithToggle>("AI Suggestions");
    aiToggle_->setStyle(zenith::ZenithToggle::Style::Switch);
    aiToggle_->setToggleState(true, false);
    addAndMakeVisible(*aiToggle_);

    autoMixToggle_ = std::make_unique<zenith::ZenithToggle>("Auto-Mix");
    autoMixToggle_->setStyle(zenith::ZenithToggle::Style::Switch);
    addAndMakeVisible(*autoMixToggle_);

    autoMixStrengthSlider_ = std::make_unique<zenith::ZenithSlider>(zenith::ZenithSlider::Horizontal);
    autoMixStrengthSlider_->setRange(0.0f, 1.0f, 0.5f);
    autoMixStrengthSlider_->setValue(0.5f, false);
    autoMixStrengthSlider_->setLabel("Strength");
    addAndMakeVisible(*autoMixStrengthSlider_);
}

void MixingAssistantUI::wireControlCallbacks() {
    if (applySelectedButton_) {
        applySelectedButton_->onClick = [this]() { applySelectedSuggestion(); };
        applySelectedButton_->setEnabled(false);
    }

    if (rejectSelectedButton_) {
        rejectSelectedButton_->onClick = [this]() { rejectSelectedSuggestion(); };
        rejectSelectedButton_->setEnabled(false);
    }

    if (applyAllButton_) {
        applyAllButton_->onClick = [this]() { applyAllSuggestions(); };
    }

    if (clearAllButton_) {
        clearAllButton_->onClick = [this]() { clearAllSuggestions(); };
    }

    if (settingsButton_) {
        settingsButton_->onClick = [this]() {
            showAnalysis_ = !showAnalysis_;
            settingsButton_->setText(showAnalysis_ ? "Advanced" : "Basic");
            resized();
            markDirty();
        };
    }

    if (realTimeToggle_) {
        realTimeToggle_->onToggle = [this](bool state) {
            updateConfig([&](MixingAssistantConfig& cfg) { cfg.enableRealTimeAnalysis = state; });
        };
    }

    if (aiToggle_) {
        aiToggle_->onToggle = [this](bool state) {
            updateConfig([&](MixingAssistantConfig& cfg) { cfg.enableAISuggestions = state; });
        };
    }

    if (autoMixToggle_) {
        autoMixToggle_->onToggle = [this](bool state) {
            if (!assistant) return;
            if (state) assistant->startAutoMix();
            else assistant->stopAutoMix();
        };
    }

    if (autoMixStrengthSlider_) {
        autoMixStrengthSlider_->onValueChange = [this](float value) {
            if (!assistant) return;
            assistant->setAutoMixStrength(value);
        };
    }
}

void MixingAssistantUI::updateAnalysisSnapshot(const MixAnalysis& analysis) {
    analysisSnapshot_ = analysis;
    if (analysis.detectedGenre.isNotEmpty()) {
        detectedGenre_ = analysis.detectedGenre;
    }
    markDirty();
}

void MixingAssistantUI::refreshSuggestions() {
    suggestionsCache_.clear();
    if (assistant) {
        suggestionsCache_ = assistant->getSuggestions();
    }

    if (selectedSuggestionIndex_ >= static_cast<int>(suggestionsCache_.size())) {
        selectedSuggestionIndex_ = -1;
    }

    if (suggestionsList_) {
        suggestionsList_->updateContent();
    }

    if (applySelectedButton_) applySelectedButton_->setEnabled(selectedSuggestionIndex_ >= 0);
    if (rejectSelectedButton_) rejectSelectedButton_->setEnabled(selectedSuggestionIndex_ >= 0);
    markDirty();
}

void MixingAssistantUI::updateAutoMixControls() {
    if (!assistant || !autoMixToggle_) return;
    autoMixToggle_->setToggleState(assistant->isAutoMixActive(), false);
}

void MixingAssistantUI::updateConfig(const std::function<void(MixingAssistantConfig&)>& mutator) {
    if (!assistant) return;
    auto config = assistant->getConfig();
    mutator(config);
    assistant->setConfig(config);
}

void MixingAssistantUI::updateVisibility() {
    if (suggestionsList_) suggestionsList_->setVisible(showSuggestions_);
    if (applySelectedButton_) applySelectedButton_->setVisible(showSuggestions_);
    if (rejectSelectedButton_) rejectSelectedButton_->setVisible(showSuggestions_);
    if (applyAllButton_) applyAllButton_->setVisible(showSuggestions_);
    if (clearAllButton_) clearAllButton_->setVisible(showSuggestions_);

    if (autoMixToggle_) autoMixToggle_->setVisible(showAutoMix_);
    if (autoMixStrengthSlider_) autoMixStrengthSlider_->setVisible(showAutoMix_);

    resized();
    markDirty();
}

void MixingAssistantUI::applySelectedSuggestion() {
    if (!assistant || selectedSuggestionIndex_ < 0 || selectedSuggestionIndex_ >= static_cast<int>(suggestionsCache_.size())) {
        return;
    }
    assistant->applySuggestion(suggestionsCache_[selectedSuggestionIndex_].id);
}

void MixingAssistantUI::rejectSelectedSuggestion() {
    if (!assistant || selectedSuggestionIndex_ < 0 || selectedSuggestionIndex_ >= static_cast<int>(suggestionsCache_.size())) {
        return;
    }
    assistant->rejectSuggestion(suggestionsCache_[selectedSuggestionIndex_].id);
}

void MixingAssistantUI::applyAllSuggestions() {
    if (!assistant) return;
    for (const auto& suggestion : suggestionsCache_) {
        assistant->applySuggestion(suggestion.id);
    }
}

void MixingAssistantUI::clearAllSuggestions() {
    if (assistant) {
        assistant->clearSuggestions();
    }
    refreshSuggestions();
}

void MixingAssistantUI::drawPanelBackground(SkCanvas* canvas, const juce::Rectangle<int>& bounds) {
    if (bounds.isEmpty()) return;

    SkRect rect = toSkRect(bounds);
    SkRRect rrect = SkRRect::MakeRectXY(rect, kPanelRadius, kPanelRadius);

    SkPaint fillPaint;
    fillPaint.setColor(design::colors::BG_02);
    canvas->drawRRect(rrect, fillPaint);

    SkPaint borderPaint;
    borderPaint.setColor(design::colors::BORDER_SUBTLE);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);
}

void MixingAssistantUI::drawAnalysisPanel(SkCanvas* canvas) {
    if (analysisBounds_.isEmpty()) return;

    auto content = analysisBounds_.reduced(kPanelPadding);

    SkFont titleFont = design::typography::getSkFont(15.0f, design::FontWeight::SemiBold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString("Mix Analysis", static_cast<float>(content.getX()),
                       static_cast<float>(content.getY() + 18), titleFont, titlePaint);

    SkFont labelFont = design::typography::getSkFont(11.0f, design::FontWeight::Regular);
    SkFont valueFont = design::typography::getSkFont(11.0f, design::FontWeight::Medium);

    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_SECONDARY);
    labelPaint.setAntiAlias(true);

    SkPaint valuePaint;
    valuePaint.setColor(design::colors::TEXT_PRIMARY);
    valuePaint.setAntiAlias(true);

    float y = static_cast<float>(content.getY() + 40);
    float rowHeight = 26.0f;
    float barWidth = static_cast<float>(content.getWidth()) - 20.0f;

    auto drawMetric = [&](const juce::String& label, const juce::String& value, float norm) {
        canvas->drawString(toStdString(label).c_str(), static_cast<float>(content.getX()), y, labelFont, labelPaint);
        float valueWidth = valueFont.measureText(value.toRawUTF8(), value.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
        canvas->drawString(toStdString(value).c_str(),
                           static_cast<float>(content.getRight()) - valueWidth,
                           y, valueFont, valuePaint);

        SkPaint barBgPaint;
        barBgPaint.setColor(design::colors::BG_04);
        SkPaint barFillPaint;
        barFillPaint.setColor(design::colors::ACCENT_PRIMARY);
        float barY = y + 6.0f;
        canvas->drawRect(SkRect::MakeXYWH(static_cast<float>(content.getX()), barY, barWidth, 3.0f), barBgPaint);
        canvas->drawRect(SkRect::MakeXYWH(static_cast<float>(content.getX()), barY, barWidth * norm, 3.0f), barFillPaint);
        y += rowHeight;
    };

    drawMetric("Loudness", juce::String(analysisSnapshot_.overallLoudness, 1) + " LUFS",
               normalizeValue(analysisSnapshot_.overallLoudness, -60.0f, -6.0f));
    drawMetric("Dynamic Range", juce::String(analysisSnapshot_.dynamicRange, 1) + " dB",
               normalizeValue(analysisSnapshot_.dynamicRange, 0.0f, 20.0f));
    drawMetric("Stereo Width", juce::String(analysisSnapshot_.stereoWidth, 2),
               normalizeValue(analysisSnapshot_.stereoWidth, 0.0f, 2.0f));
    drawMetric("Clarity", juce::String(analysisSnapshot_.clarity * 100.0f, 0) + "%",
               normalizeValue(analysisSnapshot_.clarity, 0.0f, 1.0f));
    drawMetric("Punch", juce::String(analysisSnapshot_.punch * 100.0f, 0) + "%",
               normalizeValue(analysisSnapshot_.punch, 0.0f, 1.0f));
    drawMetric("Warmth", juce::String(analysisSnapshot_.warmth * 100.0f, 0) + "%",
               normalizeValue(analysisSnapshot_.warmth, 0.0f, 1.0f));

    if (showAnalysis_) {
        juce::String genreLabel = detectedGenre_.isNotEmpty() ? detectedGenre_ : "Detecting...";
        canvas->drawString(("Genre: " + genreLabel).toRawUTF8(),
                           static_cast<float>(content.getX()),
                           y + 6.0f, labelFont, labelPaint);
    }
}

void MixingAssistantUI::drawSuggestionsHeader(SkCanvas* canvas) {
    if (suggestionsHeaderBounds_.isEmpty()) return;

    SkFont font = design::typography::getSkFont(13.0f, design::FontWeight::SemiBold);
    SkPaint paint;
    paint.setColor(design::colors::TEXT_PRIMARY);
    paint.setAntiAlias(true);

    juce::String title = "Suggestions";
    canvas->drawString(title.toRawUTF8(),
                       static_cast<float>(suggestionsHeaderBounds_.getX()),
                       static_cast<float>(suggestionsHeaderBounds_.getY() + 18),
                       font, paint);

    SkPaint countPaint;
    countPaint.setColor(design::colors::TEXT_SECONDARY);
    countPaint.setAntiAlias(true);
    juce::String countText = juce::String(suggestionsCache_.size()) + " items";
    float countWidth = font.measureText(countText.toRawUTF8(), countText.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
    canvas->drawString(countText.toRawUTF8(),
                       static_cast<float>(suggestionsHeaderBounds_.getRight()) - countWidth,
                       static_cast<float>(suggestionsHeaderBounds_.getY() + 18),
                       font, countPaint);
}

void MixingAssistantUI::drawSettingsHeader(SkCanvas* canvas) {
    if (settingsHeaderBounds_.isEmpty()) return;

    SkFont font = design::typography::getSkFont(13.0f, design::FontWeight::SemiBold);
    SkPaint paint;
    paint.setColor(design::colors::TEXT_PRIMARY);
    paint.setAntiAlias(true);

    canvas->drawString("Mix Controls",
                       static_cast<float>(settingsHeaderBounds_.getX()),
                       static_cast<float>(settingsHeaderBounds_.getY() + 18),
                       font, paint);
}

// MixingAssistantFactory Implementation
std::unique_ptr<MixingAssistant> MixingAssistantFactory::createDefaultAssistant() {
    auto assistant = std::make_unique<MixingAssistant>();
    assistant->setConfig(getDefaultConfig());
    return assistant;
}

std::unique_ptr<MixingAssistant> MixingAssistantFactory::createProfessionalAssistant() {
    auto assistant = std::make_unique<MixingAssistant>();
    assistant->setConfig(getProfessionalConfig());
    return assistant;
}

std::unique_ptr<MixingAssistant> MixingAssistantFactory::createBeginnerAssistant() {
    auto assistant = std::make_unique<MixingAssistant>();
    assistant->setConfig(getBeginnerConfig());
    return assistant;
}

MixingAssistantConfig MixingAssistantFactory::getDefaultConfig() {
    MixingAssistantConfig config;
    config.enableRealTimeAnalysis = true;
    config.analysisInterval = 1000;
    config.analysisWindow = 10.0f;
    config.enableAISuggestions = true;
    config.suggestionThreshold = 0.6f;
    config.maxSuggestionsPerTrack = 5;
    config.learnFromUserActions = true;
    config.autoDetectGenre = true;
    config.useGenreSpecificTargets = true;
    config.enableAutoCorrection = false;
    config.correctionStrength = 0.5f;
    config.preserveDynamics = true;
    config.preserveStereoImage = true;
    config.showRealTimeFeedback = true;
    config.showSuggestions = true;
    config.showAnalysis = true;
    config.enableUndoRedo = true;
    return config;
}

MixingAssistantConfig MixingAssistantFactory::getProfessionalConfig() {
    auto config = getDefaultConfig();
    config.suggestionThreshold = 0.5f;
    config.maxSuggestionsPerTrack = 10;
    config.enableAutoCorrection = true;
    config.correctionStrength = 0.3f;
    return config;
}

MixingAssistantConfig MixingAssistantFactory::getBeginnerConfig() {
    auto config = getDefaultConfig();
    config.suggestionThreshold = 0.8f;
    config.maxSuggestionsPerTrack = 3;
    config.enableAutoCorrection = true;
    config.correctionStrength = 0.7f;
    return config;
}

std::unordered_map<juce::String, float> MixingAssistantFactory::getRockTargets() {
    return {
        {"overallLoudness", -10.0f},
        {"bassLevel", -3.0f},
        {"midLevel", -6.0f},
        {"highMidLevel", -4.0f},
        {"stereoWidth", 0.8f},
        {"dynamicRange", 14.0f}
    };
}

std::unordered_map<juce::String, float> MixingAssistantFactory::getPopTargets() {
    return {
        {"overallLoudness", -8.0f},
        {"bassLevel", -6.0f},
        {"midLevel", -8.0f},
        {"highMidLevel", -6.0f},
        {"stereoWidth", 0.6f},
        {"dynamicRange", 10.0f}
    };
}

std::unordered_map<juce::String, float> MixingAssistantFactory::getJazzTargets() {
    return {
        {"overallLoudness", -14.0f},
        {"bassLevel", -12.0f},
        {"midLevel", -10.0f},
        {"highMidLevel", -12.0f},
        {"stereoWidth", 0.4f},
        {"dynamicRange", 18.0f}
    };
}

std::unordered_map<juce::String, float> MixingAssistantFactory::getElectronicTargets() {
    return {
        {"overallLoudness", -6.0f},
        {"bassLevel", -2.0f},
        {"midLevel", -8.0f},
        {"highMidLevel", -4.0f},
        {"stereoWidth", 1.0f},
        {"dynamicRange", 8.0f}
    };
}

std::unordered_map<juce::String, float> MixingAssistantFactory::getOrchestralTargets() {
    return {
        {"overallLoudness", -18.0f},
        {"bassLevel", -15.0f},
        {"midLevel", -12.0f},
        {"highMidLevel", -14.0f},
        {"stereoWidth", 0.3f},
        {"dynamicRange", 20.0f}
    };
}

} // namespace ai
} // namespace zenith
