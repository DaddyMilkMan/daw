/*
  ==============================================================================

    AdvancedWavetableEngine.h
    Created: [Date] Author: Claude AI
    Professional wavetable synthesis with morphing and editing

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class AdvancedWavetableVoice
{
public:
    // Wavetable structure
    struct WavetableData
    {
        std::vector<std::vector<float>> frames;     // Multiple waveform frames
        int sampleRate;                             // Original sample rate
        int framesPerTable;                         // Number of frames
        int samplesPerFrame;                        // Samples per frame
        juce::String name;                         // Table name
        juce::String category;                      // Table category
        std::vector<juce::String> tags;           // Search tags
        float loopCrossfade;                       // Loop crossfade amount
        bool isLooping;                            // Whether table loops
        juce::Range<float> frequencyRange;        // Optimal frequency range
        juce::Array<float> harmonicContent;        // Harmonic analysis
    };

    // Interpolation types
    enum class InterpolationType
    {
        Linear,       // Linear interpolation
        Cosine,       // Cosine interpolation
        Cubic,        // Cubic spline
        Sinc,         // Sinc interpolation
        WindowedSinc, // Windowed sinc
        Phase        // Phase coherent
    };

    // Wavetable processing modes
    enum class ProcessingMode
    {
        Normal,       // Standard wavetable playback
        Morphing,     // Between multiple tables
        Granular,     // Granular synthesis
        FM,           // FM modulation
        RingMod,      // Ring modulation
        Wavefolding   // Wave folding
    };

    AdvancedWavetableVoice();
    ~AdvancedWavetableVoice();

    // Voice lifecycle
    void noteOn(float frequency, float velocity);
    void noteOff();
    void reset();

    // Audio processing
    void process(juce::AudioBuffer<float>& buffer, int numSamples);
    bool isActive() const;

    // Wavetable management
    void setWavetable(const WavetableData& table);
    void setWavetableIndex(int index);
    WavetableData getCurrentWavetable() const;

    // Playback parameters
    void setStartFrame(float start);             // 0-1, start frame
    void setEndFrame(float end);                 // 0-1, end frame
    void setLoopMode(bool enableLoop);
    void setLoopCrossfade(float crossfade);      // 0-1, crossfade amount
    void setInterpolationType(InterpolationType type);
    void setPlaybackDirection(int direction);     // 1=forward, -1=reverse, 0=ping-pong

    // Advanced processing
    void setProcessingMode(ProcessingMode mode);
    void setMorphAmount(float amount);           // Morph between tables
    void setGranularSize(float size);           // Granular size (samples)
    void setGranularDensity(float density);      // Granular density
    void setGranularSpread(float spread);        // Spatial spread
    void setFmAmount(float amount);              // FM modulation amount
    void setFmFrequency(float frequency);        // FM frequency
    void setRingModAmount(float amount);         // Ring modulation amount
    void setRingModFrequency(float frequency);   // Ring modulation frequency
    void setWavefoldingAmount(float amount);     // Wave folding amount

    // Modulation parameters
    void setPitchModulation(float amount);       // Pitch modulation amount
    void setAmplitudeModulation(float amount);    // Amplitude modulation amount
    void setFrameModulation(float amount);       // Frame modulation amount
    void setFilterModulation(float amount);      // Filter modulation amount

    // Filter parameters
    void setFilterCutoff(float cutoff);          // Filter cutoff frequency
    void setFilterResonance(float resonance);    // Filter resonance
    void setFilterType(int type);                // Filter type (0=LPF, 1=HPF, 2=BP, 3=Notch)
    void setFilterModulationSource(int source);  // Modulation source (0=none, 1=LFO, 2=env)

    // LFO parameters
    void setLFORate(float rate);                // LFO rate (Hz)
    void setLFOAmount(float amount);              // LFO amount
    void setLFOWaveform(int waveform);          // LFO waveform (0=sine, 1=tri, 2=square, 3=saw)
    void setLFOPhase(float phase);               // LFO phase offset

    // Envelope parameters
    void setAttack(float attack);               // Attack time
    void setDecay(float decay);                 // Decay time
    void setSustain(float sustain);             // Sustain level
    void setRelease(float release);              // Release time
    void setEnvelopeCurve(float curve);          // Envelope curve shape

    // MPE support
    void setPitchBend(float bendAmount);          // Pitch bend amount
    void setPressure(float pressure);            // Pressure sensitivity
    void setTimbre(float timbre);                // Timbre modulation

    // Parameter smoothing
    void setSmoothingTime(float timeMs);          // Parameter smoothing time
    float getSmoothingTime() const;

    // Analysis and monitoring
    float getRmsLevel() const;
    float getPeakLevel() const;
    float getFundamentalFrequency() const;
    float getHarmonicContent() const;

    // Voice age
    float getVoiceAge() const;
    void updateAge();

private:
    // Core parameters
    float frequency_;
    float velocity_;
    bool isActive_;

    // Wavetable data
    WavetableData currentWavetable_;
    int wavetableIndex_;
    float startFrame_;
    float endFrame_;
    bool loopMode_;
    float loopCrossfade_;
    InterpolationType interpolationType_;
    int playbackDirection_;

    // Advanced processing
    ProcessingMode processingMode_;
    float morphAmount_;
    float granularSize_;
    float granularDensity_;
    float granularSpread_;
    float fmAmount_;
    float fmFrequency_;
    float ringModAmount_;
    float ringModFrequency_;
    float wavefoldingAmount_;

    // Modulation
    float pitchModulation_;
    float amplitudeModulation_;
    float frameModulation_;
    float filterModulation_;

    // Filter
    float filterCutoff_;
    float filterResonance_;
    int filterType_;
    int filterModulationSource_;

    // LFO
    float lfoRate_;
    float lfoAmount_;
    int lfoWaveform_;
    float lfoPhase_;
    float currentLfoValue_;

    // Envelope
    struct Envelope
    {
        float value;
        float attackTime;
        float decayTime;
        float sustainLevel;
        float releaseTime;
        float curve;
        float state;  // 0=attack, 1=decay, 2=sustain, 3=release
        float phase;
    } envelope_;

    // MPE parameters
    float pitchBend_;
    float pressure_;
    float timbre_;

    // Audio processing
    double sampleRate_;
    int bufferSize_;

    // Wavetable processing
    std::vector<float> interpolatedFrame_;
    float currentFrame_;
    float frameIncrement_;
    int currentSample_;
    int totalSamples_;

    // Granular synthesis
    struct Grain
    {
        float position;
        float size;
        float amplitude;
        float pan;
        float age;
        float maxAge;
    };
    std::vector<Grain> grains_;
    float grainPhase_;

    // Ring modulation
    float ringModPhase_;
    float ringModSample_;

    // Wave folding
    float foldThreshold_;
    float foldAmount_;

    // Filter processing
    std::unique_ptr<juce::dsp::IIR::Filter<float>> filter_;

    // Smoothed parameters
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFrequency_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmplitude_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFilterCutoff_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFrame_;

    // Voice age tracking
    juce::uint64 voiceStartTime_;
    float voiceAge_;

    // Analysis data
    float rmsLevel_;
    float peakLevel_;
    float fundamentalFrequency_;
    float harmonicContent_;

    // Private helper methods
    void initializeWavetableProcessing();
    void initializeGranularSynthesis();
    void initializeFilter();
    void updateFrameIncrement();
    void updateLFO();
    void updateEnvelope();

    // Wavetable processing methods
    void interpolateFrame(int frameIndex, float position);
    void processNormalPlayback(juce::AudioBuffer<float>& buffer, int numSamples);
    void processMorphing(juce::AudioBuffer<float>& buffer, int numSamples);
    void processGranular(juce::AudioBuffer<float>& buffer, int numSamples);
    void processFM(juce::AudioBuffer<float>& buffer, int numSamples);
    void processRingMod(juce::AudioBuffer<float>& buffer, int numSamples);
    void processWavefolding(juce::AudioBuffer<float>& buffer, int numSamples);

    // Granular synthesis
    void createGrain();
    void updateGrains();
    void processGrains(juce::AudioBuffer<float>& buffer, int numSamples);

    // Modulation processing
    void applyPitchModulation(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyAmplitudeModulation(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyFrameModulation(juce::AudioBuffer<float>& buffer, int numSamples);
    void applyFilterModulation(juce::AudioBuffer<float>& buffer, int numSamples);

    // Filter processing
    void applyFilter(juce::AudioBuffer<float>& buffer, int numSamples);

    // Utility methods
    float getInterpolatedSample(float position, int frameIndex) const;
    float interpolateLinear(float v1, float v2, float t) const;
    float interpolateCosine(float v1, float v2, float t) const;
    float interpolateCubic(float v0, float v1, float v2, float v3, float t) const;
    float windowSinc(float x, float width) const;
    float applyWavefolding(float sample, float amount) const;

    // LFO generation
    float generateLFOValue(float phase, int waveform) const;
    float sineWave(float phase) const;
    float triangleWave(float phase) const;
    float squareWave(float phase) const;
    float sawWave(float phase) const;

    // Analysis
    void analyzeWavetable();
    float calculateHarmonicContent() const;
    float calculateFundamentalFrequency() const;

    // MPE processing
    void applyMPEParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedWavetableVoice)
};

class AdvancedWavetableEngine
{
public:
    AdvancedWavetableEngine();
    ~AdvancedWavetableEngine();

    // Engine initialization
    void initialize(double sampleRate, int bufferSize);
    void shutdown();

    // Voice management
    AdvancedWavetableVoice* createVoice();
    void releaseVoice(AdvancedWavetableVoice* voice);
    void updateAllVoices();
    int getActiveVoices() const;

    // Wavetable management
    void loadWavetable(const WavetableData& table);
    void loadWavetableFromFile(const juce::File& file);
    void unloadWavetable(int index);
    WavetableData getWavetable(int index) const;
    int getWavetableCount() const;

    // Global parameters
    void setGlobalMorphAmount(float amount);
    void setGlobalInterpolationType(AdvancedWavetableVoice::InterpolationType type);
    void setGlobalProcessingMode(AdvancedWavetableVoice::ProcessingMode mode);
    void setGlobalSampleRate(double sampleRate);

    // Performance optimization
    void setOversampling(int factor);           // Oversampling factor
    void setInterpolationQuality(int quality);    // Interpolation quality
    void setCacheEnabled(bool enabled);          // Wavetable caching
    void setMemoryLimit(float limitMB);         // Memory limit

    // Analysis and monitoring
    float getTotalCpuUsage() const;
    float getAverageHarmonicContent() const;
    int getTotalWavetables() const;
    float getAverageWavetableSize() const;

private:
    // Engine state
    double sampleRate_;
    int bufferSize_;
    bool initialized_;

    // Voice management
    std::vector<std::unique_ptr<AdvancedWavetableVoice>> voices_;
    std::vector<AdvancedWavetableVoice*> activeVoices_;
    int maxVoices_;

    // Wavetable management
    std::vector<WavetableData> wavetables_;
    std::map<juce::String, int> wavetableIndex_;
    int currentWavetableIndex_;
    float globalMorphAmount_;
    bool cacheEnabled_;

    // Performance optimization
    int oversamplingFactor_;
    int interpolationQuality_;
    float memoryLimitMB_;
    size_t currentMemoryUsage_;

    // Global parameters
    AdvancedWavetableVoice::InterpolationType globalInterpolationType_;
    AdvancedWavetableVoice::ProcessingMode globalProcessingMode_;

    // Performance monitoring
    juce::ScopedCPUUsageMeter cpuMeter_;
    float totalCpuUsage_;
    float averageHarmonicContent_;

    // Private helper methods
    void initializeVoicePool();
    void updateActiveVoices();
    void optimizeWavetableCache();
    void manageMemoryUsage();
    void processGlobalParameters();

    // Wavetable operations
    void loadWavetableInternal(const WavetableData& table);
    void unloadWavetableInternal(int index);
    void optimizeWavetable(WavetableData& table);
    void cacheWavetable(int index);
    void uncacheWavetable(int index);

    // Analysis
    void updateStatistics();
    float calculateAverageHarmonicContent() const;
    size_t calculateWavetableSize(const WavetableData& table) const;

    // Utility methods
    juce::String generateWavetableId() const;
    bool validateWavetable(const WavetableData& table) const;
    void normalizeWavetable(WavetableData& table) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedWavetableEngine)
};

} // namespace Zenith