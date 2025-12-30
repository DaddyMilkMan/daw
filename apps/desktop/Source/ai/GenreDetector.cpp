/*
  ==============================================================================
    GenreDetector.cpp
    Advanced genre detection implementation
  ==============================================================================
*/

#include "GenreDetector.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <unordered_map>

namespace zenith {
namespace ai {

GenreDetector::GenreDetector() {
    visualAnalyzer = std::make_unique<VisualAnalyzer>();
    initializeGenrePrototypes();
}

GenreDetector::~GenreDetector() = default;

GenrePrediction GenreDetector::detectGenre(const juce::AudioBuffer<float>& audio, 
                                          double sampleRate) {
    GenrePrediction prediction;
    
    // Extract features
    GenreFeatures features = extractFeatures(audio, sampleRate);
    
    // Get top genre predictions
    auto topGenres = getTopGenres(features, 1);
    
    if (!topGenres.empty()) {
        prediction.genre = topGenres[0];
        prediction.confidence = calculateGenreProbability(features, topGenres[0]);
        
        // Detect subgenre
        if (prediction.genre == "electronic") {
            prediction.subgenre = detectElectronicSubgenre(features);
        } else if (prediction.genre == "rock") {
            prediction.subgenre = detectRockSubgenre(features);
        } else if (prediction.genre == "hip-hop") {
            prediction.subgenre = detectHipHopSubgenre(features);
        }
        
        // Generate reasoning
        prediction.reasoning = generateGenreReasoning(features, prediction.genre);
        
        // Extract supporting features
        auto supporting = new juce::DynamicObject();
        supporting->setProperty("spectralCentroid", features.spectralCentroidMean);
        supporting->setProperty("tempo", features.tempo);
        supporting->setProperty("attackTime", features.attackTimeMean);
        supporting->setProperty("dynamicRange", features.dynamicRange);
        prediction.supportingFeatures = juce::var(supporting);
    } else {
        prediction.genre = "unknown";
        prediction.confidence = 0.0f;
        prediction.reasoning = "Unable to determine genre from audio features";
    }
    
    return prediction;
}

std::vector<GenrePrediction> GenreDetector::detectMultipleGenres(const juce::AudioBuffer<float>& audio,
                                                               double sampleRate) {
    std::vector<GenrePrediction> predictions;
    
    // Extract features once
    GenreFeatures features = extractFeatures(audio, sampleRate);
    
    // Get top genres
    auto topGenres = getTopGenres(features, 5);
    
    for (const auto& genre : topGenres) {
        GenrePrediction prediction;
        prediction.genre = genre;
        prediction.confidence = calculateGenreProbability(features, genre);
        prediction.reasoning = generateGenreReasoning(features, genre);
        
        // Add subgenre for top prediction
        if (predictions.empty()) {
            if (genre == "electronic") {
                prediction.subgenre = detectElectronicSubgenre(features);
            } else if (genre == "rock") {
                prediction.subgenre = detectRockSubgenre(features);
            } else if (genre == "hip-hop") {
                prediction.subgenre = detectHipHopSubgenre(features);
            }
        }
        
        predictions.push_back(prediction);
    }
    
    return predictions;
}

GenreFeatures GenreDetector::extractFeatures(const juce::AudioBuffer<float>& audio,
                                            double sampleRate) {
    GenreFeatures features;
    
    // Get visual analysis
    auto visualResult = visualAnalyzer->analyzeAudio(audio, sampleRate);
    
    // Spectral features - use direct struct access
    features.spectralCentroidMean = visualResult.spectral.spectralCentroid;
    
    // Calculate spectral rolloff
    auto spectrum = visualAnalyzer->computeSpectrum(audio.getReadPointer(0), audio.getNumSamples());
    features.spectralRolloffMean = computeSpectralRolloff(spectrum, static_cast<float>(sampleRate));
    
    // Temporal features
    features.attackTimeMean = visualResult.waveform.attackTime;
    features.decayTimeMean = visualResult.waveform.decayTime;
    features.crestFactorMean = visualResult.waveform.crestFactor;
    
    // Calculate RMS statistics
    float rmsSum = 0.0f, rmsSumSq = 0.0f;
    int rmsCount = 0;
    
    for (float rms : visualResult.waveform.rmsEnvelope) {
        rmsSum += rms;
        rmsSumSq += rms * rms;
        rmsCount++;
    }
    
    if (rmsCount > 0) {
        features.rmsMean = rmsSum / rmsCount;
        features.rmsStd = std::sqrt((rmsSumSq / rmsCount) - (features.rmsMean * features.rmsMean));
    }
    
    // Zero crossing rate
    float zcrSum = 0.0f;
    for (float zcr : visualResult.waveform.zeroCrossings) {
        zcrSum += zcr;
    }
    features.zeroCrossingRateMean = zcrSum / visualResult.waveform.zeroCrossings.size();
    
    // Tempo estimation
    features.tempo = estimateTempo(audio, sampleRate);
    
    // Dynamic range
    features.dynamicRange = visualResult.waveform.dynamicRange;
    
    // MFCCs (simplified implementation)
    auto mfccs = computeMFCC(audio, sampleRate);
    for (int i = 0; i < 13 && i < mfccs.size(); ++i) {
        features.mfccMean[i] = mfccs[i];
    }
    
    // Rhythmic features
    features.rhythmicRegularity = computeRhythmicRegularity(visualResult.waveform.peaks);
    
    // Harmonic features
    features.keyClarity = computeKeyClarity(audio, sampleRate);
    
    return features;
}

bool GenreDetector::isElectronic(const GenreFeatures& features) const {
    // Electronic music characteristics:
    // - Higher spectral centroid (brighter)
    // - Strong rhythmic regularity
    // - Often 120-140 BPM range
    // - Lower harmonic complexity
    
    bool spectral = features.spectralCentroidMean > 2000.0f;
    bool rhythmic = features.rhythmicRegularity > 0.7f;
    bool tempo = (features.tempo >= 110.0f && features.tempo <= 150.0f);
    bool harmonic = features.harmonicComplexity < 0.5f;
    
    return spectral && rhythmic && (tempo || harmonic);
}

bool GenreDetector::isRock(const GenreFeatures& features) const {
    // Rock characteristics:
    // - Medium spectral centroid
    // - Strong attack times (drums)
    // - Higher dynamic range
    // - Medium harmonic complexity
    
    bool spectral = (features.spectralCentroidMean >= 1500.0f && features.spectralCentroidMean <= 3000.0f);
    bool attack = features.attackTimeMean < 0.05f;
    bool dynamic = features.dynamicRange > 3.0f;
    bool harmonic = features.harmonicComplexity > 0.3f && features.harmonicComplexity < 0.8f;
    
    return spectral && attack && dynamic && harmonic;
}

bool GenreDetector::isHipHop(const GenreFeatures& features) const {
    // Hip-hop characteristics:
    // - Lower to medium spectral centroid
    // - Strong rhythmic elements
    // - Often 80-110 BPM
    // - Emphasis on rhythm over harmony
    
    bool spectral = features.spectralCentroidMean < 2500.0f;
    bool rhythmic = features.rhythmicRegularity > 0.6f;
    bool tempo = (features.tempo >= 70.0f && features.tempo <= 120.0f);
    bool harmonic = features.harmonicComplexity < 0.6f;
    
    return spectral && rhythmic && tempo && harmonic;
}

bool GenreDetector::isClassical(const GenreFeatures& features) const {
    // Classical characteristics:
    // - Wide dynamic range
    // - High harmonic complexity
    // - Variable tempo
    // - Lower compression
    
    bool dynamic = features.dynamicRange > 5.0f;
    bool harmonic = features.harmonicComplexity > 0.7f;
    bool compression = features.compressionRatio < 2.0f;
    bool spectral = features.spectralCentroidMean > 1000.0f;
    
    return dynamic && harmonic && compression && spectral;
}

bool GenreDetector::isJazz(const GenreFeatures& features) const {
    // Jazz characteristics:
    // - High harmonic complexity
    // - Medium dynamic range
    // - Swing rhythm
    // - Moderate spectral content
    
    bool harmonic = features.harmonicComplexity > 0.6f;
    bool dynamic = (features.dynamicRange >= 2.0f && features.dynamicRange <= 6.0f);
    bool swing = features.swingRatio > 0.6f;
    bool spectral = (features.spectralCentroidMean >= 1500.0f && features.spectralCentroidMean <= 3500.0f);
    
    return harmonic && dynamic && swing && spectral;
}

bool GenreDetector::isPop(const GenreFeatures& features) const {
    // Pop characteristics:
    // - Medium everything
    // - Compressed dynamics
    // - Clear structure
    // - Moderate tempo
    
    bool spectral = (features.spectralCentroidMean >= 1500.0f && features.spectralCentroidMean <= 3000.0f);
    bool dynamic = features.dynamicRange < 4.0f;
    bool tempo = (features.tempo >= 90.0f && features.tempo <= 140.0f);
    bool harmonic = (features.harmonicComplexity >= 0.3f && features.harmonicComplexity <= 0.7f);
    
    return spectral && dynamic && tempo && harmonic;
}

juce::String GenreDetector::detectElectronicSubgenre(const GenreFeatures& features) const {
    if (features.tempo > 140.0f) return "drum-and-bass";
    if (features.tempo > 128.0f) return "techno";
    if (features.tempo > 120.0f) return "house";
    if (features.spectralCentroidMean > 3000.0f) return "trance";
    if (features.harmonicComplexity < 0.3f) return "minimal";
    return "electronic";
}

juce::String GenreDetector::detectRockSubgenre(const GenreFeatures& features) const {
    if (features.dynamicRange > 6.0f) return "progressive-rock";
    if (features.attackTimeMean < 0.01f) return "punk";
    if (features.spectralCentroidMean > 2500.0f) return "metal";
    if (features.tempo < 100.0f) return "blues-rock";
    return "rock";
}

juce::String GenreDetector::detectHipHopSubgenre(const GenreFeatures& features) const {
    if (features.tempo < 90.0f) return "boom-bap";
    if (features.spectralCentroidMean > 2000.0f) return "trap";
    if (features.swingRatio > 0.8f) return "old-school";
    return "hip-hop";
}

float GenreDetector::calculateGenreProbability(const GenreFeatures& features, const juce::String& genre) const {
    // Simplified probability calculation based on feature matching
    float probability = 0.0f;
    
    if (genre == "electronic" && isElectronic(features)) probability = 0.85f;
    else if (genre == "rock" && isRock(features)) probability = 0.85f;
    else if (genre == "hip-hop" && isHipHop(features)) probability = 0.85f;
    else if (genre == "classical" && isClassical(features)) probability = 0.85f;
    else if (genre == "jazz" && isJazz(features)) probability = 0.85f;
    else if (genre == "pop" && isPop(features)) probability = 0.85f;
    else probability = 0.3f;  // Low confidence for non-matching genres
    
    return probability;
}

std::vector<juce::String> GenreDetector::getTopGenres(const GenreFeatures& features, int topN) const {
    std::vector<std::pair<juce::String, float>> genreScores;
    
    // Calculate scores for all genres
    std::vector<juce::String> allGenres = {"electronic", "rock", "hip-hop", "classical", "jazz", "pop"};
    
    for (const auto& genre : allGenres) {
        float score = calculateGenreProbability(features, genre);
        genreScores.push_back({genre, score});
    }
    
    // Sort by score
    std::sort(genreScores.begin(), genreScores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Extract top N
    std::vector<juce::String> result;
    for (int i = 0; i < topN && i < genreScores.size(); ++i) {
        if (genreScores[i].second > 0.3f) {  // Minimum threshold
            result.push_back(genreScores[i].first);
        }
    }
    
    return result;
}

juce::String GenreDetector::generateGenreReasoning(const GenreFeatures& features, const juce::String& genre) const {
    juce::String reasoning = "Detected as " + genre + " based on:\n";
    
    if (genre == "electronic") {
        reasoning += "- Bright spectral content (centroid: " + juce::String(features.spectralCentroidMean, 0) + " Hz)\n";
        reasoning += "- Strong rhythmic regularity (" + juce::String(features.rhythmicRegularity, 2) + ")\n";
        if (features.tempo >= 110.0f && features.tempo <= 150.0f) {
            reasoning += "- Tempo in electronic range (" + juce::String(features.tempo, 0) + " BPM)\n";
        }
    } else if (genre == "rock") {
        reasoning += "- Balanced spectral content\n";
        reasoning += "- Fast attack transients (" + juce::String(features.attackTimeMean * 1000, 0) + " ms)\n";
        reasoning += "- High dynamic range (" + juce::String(features.dynamicRange, 1) + ")\n";
    } else if (genre == "hip-hop") {
        reasoning += "- Lower-mid frequency emphasis\n";
        reasoning += "- Rhythmic foundation (" + juce::String(features.tempo, 0) + " BPM)\n";
        reasoning += "- Emphasis on rhythm over harmony\n";
    }
    
    return reasoning;
}

// Helper methods (simplified implementations)
std::vector<float> GenreDetector::computeMFCC(const juce::AudioBuffer<float>& audio, double sampleRate) {
    // Simplified MFCC calculation - in practice would use proper filter banks
    std::vector<float> mfccs(13, 0.0f);
    
    // This is a placeholder - real MFCC calculation is complex
    // For now, return some dummy values based on spectral characteristics
    auto spectrum = visualAnalyzer->computeSpectrum(audio.getReadPointer(0), audio.getNumSamples());
    
    if (!spectrum.empty()) {
        // First MFCC (energy)
        float energy = 0.0f;
        for (float magnitude : spectrum) {
            energy += magnitude * magnitude;
        }
        mfccs[0] = std::log(energy + 1e-10f);
        
        // Other coefficients would be computed via DCT of filter bank energies
        // For now, use simplified approximations
        for (int i = 1; i < 13; ++i) {
            mfccs[i] = std::sin(i * 0.5f) * mfccs[0] * 0.1f;
        }
    }
    
    return mfccs;
}

float GenreDetector::computeSpectralRolloff(const std::vector<float>& spectrum, float sampleRate, float threshold) {
    float totalEnergy = 0.0f;
    for (float magnitude : spectrum) {
        totalEnergy += magnitude * magnitude;
    }
    
    float targetEnergy = totalEnergy * threshold;
    float accumulatedEnergy = 0.0f;
    
    for (size_t i = 0; i < spectrum.size(); ++i) {
        accumulatedEnergy += spectrum[i] * spectrum[i];
        if (accumulatedEnergy >= targetEnergy) {
            return static_cast<float>(i) * sampleRate / (2.0f * spectrum.size());
        }
    }
    
    return sampleRate / 2.0f;  // Nyquist frequency
}

float GenreDetector::estimateTempo(const juce::AudioBuffer<float>& audio, double sampleRate) {
    // Simplified tempo estimation using onset detection
    std::vector<float> onsets;
    
    // Detect onsets from energy changes
    const int windowSize = 1024;
    const int hopSize = 512;
    
    for (int i = hopSize; i < audio.getNumSamples() - windowSize; i += hopSize) {
        float energy1 = 0.0f, energy2 = 0.0f;
        
        for (int j = 0; j < windowSize; ++j) {
            energy1 += audio.getSample(0, i - hopSize + j) * audio.getSample(0, i - hopSize + j);
            energy2 += audio.getSample(0, i + j) * audio.getSample(0, i + j);
        }
        
        if (energy2 > energy1 * 1.5f) {
            onsets.push_back(static_cast<float>(i) / sampleRate);
        }
    }
    
    if (onsets.size() < 2) return 120.0f;  // Default tempo
    
    // Calculate intervals between onsets
    std::vector<float> intervals;
    for (size_t i = 1; i < onsets.size(); ++i) {
        intervals.push_back(onsets[i] - onsets[i-1]);
    }
    
    // Find most common interval (simplified)
    if (intervals.empty()) return 120.0f;
    
    float avgInterval = 0.0f;
    for (float interval : intervals) {
        avgInterval += interval;
    }
    avgInterval /= intervals.size();
    
    // Convert to BPM
    float bpm = 60.0f / avgInterval;
    
    // Clamp to reasonable range
    if (bpm < 60.0f) bpm *= 2.0f;
    if (bpm > 200.0f) bpm /= 2.0f;
    
    return bpm;
}

float GenreDetector::computeRhythmicRegularity(const std::vector<float>& onsetTimes) {
    if (onsetTimes.size() < 3) return 0.0f;
    
    // Calculate intervals between onsets
    std::vector<float> intervals;
    for (size_t i = 1; i < onsetTimes.size(); ++i) {
        intervals.push_back(onsetTimes[i] - onsetTimes[i-1]);
    }
    
    if (intervals.empty()) return 0.0f;
    
    // Calculate standard deviation of intervals
    float mean = 0.0f;
    for (float interval : intervals) {
        mean += interval;
    }
    mean /= intervals.size();
    
    float variance = 0.0f;
    for (float interval : intervals) {
        variance += (interval - mean) * (interval - mean);
    }
    variance /= intervals.size();
    
    float stdDev = std::sqrt(variance);
    
    // Regularity is inverse of normalized standard deviation
    return std::max(0.0f, 1.0f - (stdDev / mean));
}

float GenreDetector::computeKeyClarity(const juce::AudioBuffer<float>& audio, double sampleRate) {
    // Simplified key clarity measurement
    // In practice would use pitch detection and harmonic analysis
    
    // For now, return a value based on harmonic content
    auto spectrum = visualAnalyzer->computeSpectrum(audio.getReadPointer(0), audio.getNumSamples());
    
    if (spectrum.empty()) return 0.0f;
    
    // Look for harmonic series
    float harmonicStrength = 0.0f;
    float totalStrength = 0.0f;
    
    for (size_t i = 1; i < spectrum.size() / 4; ++i) {  // Check lower frequencies
        float fundamental = spectrum[i];
        float harmonic2 = spectrum[i * 2];
        float harmonic3 = spectrum[i * 3];
        
        if (i * 3 < spectrum.size()) {
            harmonicStrength += fundamental + harmonic2 + harmonic3;
        }
        totalStrength += fundamental;
    }
    
    return totalStrength > 0.0f ? harmonicStrength / (totalStrength * 3.0f) : 0.0f;
}

void GenreDetector::initializeGenrePrototypes() {
    // Initialize genre feature prototypes for comparison
    // In practice, these would be learned from training data
    
    GenreFeatures electronic;
    electronic.spectralCentroidMean = 2500.0f;
    electronic.rhythmicRegularity = 0.8f;
    electronic.tempo = 128.0f;
    electronic.harmonicComplexity = 0.3f;
    genrePrototypes["electronic"] = electronic;
    
    GenreFeatures rock;
    rock.spectralCentroidMean = 2000.0f;
    rock.attackTimeMean = 0.02f;
    rock.dynamicRange = 4.0f;
    rock.harmonicComplexity = 0.5f;
    genrePrototypes["rock"] = rock;
    
    // Add more genre prototypes as needed...
}

} // namespace ai
} // namespace zenith
