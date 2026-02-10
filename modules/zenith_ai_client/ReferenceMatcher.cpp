/*
  ==============================================================================
    ReferenceMatcher.cpp
    Implementation of reference track matching system
  ==============================================================================
*/

#include "ReferenceMatcher.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace ai {

ReferenceMatcher::ReferenceMatcher() 
    : fft(fftOrder), 
      window(fftOrder, juce::dsp::WindowingFunction<float>::hann) {
}

ReferenceMatcher::~ReferenceMatcher() = default;

ReferenceFingerprint ReferenceMatcher::analyzeReference(const juce::AudioBuffer<float>& audio,
                                                       double sampleRate,
                                                       const juce::String& trackName,
                                                       const juce::String& genre,
                                                       const juce::String& mood) {
    ReferenceFingerprint fingerprint;
    fingerprint.trackName = trackName;
    fingerprint.genre = genre;
    fingerprint.mood = mood;

    // Limit analysis to first 30 seconds for efficiency
    int analysisSamples = static_cast<int>(std::min(analysisDuration, 
                                                  static_cast<float>(audio.getNumSamples() / sampleRate)) * sampleRate);
    
    if (analysisSamples <= 0) {
        DBG("No audio samples to analyze");
        return fingerprint;
    }

    // Create analysis buffer (mono for efficiency)
    juce::AudioBuffer<float> analysisBuffer(1, analysisSamples);
    
    if (audio.getNumChannels() >= 2) {
        // Mix to mono
        analysisBuffer.copyFrom(0, 0, audio, 0, 0, analysisSamples);
        analysisBuffer.addFrom(0, 0, audio, 1, 0, analysisSamples);
        analysisBuffer.applyGain(0.5f);
    } else {
        analysisBuffer.copyFrom(0, 0, audio, 0, 0, analysisSamples);
    }

    // Extract features
    fingerprint.spectralCentroid = extractSpectralCentroid(analysisBuffer, sampleRate);
    fingerprint.rmsProfile = extractRMSProfile(analysisBuffer);
    fingerprint.stereoWidth = extractStereoWidth(audio);  // Use original stereo for width

    DBG("Analyzed reference: " << trackName << " - " << fingerprint.spectralCentroid.size() << " spectral points");

    return fingerprint;
}

std::vector<ReferenceFingerprint> ReferenceMatcher::findSimilarTracks(
    const juce::AudioBuffer<float>& queryAudio,
    double sampleRate,
    GrokJourney& journey,
    int maxResults) {
    
    // Create fingerprint for query
    auto queryFingerprint = analyzeReference(queryAudio, sampleRate, "query");
    
    // Get all stored fingerprints (in real implementation, this would be a database query)
    std::vector<ReferenceFingerprint> allFingerprints;
    // Note: This would need to be implemented in GrokJourney to retrieve fingerprints
    
    // Calculate similarities and sort
    std::vector<std::pair<double, ReferenceFingerprint>> similarities;
    
    for (const auto& fp : allFingerprints) {
        double similarity = compareFingerprints(queryFingerprint, fp);
        similarities.emplace_back(similarity, fp);
    }
    
    // Sort by similarity (highest first)
    std::sort(similarities.begin(), similarities.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top results
    std::vector<ReferenceFingerprint> results;
    for (int i = 0; i < std::min(maxResults, static_cast<int>(similarities.size())); ++i) {
        if (similarities[i].first > 0.3) {  // Minimum similarity threshold
            results.push_back(similarities[i].second);
        }
    }
    
    return results;
}

juce::var ReferenceMatcher::extractTargetSettings(const ReferenceFingerprint& fingerprint) {
    juce::DynamicObject::Ptr settings = new juce::DynamicObject();
    
    // Analyze spectral characteristics to suggest EQ settings
    if (!fingerprint.spectralCentroid.empty()) {
        float avgCentroid = 0.0f;
        for (float centroid : fingerprint.spectralCentroid) {
            avgCentroid += centroid;
        }
        avgCentroid /= fingerprint.spectralCentroid.size();
        
        // Map centroid to brightness
        if (avgCentroid < 1000.0f) {
            // Dark sound - boost highs
            settings->setProperty("eq_high_shelf_gain", 2.0);
            settings->setProperty("eq_presence_gain", 1.5);
        } else if (avgCentroid > 3000.0f) {
            // Bright sound - reduce harsh highs
            settings->setProperty("eq_high_shelf_gain", -1.0);
            settings->setProperty("eq_air_gain", -0.5);
        }
    }
    
    // Analyze RMS profile for compression settings
    if (!fingerprint.rmsProfile.empty()) {
        float avgRMS = 0.0f;
        float maxRMS = 0.0f;
        for (float rms : fingerprint.rmsProfile) {
            avgRMS += rms;
            maxRMS = std::max(maxRMS, rms);
        }
        avgRMS /= fingerprint.rmsProfile.size();
        
        float dynamicRange = maxRMS - avgRMS;
        
        // More compression for less dynamic tracks
        if (dynamicRange < 6.0f) {
            settings->setProperty("compression_ratio", 4.0);
            settings->setProperty("compression_threshold", -18.0);
        } else {
            settings->setProperty("compression_ratio", 2.0);
            settings->setProperty("compression_threshold", -12.0);
        }
    }
    
    // Analyze stereo width
    if (!fingerprint.stereoWidth.empty()) {
        float avgWidth = 0.0f;
        for (float width : fingerprint.stereoWidth) {
            avgWidth += width;
        }
        avgWidth /= fingerprint.stereoWidth.size();
        
        if (avgWidth < 0.3f) {
            // Narrow - suggest widening
            settings->setProperty("stereo_width_enhancement", 1.2);
        } else if (avgWidth > 0.8f) {
            // Very wide - suggest mono compatibility check
            settings->setProperty("mono_compatibility_check", true);
        }
    }
    
    return juce::var(settings);
}

double ReferenceMatcher::compareFingerprints(const ReferenceFingerprint& a, const ReferenceFingerprint& b) {
    double spectralSimilarity = 0.0;
    double rmsSimilarity = 0.0;
    double stereoSimilarity = 0.0;
    
    // Compare spectral centroids
    if (!a.spectralCentroid.empty() && !b.spectralCentroid.empty()) {
        spectralSimilarity = calculateSimilarity(a.spectralCentroid, b.spectralCentroid);
    }
    
    // Compare RMS profiles
    if (!a.rmsProfile.empty() && !b.rmsProfile.empty()) {
        rmsSimilarity = calculateSimilarity(a.rmsProfile, b.rmsProfile);
    }
    
    // Compare stereo width
    if (!a.stereoWidth.empty() && !b.stereoWidth.empty()) {
        stereoSimilarity = calculateSimilarity(a.stereoWidth, b.stereoWidth);
    }
    
    // Weighted average (spectral is most important)
    return (spectralSimilarity * 0.5 + rmsSimilarity * 0.3 + stereoSimilarity * 0.2);
}

std::vector<float> ReferenceMatcher::extractSpectralCentroid(const juce::AudioBuffer<float>& audio, double sampleRate) {
    std::vector<float> centroids;
    const float* data = audio.getReadPointer(0);
    int numSamples = audio.getNumSamples();
    
    // Process in overlapping windows
    const int hopSize = fftOrder / 2;  // 50% overlap
    
    for (int pos = 0; pos < numSamples - fftOrder; pos += hopSize) {
        std::vector<float> fftData(fftOrder * 2);
        std::vector<float> magnitudes(fftOrder / 2);
        
        // Copy window and apply windowing
        std::copy(data + pos, data + pos + fftOrder, fftData.begin());
        window.multiplyWithWindowingTable(fftData.data(), fftOrder);
        
        // Perform FFT
        fft.performFrequencyOnlyForwardTransform(fftData.data());
        
        // Calculate magnitudes
        for (int i = 0; i < fftOrder / 2; ++i) {
            magnitudes[i] = fftData[i];
        }
        
        // Calculate spectral centroid
        float centroid = calculateSpectralCentroid(magnitudes);
        centroids.push_back(centroid);
    }
    
    // Downsample for storage efficiency
    return downsampleForAnalysis(centroids, 100);  // Keep 100 points max
}

std::vector<float> ReferenceMatcher::extractRMSProfile(const juce::AudioBuffer<float>& audio) {
    std::vector<float> rmsValues;
    const float* data = audio.getReadPointer(0);
    int numSamples = audio.getNumSamples();
    
    const int windowSize = 1024;
    const int hopSize = 512;
    
    for (int pos = 0; pos < numSamples - windowSize; pos += hopSize) {
        float sumSquares = 0.0f;
        for (int i = 0; i < windowSize; ++i) {
            sumSquares += data[pos + i] * data[pos + i];
        }
        
        float rms = std::sqrt(sumSquares / windowSize);
        rmsValues.push_back(rms);
    }
    
    return downsampleForAnalysis(rmsValues, 50);  // Keep 50 points max
}

std::vector<float> ReferenceMatcher::extractStereoWidth(const juce::AudioBuffer<float>& audio) {
    std::vector<float> widthValues;
    
    if (audio.getNumChannels() < 2) {
        return widthValues;  // Mono track
    }
    
    const float* left = audio.getReadPointer(0);
    const float* right = audio.getReadPointer(1);
    int numSamples = audio.getNumSamples();
    
    const int windowSize = 1024;
    const int hopSize = 512;
    
    for (int pos = 0; pos < numSamples - windowSize; pos += hopSize) {
        float leftSum = 0.0f, rightSum = 0.0f;
        float midSum = 0.0f, sideSum = 0.0f;
        
        for (int i = 0; i < windowSize; ++i) {
            float l = left[pos + i];
            float r = right[pos + i];
            float m = (l + r) * 0.5f;
            float s = (l - r) * 0.5f;
            
            leftSum += l * l;
            rightSum += r * r;
            midSum += m * m;
            sideSum += s * s;
        }
        
        float leftRMS = std::sqrt(leftSum / windowSize);
        float rightRMS = std::sqrt(rightSum / windowSize);
        float midRMS = std::sqrt(midSum / windowSize);
        float sideRMS = std::sqrt(sideSum / windowSize);
        
        // Stereo width ratio (side/mid)
        float width = midRMS > 0.001f ? sideRMS / midRMS : 0.0f;
        widthValues.push_back(width);
    }
    
    return downsampleForAnalysis(widthValues, 50);  // Keep 50 points max
}

float ReferenceMatcher::calculateSpectralCentroid(const std::vector<float>& magnitudes) {
    float numerator = 0.0f;
    float denominator = 0.0f;
    
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        float frequency = static_cast<float>(i) * 44100.0f / fftOrder;  // Assuming 44.1kHz
        numerator += frequency * magnitudes[i];
        denominator += magnitudes[i];
    }
    
    return denominator > 0.001f ? numerator / denominator : 0.0f;
}

std::vector<float> ReferenceMatcher::downsampleForAnalysis(const std::vector<float>& data, int targetSize) {
    if (static_cast<int>(data.size()) <= targetSize) {
        return data;
    }
    
    std::vector<float> result(targetSize);
    float step = static_cast<float>(data.size()) / targetSize;
    
    for (int i = 0; i < targetSize; ++i) {
        int sourceIndex = static_cast<int>(i * step);
        result[i] = data[sourceIndex];
    }
    
    return result;
}

double ReferenceMatcher::calculateSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) {
        return 0.0;
    }
    
    // Calculate correlation coefficient
    double meanA = 0.0, meanB = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        meanA += a[i];
        meanB += b[i];
    }
    meanA /= a.size();
    meanB /= b.size();
    
    double numerator = 0.0, denomA = 0.0, denomB = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double diffA = a[i] - meanA;
        double diffB = b[i] - meanB;
        numerator += diffA * diffB;
        denomA += diffA * diffA;
        denomB += diffB * diffB;
    }
    
    double denominator = std::sqrt(denomA * denomB);
    return denominator > 0.001 ? numerator / denominator : 0.0;
}

} // namespace ai
} // namespace zenith
