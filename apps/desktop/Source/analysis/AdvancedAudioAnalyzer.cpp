/*
  ==============================================================================
    AdvancedAudioAnalyzer.cpp
    Comprehensive audio analysis implementation
  ==============================================================================
*/

#include "AdvancedAudioAnalyzer.h"
#include <algorithm>
#include <numeric>

namespace zenith {
namespace analysis {

// AdvancedAudioAnalyzer Implementation
AdvancedAudioAnalyzer::AdvancedAudioAnalyzer() 
    : fft(1 << 12), window(fft.getSize(), juce::dsp::WindowingFunction<float>::hann) {
    
    // Initialize buffers
    fftBuffer.resize(fft.getSize() * 2);
    magnitudeBuffer.resize(fft.getSize() / 2 + 1);
    phaseBuffer.resize(fft.getSize() / 2 + 1);
    
    // Initialize frequency bins
    initializeFrequencyBins();
    initializeCriticalBands();
    initializeOctaveBands();
    
    // Set default config
    config = AnalyzerConfig();
    
    // Start timer for real-time analysis
    startTimer(config.updateInterval);
}

AdvancedAudioAnalyzer::~AdvancedAudioAnalyzer() {
    stopTimer();
}

void AdvancedAudioAnalyzer::setConfig(const AnalyzerConfig& newConfig) {
    config = newConfig;
    
    // Reinitialize FFT if size changed
    if (fft.getSize() != (1 << config.fftOrder)) {
        fft = juce::dsp::FFT(1 << config.fftOrder);
        window = juce::dsp::WindowingFunction<float>(fft.getSize(), config.windowMethod);
        fftBuffer.resize(fft.getSize() * 2);
        magnitudeBuffer.resize(fft.getSize() / 2 + 1);
        phaseBuffer.resize(fft.getSize() / 2 + 1);
        
        initializeFrequencyBins();
        initializeCriticalBands();
        initializeOctaveBands();
    }
    
    // Restart timer
    startTimer(config.updateInterval);
}

AdvancedAudioAnalyzer::AnalyzerConfig AdvancedAudioAnalyzer::getConfig() const {
    return config;
}

void AdvancedAudioAnalyzer::processAudio(const juce::AudioBuffer<float>& buffer) {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    // Store buffer for analysis
    mainAudioBuffer = buffer;
    currentSampleRate = config.sampleRate;
    
    if (config.enableRealTimeAnalysis) {
        needsAnalysis.store(true);
    }
}

void AdvancedAudioAnalyzer::setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    std::lock_guard<std::mutex> lock(audioMutex);
    mainAudioBuffer = buffer;
    currentSampleRate = sampleRate;
    needsAnalysis.store(true);
}

void AdvancedAudioAnalyzer::clearAudio() {
    std::lock_guard<std::mutex> lock(audioMutex);
    mainAudioBuffer.setSize(0, 0);
    currentSampleRate = 44100.0;
}

void AdvancedAudioAnalyzer::analyzeAll() {
    if (isAnalyzing.load()) return;
    
    isAnalyzing.store(true);
    
    try {
        analyzeLoudness(mainAudioBuffer);
        analyzeSpectrum();
        analyzePhase();
        analyzeDynamics();
        analyzePitch();
        analyzeRhythm();
        analyzeTimbre();
        analyzeQuality();
        
        lastAnalysisTime = juce::Time::getCurrentTime();
        notifyAnalysisComplete(AnalysisType::Loudness);
    } catch (const std::exception& e) {
        // Handle error
        DBG("Analysis error: " + juce::String(e.what()));
    }
    
    isAnalyzing.store(false);
}

void AdvancedAudioAnalyzer::analyzeLoudness() {
    std::lock_guard<std::mutex> lock(audioMutex);
    analyzeLoudness(mainAudioBuffer);
    notifyLoudnessUpdated(loudnessAnalysis);
}

void AdvancedAudioAnalyzer::analyzeSpectrum() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    // Perform FFT
    performFFT(mainAudioBuffer);
    
    // Calculate spectrum
    calculateSpectrum();
    calculateCriticalBands();
    calculateOctaveBands();
    calculateSpectralFeatures();
    calculateHarmonics();
    
    notifySpectrumUpdated(spectrumAnalysis);
}

void AdvancedAudioAnalyzer::analyzePhase() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    calculateStereoPhase();
    calculatePhaseCoherence();
    calculateMonoCompatibility();
    
    notifyPhaseUpdated(phaseAnalysis);
}

void AdvancedAudioAnalyzer::analyzeDynamics() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    calculateEnvelope();
    calculateTransients();
    calculateCompression();
    
    notifyDynamicsUpdated(dynamicAnalysis);
}

void AdvancedAudioAnalyzer::analyzePitch() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    float fundamental = detectFundamental(magnitudeBuffer);
    pitchAnalysis.fundamental = fundamental;
    pitchAnalysis.fundamentalConfidence = calculatePitchConfidence(mainAudioBuffer);
    
    calculateChromagram();
    detectKey();
    
    notifyPitchUpdated(pitchAnalysis);
}

void AdvancedAudioAnalyzer::analyzeRhythm() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    float tempo = detectTempo();
    rhythmAnalysis.tempo = tempo;
    rhythmAnalysis.tempoConfidence = 0.8f; // Simplified
    
    detectBeats();
    detectOnsets();
    
    notifyRhythmUpdated(rhythmAnalysis);
}

void AdvancedAudioAnalyzer::analyzeTimbre() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    calculateMFCC();
    calculateTimbreDescriptors();
    classifyInstrument();
    
    notifyTimbreUpdated(timbreAnalysis);
}

void AdvancedAudioAnalyzer::analyzeQuality() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    assessClarity();
    assessPunch();
    assessWarmth();
    detectIssues();
    
    notifyQualityUpdated(qualityAnalysis);
}

LoudnessAnalysis AdvancedAudioAnalyzer::getLoudnessAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return loudnessAnalysis;
}

SpectrumAnalysis AdvancedAudioAnalyzer::getSpectrumAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return spectrumAnalysis;
}

PhaseAnalysis AdvancedAudioAnalyzer::getPhaseAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return phaseAnalysis;
}

DynamicAnalysis AdvancedAudioAnalyzer::getDynamicAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return dynamicAnalysis;
}

PitchAnalysis AdvancedAudioAnalyzer::getPitchAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return pitchAnalysis;
}

RhythmAnalysis AdvancedAudioAnalyzer::getRhythmAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return rhythmAnalysis;
}

TimbreAnalysis AdvancedAudioAnalyzer::getTimbreAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return timbreAnalysis;
}

QualityAnalysis AdvancedAudioAnalyzer::getQualityAnalysis() const {
    std::lock_guard<std::mutex> lock(analysisMutex);
    return qualityAnalysis;
}

void AdvancedAudioAnalyzer::loadReference(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    referenceBuffer = buffer;
    referenceSampleRate = sampleRate;
    hasReference = true;
    
    // Analyze reference
    analyzeReferenceTrack();
}

void AdvancedAudioAnalyzer::compareWithReference() {
    if (!hasReference) return;
    
    float loudnessDiff = calculateLoudnessDifference();
    float spectrumDiff = calculateSpectralDifference();
    float phaseDiff = calculatePhaseDifference();
    
    referenceSimilarity = calculateOverallSimilarity();
    
    notifyReferenceCompared(referenceSimilarity);
}

float AdvancedAudioAnalyzer::getReferenceSimilarity() const {
    return referenceSimilarity;
}

bool AdvancedAudioAnalyzer::exportResults(const juce::File& file) const {
    try {
        juce::DynamicObject::Ptr data = new juce::DynamicObject();
        
        // Export loudness
        juce::DynamicObject::Ptr loudnessObj = new juce::DynamicObject();
        loudnessObj->setProperty("integratedLUFS", loudnessAnalysis.integratedLUFS);
        loudnessObj->setProperty("truePeak", loudnessAnalysis.truePeak);
        data->setProperty("loudness", loudnessObj);
        
        // Export spectrum
        juce::DynamicObject::Ptr spectrumObj = new juce::DynamicObject();
        spectrumObj->setProperty("spectralCentroid", spectrumAnalysis.spectralCentroid);
        spectrumObj->setProperty("spectralSpread", spectrumAnalysis.spectralSpread);
        data->setProperty("spectrum", spectrumObj);
        
        // Export other analyses...
        
        return file.replaceWithText(juce::JSON::toString(data));
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Failed to export analysis results: " + juce::String(e.what()));
        return false;
    }
}

bool AdvancedAudioAnalyzer::exportSpectrum(const juce::File& file) const {
    try {
        juce::DynamicObject::Ptr data = new juce::DynamicObject();
        
        // Export frequency data
        juce::Array<juce::var> freqArray;
        juce::Array<juce::var> magArray;
        
        for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
            freqArray.add(spectrumAnalysis.frequencies[i]);
            magArray.add(spectrumAnalysis.magnitudes[i]);
        }
        
        data->setProperty("frequencies", freqArray);
        data->setProperty("magnitudes", magArray);
        
        return file.replaceWithText(juce::JSON::toString(data));
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("Failed to export spectrum: " + juce::String(e.what()));
        return false;
    }
}

bool AdvancedAudioAnalyzer::exportSpectrogram(const juce::File& file) const {
    // Export spectrogram data
    return false; // Placeholder
}

void AdvancedAudioAnalyzer::timerCallback() {
    if (config.enableRealTimeAnalysis && needsAnalysis.load()) {
        analyzeAll();
        needsAnalysis.store(false);
    }
}

void AdvancedAudioAnalyzer::handleAsyncUpdate() {
    // Handle async updates
}

void AdvancedAudioAnalyzer::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void AdvancedAudioAnalyzer::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void AdvancedAudioAnalyzer::performFFT(const juce::AudioBuffer<float>& buffer) {
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

void AdvancedAudioAnalyzer::calculateLoudness(const juce::AudioBuffer<float>& buffer) {
    // Calculate integrated loudness (LUFS)
    loudnessAnalysis.integratedLUFS = calculateLUFS(buffer);
    
    // Calculate momentary loudness
    loudnessAnalysis.momentaryLUFS = calculateMomentaryLUFS(buffer);
    
    // Calculate short-term loudness
    loudnessAnalysis.shortTermLUFS = calculateShortTermLUFS(buffer);
    
    // Calculate loudness range
    loudnessAnalysis.loudnessRange = calculateLRA(buffer);
    
    // Calculate true peak
    loudnessAnalysis.truePeak = calculateTruePeak(buffer);
    
    // Calculate sample peak
    loudnessAnalysis.samplePeak = calculatePeak(buffer);
    
    // Calculate crest factor
    loudnessAnalysis.crestFactor = loudnessAnalysis.truePeak - loudnessAnalysis.integratedLUFS;
    
    // Calculate dynamic range
    loudnessAnalysis.dynamicRange = calculateDynamicRange(buffer);
    
    // Channel analysis
    loudnessAnalysis.channelLUFS.clear();
    loudnessAnalysis.channelPeaks.clear();
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        juce::AudioBuffer<float> channelBuffer(1, buffer.getNumSamples());
        channelBuffer.copyFrom(0, 0, buffer, ch, 0, buffer.getNumSamples());
        
        loudnessAnalysis.channelLUFS.push_back(calculateLUFS(channelBuffer));
        loudnessAnalysis.channelPeaks.push_back(calculatePeak(channelBuffer));
    }
}

void AdvancedAudioAnalyzer::calculateSpectrum() {
    float sampleRate = currentSampleRate;
    int fftSize = fft.getSize();
    float binWidth = sampleRate / fftSize;
    
    spectrumAnalysis.frequencies.clear();
    spectrumAnalysis.magnitudes.clear();
    spectrumAnalysis.phases.clear();
    
    for (int i = 0; i < fft.getSize() / 2 + 1; ++i) {
        float frequency = i * binWidth;
        
        if (frequency >= config.minFrequency && frequency <= config.maxFrequency) {
            spectrumAnalysis.frequencies.push_back(frequency);
            spectrumAnalysis.magnitudes.push_back(magnitudeBuffer[i]);
            spectrumAnalysis.phases.push_back(phaseBuffer[i]);
        }
    }
}

void AdvancedAudioAnalyzer::calculateCriticalBands() {
    // Calculate Bark scale critical bands
    spectrumAnalysis.criticalBands.clear();
    spectrumAnalysis.bandEnergies.clear();
    
    for (int bark = 0; bark < 24; ++bark) {
        float freqLow = barkToHz(bark);
        float freqHigh = barkToHz(bark + 1);
        
        float energy = 0.0f;
        int count = 0;
        
        for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
            if (spectrumAnalysis.frequencies[i] >= freqLow && 
                spectrumAnalysis.frequencies[i] < freqHigh) {
                energy += spectrumAnalysis.magnitudes[i] * spectrumAnalysis.magnitudes[i];
                count++;
            }
        }
        
        spectrumAnalysis.criticalBands.push_back(static_cast<float>(bark));
        spectrumAnalysis.bandEnergies.push_back(count > 0 ? energy / count : 0.0f);
    }
}

void AdvancedAudioAnalyzer::calculateOctaveBands() {
    // Calculate octave band levels
    spectrumAnalysis.octaveBands = {31.5f, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
    spectrumAnalysis.octaveLevels.clear();
    
    for (float centerFreq : spectrumAnalysis.octaveBands) {
        float freqLow = centerFreq / std::sqrt(2.0f);
        float freqHigh = centerFreq * std::sqrt(2.0f);
        
        float level = 0.0f;
        int count = 0;
        
        for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
            if (spectrumAnalysis.frequencies[i] >= freqLow && 
                spectrumAnalysis.frequencies[i] < freqHigh) {
                level += spectrumAnalysis.magnitudes[i];
                count++;
            }
        }
        
        spectrumAnalysis.octaveLevels.push_back(count > 0 ? level / count : 0.0f);
    }
}

void AdvancedAudioAnalyzer::calculateSpectralFeatures() {
    // Calculate spectral centroid
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    
    for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
        weightedSum += spectrumAnalysis.frequencies[i] * spectrumAnalysis.magnitudes[i];
        magnitudeSum += spectrumAnalysis.magnitudes[i];
    }
    
    spectrumAnalysis.spectralCentroid = magnitudeSum > 0.0f ? weightedSum / magnitudeSum : 0.0f;
    
    // Calculate spectral spread
    float variance = 0.0f;
    for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
        float deviation = spectrumAnalysis.frequencies[i] - spectrumAnalysis.spectralCentroid;
        variance += deviation * deviation * spectrumAnalysis.magnitudes[i];
    }
    
    spectrumAnalysis.spectralSpread = magnitudeSum > 0.0f ? std::sqrt(variance / magnitudeSum) : 0.0f;
    
    // Calculate spectral flux (placeholder)
    spectrumAnalysis.spectralFlux = 0.0f;
    
    // Calculate spectral rolloff
    float totalEnergy = 0.0f;
    for (float magnitude : spectrumAnalysis.magnitudes) {
        totalEnergy += magnitude * magnitude;
    }
    
    float rolloffEnergy = 0.85f * totalEnergy;
    float cumulativeEnergy = 0.0f;
    
    for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
        cumulativeEnergy += spectrumAnalysis.magnitudes[i] * spectrumAnalysis.magnitudes[i];
        if (cumulativeEnergy >= rolloffEnergy) {
            spectrumAnalysis.spectralRolloff = spectrumAnalysis.frequencies[i];
            break;
        }
    }
    
    // Calculate spectral flatness
    float geometricMean = 1.0f;
    float arithmeticMean = 0.0f;
    
    for (float magnitude : spectrumAnalysis.magnitudes) {
        if (magnitude > 0.0f) {
            geometricMean *= std::log(magnitude);
        }
        arithmeticMean += magnitude;
    }
    
    if (spectrumAnalysis.magnitudes.size() > 0) {
        geometricMean = std::exp(geometricMean / spectrumAnalysis.magnitudes.size());
        arithmeticMean /= spectrumAnalysis.magnitudes.size();
        spectrumAnalysis.spectralFlatness = arithmeticMean > 0.0f ? geometricMean / arithmeticMean : 0.0f;
    }
}

void AdvancedAudioAnalyzer::calculateHarmonics() {
    // Find fundamental frequency
    float fundamental = detectFundamental(magnitudeBuffer);
    spectrumAnalysis.fundamentalFrequency = fundamental;
    
    // Calculate harmonics
    spectrumAnalysis.harmonicFrequencies.clear();
    spectrumAnalysis.harmonicMagnitudes.clear();
    
    for (int harmonic = 1; harmonic <= 10; ++harmonic) {
        float harmonicFreq = fundamental * harmonic;
        
        // Find closest frequency bin
        size_t closestBin = 0;
        float minDiff = std::numeric_limits<float>::max();
        
        for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
            float diff = std::abs(spectrumAnalysis.frequencies[i] - harmonicFreq);
            if (diff < minDiff) {
                minDiff = diff;
                closestBin = i;
            }
        }
        
        if (minDiff < 50.0f) { // Within 50 Hz
            spectrumAnalysis.harmonicFrequencies.push_back(spectrumAnalysis.frequencies[closestBin]);
            spectrumAnalysis.harmonicMagnitudes.push_back(spectrumAnalysis.magnitudes[closestBin]);
        }
    }
    
    // Calculate harmonic ratio
    float harmonicEnergy = 0.0f;
    float totalEnergy = 0.0f;
    
    for (size_t i = 0; i < spectrumAnalysis.magnitudes.size(); ++i) {
        totalEnergy += spectrumAnalysis.magnitudes[i] * spectrumAnalysis.magnitudes[i];
    }
    
    for (float magnitude : spectrumAnalysis.harmonicMagnitudes) {
        harmonicEnergy += magnitude * magnitude;
    }
    
    spectrumAnalysis.harmonicRatio = totalEnergy > 0.0f ? harmonicEnergy / totalEnergy : 0.0f;
    
    // Calculate inharmonicity
    float inharmonicity = 0.0f;
    for (size_t i = 1; i < spectrumAnalysis.harmonicFrequencies.size(); ++i) {
        float expectedFreq = fundamental * (i + 1);
        float actualFreq = spectrumAnalysis.harmonicFrequencies[i];
        float deviation = std::abs(actualFreq - expectedFreq) / expectedFreq;
        inharmonicity += deviation;
    }
    
    spectrumAnalysis.inharmonicity = spectrumAnalysis.harmonicFrequencies.size() > 1 ? 
                                   inharmonicity / (spectrumAnalysis.harmonicFrequencies.size() - 1) : 0.0f;
}

void AdvancedAudioAnalyzer::calculateStereoPhase() {
    if (mainAudioBuffer.getNumChannels() < 2) {
        phaseAnalysis.stereoWidth = 0.0f;
        phaseAnalysis.phaseCorrelation = 1.0f;
        phaseAnalysis.midSideRatio = 0.0f;
        return;
    }
    
    // Calculate stereo width
    phaseAnalysis.stereoWidth = calculateStereoWidth(mainAudioBuffer);
    
    // Calculate phase correlation
    phaseAnalysis.phaseCorrelation = calculatePhaseCorrelation(mainAudioBuffer);
    
    // Calculate phase vectors
    phaseAnalysis.leftPhase = phaseBuffer;
    // Would calculate right phase separately
    
    // Calculate mid-side ratio
    float midLevel = 0.0f;
    float sideLevel = 0.0f;
    
    for (int i = 0; i < mainAudioBuffer.getNumSamples(); ++i) {
        float mid = (mainAudioBuffer.getSample(0, i) + mainAudioBuffer.getSample(1, i)) * 0.5f;
        float side = (mainAudioBuffer.getSample(0, i) - mainAudioBuffer.getSample(1, i)) * 0.5f;
        
        midLevel += mid * mid;
        sideLevel += side * side;
    }
    
    midLevel = std::sqrt(midLevel / mainAudioBuffer.getNumSamples());
    sideLevel = std::sqrt(sideLevel / mainAudioBuffer.getNumSamples());
    
    phaseAnalysis.midSideRatio = 20.0f * std::log10(sideLevel / (midLevel + 1e-10f));
}

void AdvancedAudioAnalyzer::calculatePhaseCoherence() {
    // Calculate phase coherence across frequency bands
    phaseAnalysis.phaseCoherence = 0.8f; // Placeholder
    
    phaseAnalysis.coherenceBands.clear();
    for (int band = 0; band < 10; ++band) {
        phaseAnalysis.coherenceBands.push_back(0.8f); // Placeholder
    }
}

void AdvancedAudioAnalyzer::calculateMonoCompatibility() {
    // Calculate mono compatibility
    phaseAnalysis.monoCompatibility = 1.0f; // Placeholder
    
    phaseAnalysis.monoCompatibilityBands.clear();
    for (int band = 0; band < 10; ++band) {
        phaseAnalysis.monoCompatibilityBands.push_back(1.0f); // Placeholder
    }
    
    // Detect phase issues
    phaseAnalysis.phaseIssues.clear();
    if (phaseAnalysis.phaseCorrelation < 0.5f) {
        phaseAnalysis.phaseIssues.push_back({1000.0f, 0.8f});
        phaseAnalysis.phaseProblemCount = 1;
    }
}

void AdvancedAudioAnalyzer::calculateEnvelope() {
    // Calculate attack and release envelopes
    int windowSize = 1024;
    int numWindows = mainAudioBuffer.getNumSamples() / windowSize;
    
    dynamicAnalysis.attackEnvelope.clear();
    dynamicAnalysis.releaseEnvelope.clear();
    
    for (int w = 0; w < numWindows; ++w) {
        float rms = 0.0f;
        
        for (int ch = 0; ch < mainAudioBuffer.getNumChannels(); ++ch) {
            for (int i = 0; i < windowSize; ++i) {
                float sample = mainAudioBuffer.getSample(ch, w * windowSize + i);
                rms += sample * sample;
            }
        }
        
        rms = std::sqrt(rms / (windowSize * mainAudioBuffer.getNumChannels()));
        
        dynamicAnalysis.attackEnvelope.push_back(rms);
        dynamicAnalysis.releaseEnvelope.push_back(rms);
    }
    
    // Calculate attack and release times
    dynamicAnalysis.attackTime = 10.0f; // Placeholder
    dynamicAnalysis.releaseTime = 100.0f; // Placeholder
}

void AdvancedAudioAnalyzer::calculateTransients() {
    // Detect transients
    dynamicAnalysis.transientPositions.clear();
    dynamicAnalysis.transientAmplitudes.clear();
    
    float threshold = 0.1f;
    int windowSize = 64;
    
    for (int i = windowSize; i < mainAudioBuffer.getNumSamples() - windowSize; ++i) {
        float before = 0.0f;
        float after = 0.0f;
        
        for (int j = 0; j < windowSize; ++j) {
            before += std::abs(mainAudioBuffer.getSample(0, i - windowSize + j));
            after += std::abs(mainAudioBuffer.getSample(0, i + j));
        }
        
        before /= windowSize;
        after /= windowSize;
        
        if (after - before > threshold) {
            dynamicAnalysis.transientPositions.push_back(i);
            dynamicAnalysis.transientAmplitudes.push_back(after - before);
        }
    }
    
    dynamicAnalysis.transientDensity = dynamicAnalysis.transientPositions.size() / 
                                     (mainAudioBuffer.getNumSamples() / currentSampleRate);
    dynamicAnalysis.transientStrength = dynamicAnalysis.transientAmplitudes.empty() ? 0.0f :
                                       *std::max_element(dynamicAnalysis.transientAmplitudes.begin(),
                                                       dynamicAnalysis.transientAmplitudes.end());
}

void AdvancedAudioAnalyzer::calculateCompression() {
    // Analyze compression characteristics
    dynamicAnalysis.compressionRatio = 1.0f; // No compression by default
    dynamicAnalysis.compressionThreshold = -60.0f;
    dynamicAnalysis.compressionKnee = 0.0f;
    dynamicAnalysis.compressionGain = 0.0f;
    
    // Calculate RMS history
    int windowSize = 512;
    int numWindows = mainAudioBuffer.getNumSamples() / windowSize;
    
    dynamicAnalysis.rmsHistory.clear();
    float totalRMS = 0.0f;
    
    for (int w = 0; w < numWindows; ++w) {
        float rms = 0.0f;
        
        for (int ch = 0; ch < mainAudioBuffer.getNumChannels(); ++ch) {
            for (int i = 0; i < windowSize; ++i) {
                float sample = mainAudioBuffer.getSample(ch, w * windowSize + i);
                rms += sample * sample;
            }
        }
        
        rms = std::sqrt(rms / (windowSize * mainAudioBuffer.getNumChannels()));
        rms = 20.0f * std::log10(rms + 1e-10f);
        
        dynamicAnalysis.rmsHistory.push_back(rms);
        totalRMS += rms;
    }
    
    dynamicAnalysis.averageRMS = totalRMS / numWindows;
    
    // Calculate RMS variation
    float variance = 0.0f;
    for (float rms : dynamicAnalysis.rmsHistory) {
        variance += (rms - dynamicAnalysis.averageRMS) * (rms - dynamicAnalysis.averageRMS);
    }
    dynamicAnalysis.rmsVariation = std::sqrt(variance / numWindows);
    
    // Calculate peak-to-RMS ratio
    float peak = calculatePeak(mainAudioBuffer);
    dynamicAnalysis.peakToRMSRatio = peak - dynamicAnalysis.averageRMS;
}

float AdvancedAudioAnalyzer::detectFundamental(const std::vector<float>& magnitude) {
    // Find the peak frequency
    int maxBin = 1;
    float maxMagnitude = magnitude[1];
    
    for (int i = 2; i < magnitude.size(); ++i) {
        if (magnitude[i] > maxMagnitude) {
            maxMagnitude = magnitude[i];
            maxBin = i;
        }
    }
    
    return maxBin * currentSampleRate / fft.getSize();
}

float AdvancedAudioAnalyzer::calculatePitchConfidence(const juce::AudioBuffer<float>& buffer) {
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

void AdvancedAudioAnalyzer::calculateChromagram() {
    // Calculate chromagram (12 pitch classes)
    pitchAnalysis.chromagram.clear();
    pitchAnalysis.chromagram.resize(12, 0.0f);
    
    // Map frequencies to pitch classes
    for (size_t i = 0; i < spectrumAnalysis.frequencies.size(); ++i) {
        float freq = spectrumAnalysis.frequencies[i];
        float magnitude = spectrumAnalysis.magnitudes[i];
        
        // Convert frequency to MIDI note number
        float midiNote = 69.0f + 12.0f * std::log2(freq / 440.0f);
        
        if (midiNote >= 0 && midiNote <= 127) {
            int pitchClass = static_cast<int>(midiNote) % 12;
            pitchAnalysis.chromagram[pitchClass] += magnitude;
        }
    }
    
    // Normalize
    float maxChroma = *std::max_element(pitchAnalysis.chromagram.begin(), 
                                       pitchAnalysis.chromagram.end());
    if (maxChroma > 0.0f) {
        for (float& chroma : pitchAnalysis.chromagram) {
            chroma /= maxChroma;
        }
    }
}

void AdvancedAudioAnalyzer::detectKey() {
    // Simple key detection based on chromagram
    std::vector<juce::String> majorKeys = {"C", "C#", "D", "D#", "E", "F", 
                                          "F#", "G", "G#", "A", "A#", "B"};
    std::vector<juce::String> minorKeys = {"C", "C#", "D", "D#", "E", "F", 
                                          "F#", "G", "G#", "A", "A#", "B"};
    
    // Calculate correlation with major and minor templates
    std::vector<float> majorCorrelations(12, 0.0f);
    std::vector<float> minorCorrelations(12, 0.0f);
    
    // Major template: [1,0,1,0,1,1,0,1,0,1,0,1]
    std::vector<float> majorTemplate = {1,0,1,0,1,1,0,1,0,1,0,1};
    
    // Minor template: [1,0,1,1,0,1,0,1,1,0,1,0]
    std::vector<float> minorTemplate = {1,0,1,1,0,1,0,1,1,0,1,0};
    
    for (int root = 0; root < 12; ++root) {
        for (int i = 0; i < 12; ++i) {
            int idx = (root + i) % 12;
            majorCorrelations[root] += pitchAnalysis.chromagram[idx] * majorTemplate[i];
            minorCorrelations[root] += pitchAnalysis.chromagram[idx] * minorTemplate[i];
        }
    }
    
    // Find best match
    float maxCorrelation = 0.0f;
    int bestKey = 0;
    bool isMinor = false;
    
    for (int i = 0; i < 12; ++i) {
        if (majorCorrelations[i] > maxCorrelation) {
            maxCorrelation = majorCorrelations[i];
            bestKey = i;
            isMinor = false;
        }
        if (minorCorrelations[i] > maxCorrelation) {
            maxCorrelation = minorCorrelations[i];
            bestKey = i;
            isMinor = true;
        }
    }
    
    pitchAnalysis.detectedKey = (isMinor ? minorKeys[bestKey] : majorKeys[bestKey]) + 
                               (isMinor ? " minor" : " major");
    pitchAnalysis.keyConfidence = maxCorrelation;
}

float AdvancedAudioAnalyzer::detectTempo() {
    // Simplified tempo detection using autocorrelation
    int minPeriod = static_cast<int>(60.0f / 200.0f * currentSampleRate); // 200 BPM max
    int maxPeriod = static_cast<int>(60.0f / 40.0f * currentSampleRate);  // 40 BPM min
    
    // Calculate autocorrelation
    std::vector<float> autocorr(maxPeriod);
    
    for (int lag = minPeriod; lag < maxPeriod; ++lag) {
        float correlation = 0.0f;
        
        for (int i = 0; i < mainAudioBuffer.getNumSamples() - lag; ++i) {
            correlation += mainAudioBuffer.getSample(0, i) * mainAudioBuffer.getSample(0, i + lag);
        }
        
        autocorr[lag] = correlation;
    }
    
    // Find peak
    int peakLag = minPeriod;
    float maxValue = autocorr[minPeriod];
    
    for (int lag = minPeriod; lag < maxPeriod; ++lag) {
        if (autocorr[lag] > maxValue) {
            maxValue = autocorr[lag];
            peakLag = lag;
        }
    }
    
    float tempo = 60.0f * currentSampleRate / peakLag;
    return juce::jlimit(40.0f, 200.0f, tempo);
}

void AdvancedAudioAnalyzer::detectBeats() {
    // Generate beat positions based on tempo
    float tempo = rhythmAnalysis.tempo;
    float beatInterval = 60.0f / tempo;
    
    rhythmAnalysis.beatPositions.clear();
    rhythmAnalysis.beatStrengths.clear();
    
    for (float t = 0.0f; t < mainAudioBuffer.getNumSamples() / currentSampleRate; t += beatInterval) {
        rhythmAnalysis.beatPositions.push_back(t);
        rhythmAnalysis.beatStrengths.push_back(0.8f); // Placeholder
    }
    
    rhythmAnalysis.beatStrength = 0.8f; // Placeholder
}

void AdvancedAudioAnalyzer::detectOnsets() {
    // Detect onsets using spectral flux
    rhythmAnalysis.onsets.clear();
    
    int windowSize = 512;
    int hopSize = 256;
    
    std::vector<float> prevSpectrum(windowSize / 2 + 1, 0.0f);
    
    for (int pos = 0; pos < mainAudioBuffer.getNumSamples() - windowSize; pos += hopSize) {
        // Extract window
        juce::AudioBuffer<float> window(mainAudioBuffer.getNumChannels(), windowSize);
        window.copyFrom(0, 0, mainAudioBuffer, 0, pos, windowSize);
        
        // Calculate spectrum
        performFFT(window);
        
        // Calculate spectral flux
        float flux = 0.0f;
        for (int i = 0; i < magnitudeBuffer.size(); ++i) {
            float diff = magnitudeBuffer[i] - prevSpectrum[i];
            if (diff > 0) {
                flux += diff;
            }
            prevSpectrum[i] = magnitudeBuffer[i];
        }
        
        // Detect onset
        if (flux > 0.5f) {
            float time = pos / currentSampleRate;
            rhythmAnalysis.onsets.push_back(time);
        }
    }
    
    rhythmAnalysis.onsetRate = rhythmAnalysis.onsets.size() / 
                             (mainAudioBuffer.getNumSamples() / currentSampleRate);
}

void AdvancedAudioAnalyzer::calculateMFCC() {
    // Calculate MFCC coefficients
    int numCoeffs = 13;
    pitchAnalysis.mfccCoefficients.resize(numCoeffs, 0.0f);
    
    // This is a simplified MFCC calculation
    // Real implementation would use mel filter banks and DCT
    
    for (int i = 0; i < numCoeffs; ++i) {
        pitchAnalysis.mfccCoefficients[i] = 0.1f * i; // Placeholder
    }
}

void AdvancedAudioAnalyzer::calculateTimbreDescriptors() {
    // Calculate timbre descriptors
    timbreAnalysis.brightness = juce::jmap(spectrumAnalysis.spectralCentroid, 
                                         200.0f, 8000.0f, 0.0f, 1.0f);
    timbreAnalysis.roughness = 0.3f; // Placeholder
    timbreAnalysis.warmth = juce::jmap(spectrumAnalysis.octaveLevels[1], // 63 Hz band
                                      -60.0f, 0.0f, 0.0f, 1.0f);
    timbreAnalysis.hardness = 0.4f; // Placeholder
    timbreAnalysis.depth = 0.5f; // Placeholder
}

void AdvancedAudioAnalyzer::classifyInstrument() {
    // Simple instrument classification
    std::vector<std::pair<juce::String, float>> probabilities;
    
    // Based on spectral characteristics
    if (spectrumAnalysis.spectralCentroid < 1000) {
        probabilities.push_back({"bass", 0.4f});
        probabilities.push_back({"kick", 0.3f});
        probabilities.push_back({"tom", 0.2f});
        probabilities.push_back({"synth", 0.1f});
    } else if (spectrumAnalysis.spectralCentroid < 3000) {
        probabilities.push_back({"vocal", 0.3f});
        probabilities.push_back({"guitar", 0.3f});
        probabilities.push_back({"piano", 0.2f});
        probabilities.push_back({"strings", 0.2f});
    } else {
        probabilities.push_back({"hihat", 0.3f});
        probabilities.push_back({"cymbal", 0.3f});
        probabilities.push_back({"snare", 0.2f});
        probabilities.push_back({"percussion", 0.2f});
    }
    
    timbreAnalysis.instrumentProbabilities = probabilities;
    timbreAnalysis.primaryInstrument = probabilities[0].first;
    timbreAnalysis.instrumentConfidence = probabilities[0].second;
}

void AdvancedAudioAnalyzer::assessClarity() {
    // Assess mix clarity
    float clarity = 1.0f;
    
    // Reduce clarity for muddy mixes
    if (spectrumAnalysis.octaveLevels[2] > spectrumAnalysis.octaveLevels[3] + 3.0f) {
        clarity -= 0.2f;
    }
    
    // Reduce clarity for harsh mixes
    if (spectrumAnalysis.octaveLevels[7] > spectrumAnalysis.octaveLevels[6] + 6.0f) {
        clarity -= 0.2f;
    }
    
    qualityAnalysis.clarity = juce::jlimit(0.0f, 1.0f, clarity);
}

void AdvancedAudioAnalyzer::assessPunch() {
    // Assess punch based on transients and dynamics
    float punch = 0.5f;
    
    // Increase punch for dynamic mixes
    if (dynamicAnalysis.dynamicRange > 12.0f) {
        punch += 0.3f;
    }
    
    // Increase punch for strong transients
    if (dynamicAnalysis.transientStrength > 0.5f) {
        punch += 0.2f;
    }
    
    qualityAnalysis.punch = juce::jlimit(0.0f, 1.0f, punch);
}

void AdvancedAudioAnalyzer::assessWarmth() {
    // Assess warmth based on bass and low-mid balance
    float warmth = juce::jmap(spectrumAnalysis.octaveLevels[1], -60.0f, 0.0f, 0.0f, 0.5f);
    warmth += juce::jmap(spectrumAnalysis.octaveLevels[2], -60.0f, 0.0f, 0.0f, 0.5f);
    
    qualityAnalysis.warmth = juce::jlimit(0.0f, 1.0f, warmth);
}

void AdvancedAudioAnalyzer::detectIssues() {
    qualityAnalysis.issues.clear();
    
    // Check for clipping
    if (loudnessAnalysis.truePeak > -0.1f) {
        QualityAnalysis::QualityIssue issue;
        issue.description = "Audio is clipping - peak level exceeds -0.1 dBFS";
        issue.severity = "critical";
        issue.category = "clipping";
        issue.confidence = 1.0f;
        qualityAnalysis.issues.push_back(issue);
    }
    
    // Check for low dynamic range
    if (dynamicAnalysis.dynamicRange < 6.0f) {
        QualityAnalysis::QualityIssue issue;
        issue.description = "Low dynamic range - mix may sound flat";
        issue.severity = "medium";
        issue.category = "dynamic";
        issue.confidence = 0.7f;
        qualityAnalysis.issues.push_back(issue);
    }
    
    // Check for phase issues
    if (phaseAnalysis.phaseCorrelation < 0.5f) {
        QualityAnalysis::QualityIssue issue;
        issue.description = "Phase correlation issues - potential mono compatibility problems";
        issue.severity = "high";
        issue.category = "phase";
        issue.confidence = 0.8f;
        qualityAnalysis.issues.push_back(issue);
    }
}

void AdvancedAudioAnalyzer::analyzeReferenceTrack() {
    // Analyze reference track
    analyzeLoudness(referenceBuffer, referenceAnalysis);
    
    // Analyze frequency balance
    performFFT(referenceBuffer);
    calculateSpectrum();
    referenceAnalysis = spectrumAnalysis;
    
    // Analyze stereo image
    analyzePhase();
    referenceAnalysis = phaseAnalysis;
}

float AdvancedAudioAnalyzer::calculateLoudnessDifference() {
    return std::abs(loudnessAnalysis.integratedLUFS - referenceAnalysis.overallLoudness);
}

float AdvancedAudioAnalyzer::calculateSpectralDifference() {
    float diff = 0.0f;
    
    for (size_t i = 0; i < spectrumAnalysis.octaveLevels.size(); ++i) {
        diff += std::abs(spectrumAnalysis.octaveLevels[i] - referenceAnalysis.octaveLevels[i]);
    }
    
    return diff / spectrumAnalysis.octaveLevels.size();
}

float AdvancedAudioAnalyzer::calculatePhaseDifference() {
    return std::abs(phaseAnalysis.stereoWidth - referenceAnalysis.stereoWidth);
}

float AdvancedAudioAnalyzer::calculateOverallSimilarity() {
    float loudnessSim = 1.0f - juce::jlimit(0.0f, 1.0f, calculateLoudnessDifference() / 10.0f);
    float spectrumSim = 1.0f - juce::jlimit(0.0f, 1.0f, calculateSpectralDifference() / 20.0f);
    float phaseSim = 1.0f - juce::jlimit(0.0f, 1.0f, calculatePhaseDifference() / 1.0f);
    
    return (loudnessSim + spectrumSim + phaseSim) / 3.0f;
}

// Helper methods
float AdvancedAudioAnalyzer::calculateLUFS(const juce::AudioBuffer<float>& buffer) {
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
    return -0.691f + 10.0f * std::log10(rms + 1e-10f);
}

float AdvancedAudioAnalyzer::calculateTruePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(buffer.getSample(ch, i));
            peak = juce::jmax(peak, sample);
        }
    }
    
    return 20.0f * std::log10(peak + 1e-10f);
}

float AdvancedAudioAnalyzer::calculateLRA(const juce::AudioBuffer<float>& buffer) {
    // Simplified loudness range calculation
    std::vector<float> shortTermLoudness;
    
    int windowSize = static_cast<int>(3.0f * currentSampleRate); // 3 second windows
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

float AdvancedAudioAnalyzer::calculateDynamicRange(const juce::AudioBuffer<float>& buffer) {
    // Calculate peak to RMS ratio
    float peak = calculatePeak(buffer);
    float rms = 20.0f * std::log10(calculateRMS(buffer) + 1e-10f);
    
    return peak - rms;
}

float AdvancedAudioAnalyzer::calculatePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(buffer.getSample(ch, i));
            peak = juce::jmax(peak, sample);
        }
    }
    
    return 20.0f * std::log10(peak + 1e-10f);
}

float AdvancedAudioAnalyzer::calculateRMS(const juce::AudioBuffer<float>& buffer) {
    float sum = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = buffer.getSample(ch, i);
            sum += sample * sample;
        }
    }
    
    return std::sqrt(sum / (buffer.getNumSamples() * buffer.getNumChannels()));
}

float AdvancedAudioAnalyzer::calculateStereoWidth(const juce::AudioBuffer<float>& buffer) {
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

float AdvancedAudioAnalyzer::calculatePhaseCorrelation(const juce::AudioBuffer<float>& buffer) {
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

std::vector<float> AdvancedAudioAnalyzer::calculateSpectrum(const juce::AudioBuffer<float>& buffer) {
    performFFT(buffer);
    return magnitudeBuffer;
}

float AdvancedAudioAnalyzer::calculateFrequencyCenter(const juce::AudioBuffer<float>& buffer) {
    performFFT(buffer);
    
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    float binWidth = currentSampleRate / fft.getSize();
    
    for (int i = 1; i < magnitudeBuffer.size(); ++i) {
        float frequency = i * binWidth;
        float magnitude = magnitudeBuffer[i];
        
        weightedSum += frequency * magnitude;
        magnitudeSum += magnitude;
    }
    
    return magnitudeSum > 0.0f ? weightedSum / magnitudeSum : 0.0f;
}

float AdvancedAudioAnalyzer::calculateMaskingIndex(const juce::AudioBuffer<float>& track1, 
                                                  const juce::AudioBuffer<float>& track2) {
    // Simplified masking calculation
    float track1RMS = calculateRMS(track1);
    float track2RMS = calculateRMS(track2);
    
    return track1RMS / (track1RMS + track2RMS + 1e-6f);
}

void AdvancedAudioAnalyzer::initializeFrequencyBins() {
    spectrumAnalysis.frequencies.clear();
    
    float binWidth = config.sampleRate / fft.getSize();
    for (int i = 0; i < fft.getSize() / 2 + 1; ++i) {
        float frequency = i * binWidth;
        if (frequency >= config.minFrequency && frequency <= config.maxFrequency) {
            spectrumAnalysis.frequencies.push_back(frequency);
        }
    }
}

void AdvancedAudioAnalyzer::initializeCriticalBands() {
    // Initialize Bark scale
    spectrumAnalysis.criticalBands.clear();
    for (int bark = 0; bark < 24; ++bark) {
        spectrumAnalysis.criticalBands.push_back(static_cast<float>(bark));
    }
}

void AdvancedAudioAnalyzer::initializeOctaveBands() {
    // Initialize octave bands
    spectrumAnalysis.octaveBands = {31.5f, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
}

float AdvancedAudioAnalyzer::hzToBark(float hz) const {
    return 13.0f * std::atan(0.00076f * hz) + 3.5f * std::atan((hz / 7500.0f) * (hz / 7500.0f));
}

float AdvancedAudioAnalyzer::barkToHz(float bark) const {
    // Inverse of Bark scale (approximation)
    return 600.0f * std::sinh(bark / 4.0f);
}

float AdvancedAudioAnalyzer::hzToMel(float hz) const {
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

float AdvancedAudioAnalyzer::melToHz(float mel) const {
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

float AdvancedAudioAnalyzer::hzToCent(float hz) const {
    return 1200.0f * std::log2(hz / 440.0f);
}

void AdvancedAudioAnalyzer::notifyAnalysisComplete(AnalysisType type) {
    for (auto* listener : listeners) {
        listener->analysisComplete(type);
    }
}

void AdvancedAudioAnalyzer::notifyLoudnessUpdated(const LoudnessAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->loudnessUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifySpectrumUpdated(const SpectrumAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->spectrumUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifyPhaseUpdated(const PhaseAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->phaseUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifyDynamicsUpdated(const DynamicAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->dynamicsUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifyPitchUpdated(const PitchAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->pitchUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifyRhythmUpdated(const RhythmAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->rhythmUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifyTimbreUpdated(const TimbreAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->timbreUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifyQualityUpdated(const QualityAnalysis& analysis) {
    for (auto* listener : listeners) {
        listener->qualityUpdated(analysis);
    }
}

void AdvancedAudioAnalyzer::notifyReferenceCompared(float similarity) {
    for (auto* listener : listeners) {
        listener->referenceCompared(similarity);
    }
}

} // namespace analysis
} // namespace zenith
