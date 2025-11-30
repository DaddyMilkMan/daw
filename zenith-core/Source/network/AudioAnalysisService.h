/*
  ==============================================================================

    AudioAnalysisService.h
    Created: 2025-11-29
    Author:  Dr. Maya Rodriguez (Lead Integration)

    Service for analyzing audio files using Python + librosa
    Integrates with Grok for AI-powered feedback

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <memory>

namespace zenith {

/**
    Audio analysis results from Python librosa
*/
struct AudioAnalysisResults
{
    bool success = false;
    juce::String errorMessage;
    
    // File info
    double durationSeconds = 0.0;
    int sampleRate = 0;
    bool isStereo = false;
    
    // Loudness
    double averageRmsDb = 0.0;
    double peakRmsDb = 0.0;
    double peakAmplitudeDb = 0.0;
    double dynamicRangeDb = 0.0;
    double headroomDb = 0.0;
    
    // Spectral
    double spectralCentroidHz = 0.0;
    double spectralRolloffHz = 0.0;
    double spectralBandwidthHz = 0.0;
    double zeroCrossingRate = 0.0;
    juce::String brightness;  // "bright" or "dark"
    
    // Frequency bands (dB)
    double subBassDb = 0.0;      // 20-60 Hz
    double bassDb = 0.0;         // 60-250 Hz
    double lowMidsDb = 0.0;      // 250-500 Hz
    double midsDb = 0.0;         // 500-2000 Hz
    double highMidsDb = 0.0;     // 2000-4000 Hz
    double presenceDb = 0.0;     // 4000-6000 Hz
    double brillianceDb = 0.0;   // 6000-20000 Hz
    
    double dominantFrequencyHz = 0.0;
    
    // Rhythm
    double tempoBpm = 0.0;
    double beatStrength = 0.0;
    
    // Stereo (if applicable)
    double stereoCorrelation = 0.0;
    juce::String stereoWidth;  // "wide" or "narrow"
    double stereoBalance = 0.0;  // -1 (left) to 1 (right)
    
    /**
        Convert to JSON for Grok
    */
    juce::var toJSON() const;
    
    /**
        Create human-readable summary
    */
    juce::String toSummary() const;
};

/**
    Service for audio analysis
*/
class AudioAnalysisService
{
public:
    //==========================================================================
    AudioAnalysisService();
    ~AudioAnalysisService();
    
    //==========================================================================
    /**
        Check if Python and librosa are available
        
        @return true if audio analysis is available
    */
    bool isAvailable() const;
    
    /**
        Get error message if not available
    */
    juce::String getAvailabilityError() const;
    
    //==========================================================================
    /**
        Analyze an audio file
        
        @param audioFile Path to audio file
        @param onComplete Callback with results
        @param onError Callback on error
    */
    void analyzeAudioFile(
        const juce::File& audioFile,
        std::function<void(AudioAnalysisResults results)> onComplete,
        std::function<void(juce::String error)> onError
    );
    
    /**
        Cancel ongoing analysis
    */
    void cancelAnalysis();
    
private:
    //==========================================================================
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalysisService)
};

} // namespace zenith
