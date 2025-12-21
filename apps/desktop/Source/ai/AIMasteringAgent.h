/*
  ==============================================================================
    apps/desktop/Source/ai/AIMasteringAgent.h
    Automated mixing and mastering agent with real DSP and Grok AI integration.
  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../network/GrokAPIClient.h"
#include <map>

// Forward declarations
namespace zenith {
class Engine;
}

namespace zenith {
namespace ai {

//==============================================================================
/**
    Audio Feature Extractor for AI analysis
*/
class AudioFeatureExtractor {
public:
    struct Features {
        float peakDb = -100.0f;
        float rmsDb = -100.0f;
        float crestFactor = 0.0f;
        float subBass = 0.0f;
        float bass = 0.0f;
        float lowMid = 0.0f;
        float mid = 0.0f;
        float highMid = 0.0f;
        float presence = 0.0f;
        float brilliance = 0.0f;
        float stereoWidth = 0.0f;
        bool valid = false;
    };
    
    Features extract(const juce::AudioBuffer<float>& buffer, double sampleRate);
    static juce::String toJSON(const Features& features);
    
private:
    void analyzeFrequencyBands(const juce::AudioBuffer<float>& buffer, double sampleRate, Features& features);
    void analyzeStereo(const juce::AudioBuffer<float>& buffer, Features& features);
};

//==============================================================================
/**
    Grok AI interface for mastering decisions
*/
class GrokMasteringAI {
public:
    struct EQDecision {
        float lowShelfGain = 0.0f;
        float midCutGain = 0.0f;
        float presenceGain = 0.0f;
        float airGain = 0.0f;
    };
    
    struct CompressionDecision {
        float threshold = -12.0f;
        float ratio = 2.0f;
        float attack = 30.0f;
        float release = 200.0f;
    };
    
    struct MasteringDecision {
        EQDecision eq;
        CompressionDecision compression;
        float limiterCeiling = -0.3f;
        juce::String reasoning;
        bool valid = false;
        std::map<int, float> trackGains;
    };
    
    GrokMasteringAI();
    
    MasteringDecision getMasteringDecision(
        const AudioFeatureExtractor::Features& features,
        const juce::String& userIntent,
        float targetLoudness);
        
private:
    juce::String buildPrompt(
        const AudioFeatureExtractor::Features& features,
        const juce::String& userIntent,
        float targetLoudness);
    MasteringDecision parseGrokResponse(const juce::String& response);
    
    std::unique_ptr<GrokAPIClient> grokClient_;
};

//==============================================================================
/**
    Mastering-grade Parametric EQ
*/
class MasteringEQ {
public:
    struct Settings {
        float lowShelfGain = 0.0f;
        float midCutGain = 0.0f;
        float presenceGain = 0.0f;
        float airGain = 0.0f;
        bool enabled = true;
    };
    
    MasteringEQ() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) {
        highPass_.prepare(spec);
        highPass_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highPass_.setCutoffFrequency(30.0f);
        
        lowShelf_.prepare(spec);
        *lowShelf_.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            spec.sampleRate, 100.0f, 0.7f, juce::Decibels::decibelsToGain(1.0f));
        
        midCut_.prepare(spec);
        *midCut_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            spec.sampleRate, 300.0f, 1.0f, juce::Decibels::decibelsToGain(-1.0f));
        
        presence_.prepare(spec);
        *presence_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            spec.sampleRate, 3000.0f, 1.0f, juce::Decibels::decibelsToGain(1.5f));
        
        airBand_.prepare(spec);
        *airBand_.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            spec.sampleRate, 10000.0f, 0.7f, juce::Decibels::decibelsToGain(2.0f));
    }
    
    void reset() {
        highPass_.reset();
        lowShelf_.reset();
        midCut_.reset();
        presence_.reset();
        airBand_.reset();
    }
    
    void setSettings(const Settings& settings) { settings_ = settings; }
    
    template<typename ProcessContext>
    void process(const ProcessContext& context) {
        if (!settings_.enabled) return;
        highPass_.process(context);
        lowShelf_.process(context);
        midCut_.process(context);
        presence_.process(context);
        airBand_.process(context);
    }
    
private:
    Settings settings_;
    juce::dsp::StateVariableTPTFilter<float> highPass_;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, 
                                   juce::dsp::IIR::Coefficients<float>> lowShelf_;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, 
                                   juce::dsp::IIR::Coefficients<float>> midCut_;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, 
                                   juce::dsp::IIR::Coefficients<float>> presence_;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, 
                                   juce::dsp::IIR::Coefficients<float>> airBand_;
};

//==============================================================================
/**
    Glue Compressor for master bus
*/
class MasteringCompressor {
public:
    struct Settings {
        float threshold = -12.0f;
        float ratio = 2.0f;
        float attack = 30.0f;
        float release = 200.0f;
        bool enabled = true;
    };
    
    MasteringCompressor() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) {
        compressor_.prepare(spec);
        compressor_.setThreshold(-12.0f);
        compressor_.setRatio(2.0f);
        compressor_.setAttack(30.0f);
        compressor_.setRelease(200.0f);
    }
    
    void reset() { compressor_.reset(); }
    void setSettings(const Settings& settings) { settings_ = settings; }
    
    template<typename ProcessContext>
    void process(const ProcessContext& context) {
        if (!settings_.enabled) return;
        compressor_.process(context);
    }
    
private:
    Settings settings_;
    juce::dsp::Compressor<float> compressor_;
};

//==============================================================================
/**
    Real-time Brickwall Limiter for mastering
*/
class MasteringLimiter {
public:
    MasteringLimiter() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate_ = spec.sampleRate;
        lookaheadSamples_ = static_cast<int>(sampleRate_ * 0.005);
        lookaheadBuffer_.setSize(static_cast<int>(spec.numChannels), lookaheadSamples_ + 1);
        lookaheadBuffer_.clear();
        lookaheadPos_ = 0;
        envelope_ = 1.0f;
        attackCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.001f));
        releaseCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.100f));
    }
    
    void reset() {
        lookaheadBuffer_.clear();
        lookaheadPos_ = 0;
        envelope_ = 1.0f;
    }
    
    void setCeiling(float ceilingDb) {
        ceiling_ = juce::Decibels::decibelsToGain(ceilingDb);
    }
    
    template<typename ProcessContext>
    void process(const ProcessContext& context) {
        auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        auto numChannels = inputBlock.getNumChannels();
        auto numSamples = inputBlock.getNumSamples();
        
        for (size_t sample = 0; sample < numSamples; ++sample) {
            float peak = 0.0f;
            for (size_t ch = 0; ch < numChannels; ++ch) {
                float absVal = std::abs(inputBlock.getSample(static_cast<int>(ch), static_cast<int>(sample)));
                peak = std::max(peak, absVal);
            }
            
            float targetGain = (peak > ceiling_) ? ceiling_ / peak : 1.0f;
            
            if (targetGain < envelope_) {
                envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * targetGain;
            } else {
                envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * targetGain;
            }
            
            for (size_t ch = 0; ch < numChannels; ++ch) {
                float currentSample = inputBlock.getSample(static_cast<int>(ch), static_cast<int>(sample));
                int readPos = (lookaheadPos_ + 1) % (lookaheadSamples_ + 1);
                float delayedSample = lookaheadBuffer_.getSample(static_cast<int>(ch), readPos);
                lookaheadBuffer_.setSample(static_cast<int>(ch), lookaheadPos_, currentSample);
                outputBlock.setSample(static_cast<int>(ch), static_cast<int>(sample), delayedSample * envelope_);
            }
            
            lookaheadPos_ = (lookaheadPos_ + 1) % (lookaheadSamples_ + 1);
        }
    }
    
private:
    double sampleRate_ = 44100.0;
    float ceiling_ = 0.99f;
    float envelope_ = 1.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    juce::AudioBuffer<float> lookaheadBuffer_;
    int lookaheadSamples_ = 0;
    int lookaheadPos_ = 0;
};

//==============================================================================
/**
    Main AI Mastering Agent - performs actual DSP processing with Grok AI
*/
class AIMasteringAgent {
public:
    struct Options {
        bool useAI = true;
        bool autoBalance = true;
        float targetLoudness = -14.0f;
        juce::String userIntent = "balanced, professional master";
    };

    explicit AIMasteringAgent(Engine& engine);
    ~AIMasteringAgent();
    
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void analyzeAndConfigure(const juce::AudioBuffer<float>& analysisBuffer, const Options& options);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();
    
    void setBypass(bool bypass) { bypassed_.store(bypass); }
    bool isBypassed() const { return bypassed_.load(); }
    bool isConfigured() const { return isConfigured_.load(); }
    
    GrokMasteringAI::MasteringDecision getLastDecision() const {
        const juce::ScopedLock lock(decisionLock_);
        return lastDecision_;
    }

private:
    void applyAIDecision(const GrokMasteringAI::MasteringDecision& decision);
    void balanceTracks(const Options& options);
    
    Engine& engine_;
    double sampleRate_ = 44100.0;
    
    // AI components
    AudioFeatureExtractor featureExtractor_;
    GrokMasteringAI grokAI_;
    
    // DSP processors
    MasteringEQ eq_;
    MasteringCompressor compressor_;
    MasteringLimiter limiter_;
    
    // State
    std::atomic<bool> isPrepared_{false};
    std::atomic<bool> isConfigured_{false};
    std::atomic<bool> bypassed_{false};
    
    mutable juce::CriticalSection decisionLock_;
    GrokMasteringAI::MasteringDecision lastDecision_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIMasteringAgent)
};

} // namespace ai
} // namespace zenith
