/*
  ==============================================================================

    TimbreTransfer.h
    Created: [Date] Author: Claude AI
    Neural timbre transfer between sounds

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class TimbreTransferEngine
{
public:
    // Timbre feature structure
    struct TimbreFeature
    {
        // Spectral features
        std::array<float, 128> spectralEnvelope;   // Spectral envelope
        std::array<float, 64> spectralCentroid;   // Spectral centroid history
        float brightness;                          // High-frequency content (0-1)
        float warmth;                             // Low-mid emphasis (0-1)
        float clarity;                           // Spectral clarity (0-1)

        // Temporal features
        std::array<float, 32> temporalEnvelope;   // Temporal envelope shape
        float attack;                             // Transient sharpness
        float decay;                              // Decay speed
        float sustain;                            // Sustain level
        float release;                            // Release speed

        // Texture features
        float roughness;                          // Signal roughness
        float noisiness;                          // Noise component amount
        float modularity;                         // Periodic vs random content
        float complexity;                         // Signal complexity

        // Harmonic features
        float harmonicContent;                     // Harmonic vs noise ratio
        float fundamentalFrequency;               // Detected fundamental
        float inharmonicity;                      // Inharmonicity amount
        float formantStrength;                    // Formant presence

        // Metadata
        juce::String sourceDescription;
        juce::String category;
        juce::uint64 timestamp;
    };

    // Transfer modes
    enum class TransferMode
    {
        Full,            // Transfer all features
        Spectral,        // Only spectral features
        Temporal,        // Only temporal features
        Texture,        // Only texture features
        Harmonic,       // Only harmonic features
        Custom          // Custom feature subset
    };

    // Feature extraction parameters
    struct ExtractionParameters
    {
        int fftSize = 2048;                        // FFT analysis size
        float hopSize = 0.5f;                     // Hop size as fraction of FFT
        int analysisRate = 100;                   // Analysis rate (Hz)
        float noiseFloor = -60.0f;                 // Noise floor (dB)
        float preEmphasis = 6.0f;                  // Pre-emphasis coefficient
        bool usePhase = true;                      // Include phase analysis
        bool useHarmonics = true;                  // Harmonic analysis
        int maxFormants = 4;                      // Maximum formants to detect
        bool realTime = true;                     // Real-time processing mode
    };

    TimbreTransferEngine();
    ~TimbreTransferEngine();

    // Engine initialization
    void initialize(double sampleRate);
    void shutdown();

    // Feature extraction
    bool extractTimbre(const juce::AudioBuffer<float>& referenceAudio,
                       TimbreFeature& output,
                       const ExtractionParameters& params = ExtractionParameters());
    bool extractTimbreFromFile(const juce::File& audioFile,
                             TimbreFeature& output,
                             const ExtractionParameters& params = ExtractionParameters());

    // Timbre transfer
    void applyTimbre(const TimbreFeature& timbre);
    void applyTimbre(const TimbreFeature& timbre, const juce::AudioBuffer<float>& targetOutput);
    void blendTimbres(const TimbreFeature& source1, const TimbreFeature& source2,
                     float blendRatio, TimbreFeature& result);

    // Transfer control
    void setTransferMode(TransferMode mode);
    TransferMode getTransferMode() const;
    void setFeatureWeights(const std::map<juce::String, float>& weights);
    std::map<juce::String, float> getFeatureWeights() const;

    // Real-time processing
    void enableRealTime(bool enabled);
    bool isRealTimeEnabled() const;
    void setProcessingLatency(float latencyMs);
    float getProcessingLatency() const;

    // Model management
    bool loadTransferModel(const juce::File& modelFile);
    void loadPredefinedTimbres();
    TimbreFeature getPredefinedTimbre(const juce::String& name) const;
    juce::StringArray getPredefinedTimbres() const;

    // Analysis and monitoring
    float getExtractionAccuracy() const;
    float getTransferQuality() const;
    juce::Array<float> getFeatureImportance() const;

    // Export/Import
    bool saveTimbreToFile(const TimbreFeature& timbre, const juce::File& file);
    bool loadTimbreFromFile(const juce::File& file, TimbreFeature& timbre);
    bool saveTimbreLibrary(const juce::File& directory);
    bool loadTimbreLibrary(const juce::File& directory);

private:
    // Engine state
    double sampleRate_;
    bool initialized_;
    bool realTimeEnabled_;
    TransferMode transferMode_;

    // Feature extraction
    ExtractionParameters extractionParams_;
    std::unique_ptr<juce::dsp::FFT> fft_;
    juce::AudioBuffer<float> analysisBuffer_;
    std::vector<float> windowBuffer_;

    // Transfer processing
    std::map<juce::String, float> featureWeights_;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> windowFunction_;

    // Feature analysis
    struct AnalysisState
    {
        std::vector<float> spectralFeatures;
        std::vector<float> temporalFeatures;
        std::vector<float> harmonicFeatures;
        std::vector<float> textureFeatures;
        float confidence;
        float processingTime;
    } analysisState_;

    // Predefined timbres
    juce::HashMap<juce::String, TimbreFeature> predefinedTimbres_;

    // Model loading
    juce::File modelFile_;
    bool modelLoaded_;

    // Performance monitoring
    float extractionAccuracy_;
    float transferQuality_;
    juce::Array<float> featureImportance_;

    // Real-time processing
    float processingLatency_;
    juce::AudioBuffer<float> inputBuffer_;
    juce::AudioBuffer<float> outputBuffer_;
    std::vector<TimbreFeature> timbreQueue_;

    // Private helper methods
    void initializeFFT();
    void initializeWindowing();
    void extractSpectralFeatures(const juce::AudioBuffer<float>& buffer,
                               std::vector<float>& features,
                               TimbreFeature& output);
    void extractTemporalFeatures(const juce::AudioBuffer<float>& buffer,
                                std::vector<float>& features,
                                TimbreFeature& output);
    void extractHarmonicFeatures(const juce::AudioBuffer<float>& buffer,
                               std::vector<float>& features,
                               TimbreFeature& output);
    void extractTextureFeatures(const juce::AudioBuffer<float>& buffer,
                              std::vector<float>& features,
                              TimbreFeature& output);

    // Transfer methods
    void transferSpectral(const TimbreFeature& source, TimbreFeature& target);
    void transferTemporal(const TimbreFeature& source, TimbreFeature& target);
    void transferHarmonic(const TimbreFeature& source, TimbreFeature& target);
    void transferTexture(const TimbreFeature& source, TimbreFeature& target);
    void transferCustom(const TimbreFeature& source, TimbreFeature& target);

    // Analysis utilities
    float calculateSpectralCentroid(const std::vector<float>& spectrum) const;
    float calculateRoughness(const std::vector<float>& spectrum) const;
    float calculateBrightness(const std::vector<float>& spectrum) const;
    float calculateSpectralContrast(const std::vector<float>& spectrum) const;
    std::vector<float> detectFormants(const std::vector<float>& spectrum) const;

    // Real-time processing
    void processRealTimeTimbre(const juce::AudioBuffer<float>& input);
    void updateTimbreQueue();
    void applyTimbreTransferRealTime(const TimbreFeature& timbre);

    // Utility methods
    void normalizeFeatureVector(std::vector<float>& features) const;
    float calculateFeatureDistance(const TimbreFeature& f1, const TimbreFeature& f2) const;
    juce::String classifyTimbreCategory(const TimbreFeature& timbre) const;
    void updatePredefinedTimbres();

    // Model operations
    bool loadNeuralModel(const juce::File& modelFile);
    void unloadNeuralModel();
    bool validateModel(const TimbreFeature& input, TimbreFeature& output);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimbreTransferEngine)
};

} // namespace Zenith