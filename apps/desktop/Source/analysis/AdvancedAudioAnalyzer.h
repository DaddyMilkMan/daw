/*
  ==============================================================================
    AdvancedAudioAnalyzer.h
    Comprehensive audio analysis suite with professional features
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>
#include <complex>
#include <atomic>

namespace zenith {
namespace analysis {

// Analysis types
enum class AnalysisType {
    Loudness,
    Spectrum,
    Spectrogram,
    Phase,
    Stereo,
    Dynamics,
    Transients,
    Pitch,
    Harmony,
    Rhythm,
    Timbre,
    Quality,
    Custom
};

// Loudness measurements
struct LoudnessAnalysis {
    // Integrated loudness (LUFS)
    float integratedLUFS = -23.0f;
    float momentaryLUFS = -23.0f;
    float shortTermLUFS = -23.0f;
    float loudnessRange = 7.0f;  // LRA
    
    // Peak measurements
    float truePeak = -1.0f;      // dBTP
    float samplePeak = -1.0f;    // dBFS
    
    // Dynamic measurements
    float crestFactor = 12.0f;   // dB
    float dynamicRange = 12.0f;  // dB
    
    // Channel measurements
    std::vector<float> channelLUFS;
    std::vector<float> channelPeaks;
    
    // Time history (for graphs)
    std::vector<std::pair<float, float>> loudnessHistory;  // (time, LUFS)
    std::vector<std::pair<float, float>> peakHistory;      // (time, dB)
};

// Spectrum analysis
struct SpectrumAnalysis {
    // Frequency bins
    std::vector<float> frequencies;
    std::vector<float> magnitudes;
    std::vector<float> phases;
    
    // Critical bands (Bark scale)
    std::vector<float> criticalBands;
    std::vector<float> bandEnergies;
    
    // Octave bands
    std::vector<float> octaveBands;  // 31, 63, 125, 250, 500, 1k, 2k, 4k, 8k, 16k
    std::vector<float> octaveLevels;
    
    // Spectral features
    float spectralCentroid = 1000.0f;      // Hz
    float spectralSpread = 1000.0f;        // Hz
    float spectralSkewness = 0.0f;
    float spectralKurtosis = 0.0f;
    float spectralFlux = 0.0f;
    float spectralRolloff = 4000.0f;       // Hz
    float spectralFlatness = 0.0f;
    
    // Harmonic analysis
    std::vector<float> harmonicFrequencies;
    std::vector<float> harmonicMagnitudes;
    float fundamentalFrequency = 0.0f;     // Hz
    float harmonicRatio = 0.0f;
    float inharmonicity = 0.0f;
    
    // 2D spectrogram data
    std::vector<std::vector<float>> spectrogramData;
    int spectrogramFrames = 0;
    int spectrogramBins = 0;
    float spectrogramResolution = 10.0f;   // Hz per bin
};

// Phase analysis
struct PhaseAnalysis {
    // Stereo phase
    float phaseCorrelation = 1.0f;
    float phaseDifference = 0.0f;          // degrees
    float stereoWidth = 1.0f;
    float midSideRatio = 0.0f;             // dB
    
    // Phase vectors
    std::vector<float> leftPhase;
    std::vector<float> rightPhase;
    std::vector<float> phaseDifferenceHistory;
    
    // Phase coherence
    float phaseCoherence = 1.0f;
    std::vector<float> coherenceBands;
    
    // Mono compatibility
    float monoCompatibility = 1.0f;        // 0-1 scale
    std::vector<float> monoCompatibilityBands;
    
    // Phase issues
    std::vector<std::pair<float, float>> phaseIssues;  // (frequency, severity)
    int phaseProblemCount = 0;
};

// Dynamic analysis
struct DynamicAnalysis {
    // Compression characteristics
    float compressionRatio = 1.0f;
    float compressionThreshold = -60.0f;   // dB
    float compressionKnee = 0.0f;          // dB
    float compressionGain = 0.0f;          // dB
    
    // Envelope followers
    std::vector<float> attackEnvelope;
    std::vector<float> releaseEnvelope;
    float attackTime = 10.0f;              // ms
    float releaseTime = 100.0f;            // ms
    
    // Transient analysis
    float transientDensity = 0.0f;         // transients per second
    float transientStrength = 0.0f;
    std::vector<float> transientPositions; // sample indices
    std::vector<float> transientAmplitudes;
    
    // RMS analysis
    std::vector<float> rmsHistory;
    float averageRMS = -20.0f;             // dB
    float rmsVariation = 3.0f;             // dB
    
    // Peak-to-RMS ratio
    float peakToRMSRatio = 12.0f;          // dB
    std::vector<float> peakToRMSHistory;
};

// Pitch and harmony analysis
struct PitchAnalysis {
    // Fundamental frequency
    float fundamental = 0.0f;              // Hz
    float fundamentalConfidence = 0.0f;
    std::vector<float> fundamentalHistory;
    
    // Pitch detection
    std::vector<float> pitchSalience;
    float pitchStrength = 0.0f;
    float pitchStability = 0.0f;
    
    // Harmony
    std::vector<float> chromagram;         // 12 bins for chroma
    std::vector<float> chordProbabilities; // Common chords
    juce::String detectedChord;
    float chordConfidence = 0.0f;
    
    // Key detection
    juce::String detectedKey;
    juce::String detectedMode;             // "major" or "minor"
    float keyConfidence = 0.0f;
    
    // Tuning
    float tuningDeviation = 0.0f;          // cents from A=440
    float driftRate = 0.0f;                // cents per second
};

// Rhythm analysis
struct RhythmAnalysis {
    // Tempo
    float tempo = 120.0f;                  // BPM
    float tempoConfidence = 0.0f;
    std::vector<float> tempoHistory;
    
    // Time signature
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    float timeSignatureConfidence = 0.0f;
    
    // Beat tracking
    std::vector<float> beatPositions;      // seconds
    std::vector<float> beatStrengths;
    float beatStrength = 0.0f;
    
    // Rhythm patterns
    std::vector<float> rhythmicPattern;
    float rhythmicComplexity = 0.0f;
    float swingRatio = 0.5f;               // 0.0 = straight, 1.0 = triplet
    
    // Onset detection
    std::vector<float> onsets;
    float onsetRate = 0.0f;                // onsets per second
};

// Timbre analysis
struct TimbreAnalysis {
    // MFCC coefficients
    std::vector<float> mfccCoefficients;   // Typically 13 coefficients
    std::vector<std::vector<float>> mfccHistory;
    
    // Timbre descriptors
    float brightness = 0.0f;
    float roughness = 0.0f;
    float warmth = 0.0f;
    float hardness = 0.0f;
    float depth = 0.0f;
    
    // Instrument classification
    std::vector<std::pair<juce::String, float>> instrumentProbabilities;
    juce::String primaryInstrument;
    float instrumentConfidence = 0.0f;
    
    // Formant analysis (for vocals)
    std::vector<float> formantFrequencies; // F1, F2, F3, etc.
    std::vector<float> formantBandwidths;
    float formantCenter = 0.0f;
};

// Quality assessment
struct QualityAnalysis {
    // Overall quality score (0-100)
    float overallScore = 75.0f;
    
    // Quality metrics
    float clarity = 75.0f;
    float punch = 75.0f;
    float warmth = 75.0f;
    float presence = 75.0f;
    float stereoImage = 75.0f;
    float dynamicRange = 75.0f;
    
    // Issues detected
    struct QualityIssue {
        juce::String description;
        juce::String severity;             // "low", "medium", "high", "critical"
        juce::String category;             // "clipping", "noise", "phase", "frequency", "dynamic"
        float frequency = 0.0f;            // Hz (if applicable)
        float confidence = 0.0f;
    };
    
    std::vector<QualityIssue> issues;
    
    // Recommendations
    std::vector<juce::String> recommendations;
    
    // Comparison to reference
    float referenceSimilarity = 0.0f;      // 0-1 scale
    std::vector<float> referenceDifferences;
};

// Advanced audio analyzer
class AdvancedAudioAnalyzer : public juce::Timer,
                             public juce::AsyncUpdater {
public:
    struct AnalyzerConfig {
        // Analysis settings
        bool enableLoudnessAnalysis = true;
        bool enableSpectrumAnalysis = true;
        bool enablePhaseAnalysis = true;
        bool enableDynamicAnalysis = true;
        bool enablePitchAnalysis = true;
        bool enableRhythmAnalysis = true;
        bool enableTimbreAnalysis = true;
        bool enableQualityAnalysis = true;
        
        // FFT settings
        int fftOrder = 12;                  // 4096 samples
        int fftOverlap = 4;                 // 75% overlap
        juce::dsp::WindowingFunction<float>::WindowingMethod windowMethod = 
            juce::dsp::WindowingFunction<float>::hann;
        
        // Frequency resolution
        float minFrequency = 20.0f;         // Hz
        float maxFrequency = 20000.0f;      // Hz
        int frequencyBins = 2048;
        
        // Time resolution
        float analysisWindow = 10.0f;       // seconds
        float updateInterval = 100.0f;      // ms
        
        // Quality assessment
        bool enableQualityAssessment = true;
        float qualityThreshold = 70.0f;
        bool detectIssues = true;
        
        // Real-time settings
        bool enableRealTimeAnalysis = false;
        int bufferSize = 1024;
        double sampleRate = 44100.0;
    };
    
    AdvancedAudioAnalyzer();
    ~AdvancedAudioAnalyzer() override;
    
    // Configuration
    void setConfig(const AnalyzerConfig& config);
    AnalyzerConfig getConfig() const;
    
    // Audio input
    void processAudio(const juce::AudioBuffer<float>& buffer);
    void setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void clearAudio();
    
    // Analysis control
    void analyzeAll();
    void analyzeLoudness();
    void analyzeSpectrum();
    void analyzePhase();
    void analyzeDynamics();
    void analyzePitch();
    void analyzeRhythm();
    void analyzeTimbre();
    void analyzeQuality();
    
    // Results access
    LoudnessAnalysis getLoudnessAnalysis() const;
    SpectrumAnalysis getSpectrumAnalysis() const;
    PhaseAnalysis getPhaseAnalysis() const;
    DynamicAnalysis getDynamicAnalysis() const;
    PitchAnalysis getPitchAnalysis() const;
    RhythmAnalysis getRhythmAnalysis() const;
    TimbreAnalysis getTimbreAnalysis() const;
    QualityAnalysis getQualityAnalysis() const;
    
    // Reference comparison
    void loadReference(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void compareWithReference();
    float getReferenceSimilarity() const;
    
    // Export
    bool exportResults(const juce::File& file) const;
    bool exportSpectrum(const juce::File& file) const;
    bool exportSpectrogram(const juce::File& file) const;
    
    // Timer callback for real-time analysis
    void timerCallback() override;
    void handleAsyncUpdate() override;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void analysisComplete(AnalysisType type) {}
        virtual void loudnessUpdated(const LoudnessAnalysis& analysis) {}
        virtual void spectrumUpdated(const SpectrumAnalysis& analysis) {}
        virtual void phaseUpdated(const PhaseAnalysis& analysis) {}
        virtual void dynamicsUpdated(const DynamicAnalysis& analysis) {}
        virtual void pitchUpdated(const PitchAnalysis& analysis) {}
        virtual void rhythmUpdated(const RhythmAnalysis& analysis) {}
        virtual void timbreUpdated(const TimbreAnalysis& analysis) {}
        virtual void qualityUpdated(const QualityAnalysis& analysis) {}
        virtual void referenceCompared(float similarity) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Configuration
    AnalyzerConfig config;
    
    // Audio processing
    juce::AudioBuffer<float> audioBuffer;
    double currentSampleRate = 44100.0;
    std::mutex audioMutex;
    
    // FFT processor
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<std::complex<float>> fftBuffer;
    std::vector<float> magnitudeBuffer;
    std::vector<float> phaseBuffer;
    
    // Analysis results
    LoudnessAnalysis loudnessAnalysis;
    SpectrumAnalysis spectrumAnalysis;
    PhaseAnalysis phaseAnalysis;
    DynamicAnalysis dynamicAnalysis;
    PitchAnalysis pitchAnalysis;
    RhythmAnalysis rhythmAnalysis;
    TimbreAnalysis timbreAnalysis;
    QualityAnalysis qualityAnalysis;
    
    // Reference audio
    juce::AudioBuffer<float> referenceBuffer;
    double referenceSampleRate = 44100.0;
    bool hasReference = false;
    float referenceSimilarity = 0.0f;
    
    // State
    std::atomic<bool> isAnalyzing{false};
    std::atomic<bool> needsAnalysis{false};
    juce::Time lastAnalysisTime;
    
    // Listeners
    std::vector<Listener*> listeners;
    
    // Analysis methods
    void performFFT(const juce::AudioBuffer<float>& buffer);
    void calculateLoudness(const juce::AudioBuffer<float>& buffer);
    void calculateSpectrum();
    void calculatePhase();
    void calculateDynamics();
    void calculatePitch();
    void calculateRhythm();
    void calculateTimbre();
    void calculateQuality();
    
    // Loudness calculations
    float calculateLUFS(const juce::AudioBuffer<float>& buffer);
    float calculateTruePeak(const juce::AudioBuffer<float>& buffer);
    float calculateLRA(const juce::AudioBuffer<float>& buffer);
    
    // Spectrum calculations
    void calculateCriticalBands();
    void calculateOctaveBands();
    void calculateSpectralFeatures();
    void calculateHarmonics();
    
    // Phase calculations
    void calculateStereoPhase();
    void calculatePhaseCoherence();
    void calculateMonoCompatibility();
    
    // Dynamic calculations
    void calculateEnvelope();
    void calculateTransients();
    void calculateCompression();
    
    // Pitch calculations
    float detectFundamental(const std::vector<float>& magnitude);
    void calculateChromagram();
    void detectKey();
    
    // Rhythm calculations
    float detectTempo();
    void detectBeats();
    void detectOnsets();
    
    // Timbre calculations
    void calculateMFCC();
    void calculateTimbreDescriptors();
    void classifyInstrument();
    
    // Quality assessment
    void assessClarity();
    void assessPunch();
    void assessWarmth();
    void detectIssues();
    
    // Reference comparison
    float calculateLoudnessDifference();
    float calculateSpectralDifference();
    float calculatePhaseDifference();
    float calculateOverallSimilarity();
    
    // Utility methods
    void initializeFrequencyBins();
    void initializeCriticalBands();
    void initializeOctaveBands();
    float hzToBark(float hz) const;
    float barkToHz(float bark) const;
    float hzToMel(float hz) const;
    float melToHz(float mel) const;
    float hzToCent(float hz) const;
    
    // Notification
    void notifyAnalysisComplete(AnalysisType type);
    void notifyLoudnessUpdated(const LoudnessAnalysis& analysis);
    void notifySpectrumUpdated(const SpectrumAnalysis& analysis);
    void notifyPhaseUpdated(const PhaseAnalysis& analysis);
    void notifyDynamicsUpdated(const DynamicAnalysis& analysis);
    void notifyPitchUpdated(const PitchAnalysis& analysis);
    void notifyRhythmUpdated(const RhythmAnalysis& analysis);
    void notifyTimbreUpdated(const TimbreAnalysis& analysis);
    void notifyQualityUpdated(const QualityAnalysis& analysis);
    void notifyReferenceCompared(float similarity);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedAudioAnalyzer)
};

// Analysis visualization component
class AnalysisVisualizer : public juce::Component,
                          public AdvancedAudioAnalyzer::Listener {
public:
    AnalysisVisualizer();
    ~AnalysisVisualizer() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Analyzer access
    void setAnalyzer(AdvancedAudioAnalyzer* analyzer);
    AdvancedAudioAnalyzer* getAnalyzer() const { return analyzer; }
    
    // Visualization control
    void setAnalysisType(AnalysisType type);
    void setShowGrid(bool show);
    void setShowLabels(bool show);
    void setScaleLinear(bool linear);
    void setFrequencyRange(float minHz, float maxHz);
    void setDynamicRange(float minDb, float maxDb);
    
    // AdvancedAudioAnalyzer::Listener
    void loudnessUpdated(const LoudnessAnalysis& analysis) override;
    void spectrumUpdated(const SpectrumAnalysis& analysis) override;
    void phaseUpdated(const PhaseAnalysis& analysis) override;
    void dynamicsUpdated(const DynamicAnalysis& analysis) override;
    void pitchUpdated(const PitchAnalysis& analysis) override;
    void rhythmUpdated(const RhythmAnalysis& analysis) override;
    void timbreUpdated(const TimbreAnalysis& analysis) override;
    void qualityUpdated(const QualityAnalysis& analysis) override;
    
private:
    AdvancedAudioAnalyzer* analyzer = nullptr;
    AnalysisType currentType = AnalysisType::Spectrum;
    
    // Visualization settings
    bool showGrid = true;
    bool showLabels = true;
    bool scaleLinear = false;
    float minFrequency = 20.0f;
    float maxFrequency = 20000.0f;
    float minDb = -120.0f;
    float maxDb = 0.0f;
    
    // Drawing methods
    void drawLoudnessMeter(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g);
    void drawSpectrogram(juce::Graphics& g);
    void drawPhaseMeter(juce::Graphics& g);
    void drawDynamicRange(juce::Graphics& g);
    void drawPitchDetection(juce::Graphics& g);
    void drawRhythmGrid(juce::Graphics& g);
    void drawTimbreRadar(juce::Graphics& g);
    void drawQualityMeter(juce::Graphics& g);
    
    // Utility
    void drawGrid(juce::Graphics& g);
    void drawLabels(juce::Graphics& g);
    float frequencyToX(float hz) const;
    float dbToY(float db) const;
    float xToFrequency(float x) const;
    float yToDb(float y) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalysisVisualizer)
};

} // namespace analysis
} // namespace zenith
