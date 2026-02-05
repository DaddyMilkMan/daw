/*
  ==============================================================================
    GenreDetector.h
    Advanced genre detection from audio content analysis
    Phase 2: Context-Aware AI (9/10)
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "VisualAnalyzer.h"
#include <memory>
#include <vector>

namespace zenith {
namespace ai {

struct GenreFeatures {
    // Spectral features
    float spectralCentroidMean = 0.0f;
    float spectralCentroidStd = 0.0f;
    float spectralRolloffMean = 0.0f;
    float spectralFluxMean = 0.0f;
    float mfccMean[13] = {0};  // Mel-frequency cepstral coefficients
    
    // Temporal features
    float tempo = 0.0f;
    float attackTimeMean = 0.0f;
    float decayTimeMean = 0.0f;
    float rmsMean = 0.0f;
    float rmsStd = 0.0f;
    float zeroCrossingRateMean = 0.0f;
    
    // Rhythmic features
    float rhythmicRegularity = 0.0f;
    float swingRatio = 0.0f;
    float grooveStrength = 0.0f;
    
    // Harmonic features
    float keyClarity = 0.0f;
    float harmonicComplexity = 0.0f;
    float chordChanges = 0.0f;
    
    // Dynamic features
    float dynamicRange = 0.0f;
    float crestFactorMean = 0.0f;
    float compressionRatio = 0.0f;
};

struct GenrePrediction {
    juce::String genre;
    float confidence;           // 0.0 to 1.0
    juce::String subgenre;      // More specific classification
    juce::var supportingFeatures;  // Which features contributed most
    juce::String reasoning;     // AI explanation

    bool operator==(const GenrePrediction& other) const {
        return genre == other.genre && subgenre == other.subgenre;
    }
    bool operator!=(const GenrePrediction& other) const {
        return !(*this == other);
    }
};

class GenreDetector {
public:
    GenreDetector();
    ~GenreDetector();

    // Main detection methods
    GenrePrediction detectGenre(const juce::AudioBuffer<float>& audio, 
                               double sampleRate);
    std::vector<GenrePrediction> detectMultipleGenres(const juce::AudioBuffer<float>& audio,
                                                      double sampleRate);

    // Feature extraction
    GenreFeatures extractFeatures(const juce::AudioBuffer<float>& audio,
                                 double sampleRate);

    // Genre-specific analysis
    bool isElectronic(const GenreFeatures& features) const;
    bool isRock(const GenreFeatures& features) const;
    bool isHipHop(const GenreFeatures& features) const;
    bool isClassical(const GenreFeatures& features) const;
    bool isJazz(const GenreFeatures& features) const;
    bool isPop(const GenreFeatures& features) const;

    // Subgenre detection
    juce::String detectElectronicSubgenre(const GenreFeatures& features) const;
    juce::String detectRockSubgenre(const GenreFeatures& features) const;
    juce::String detectHipHopSubgenre(const GenreFeatures& features) const;

    // Configuration
    void setAnalysisParameters(int fftSize = 2048, int hopSize = 512);
    void setGenreDatabase(const juce::File& databasePath);

private:
    std::unique_ptr<VisualAnalyzer> visualAnalyzer;
    int fftSize = 2048;
    int hopSize = 512;
    
    // Feature extraction helpers
    std::vector<float> computeMFCC(const juce::AudioBuffer<float>& audio, double sampleRate);
    float computeSpectralRolloff(const std::vector<float>& spectrum, float sampleRate, float threshold = 0.85f);
    float computeSpectralFlux(const std::vector<std::vector<float>>& spectrogram);
    float estimateTempo(const juce::AudioBuffer<float>& audio, double sampleRate);
    float computeRhythmicRegularity(const std::vector<float>& onsetTimes);
    float computeKeyClarity(const juce::AudioBuffer<float>& audio, double sampleRate);
    
    // Genre classification logic
    float calculateGenreProbability(const GenreFeatures& features, const juce::String& genre) const;
    std::vector<juce::String> getTopGenres(const GenreFeatures& features, int topN = 3) const;
    
    // Reasoning generation
    juce::String generateGenreReasoning(const GenreFeatures& features, const juce::String& genre) const;
    
    // Machine learning models (simplified for this implementation)
    std::unordered_map<juce::String, GenreFeatures> genrePrototypes;
    void initializeGenrePrototypes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenreDetector)
};

} // namespace ai
} // namespace zenith
