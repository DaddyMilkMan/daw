/*
  ==============================================================================
    apps/desktop/Source/ai/AIMasteringAgent.h
    Automated mixing and mastering agent with real DSP.
  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Engine.h"
#include "../engine/Track.h"
#include "GrokAPIClient.h"

namespace zenith {
namespace ai {

//==============================================================================
/**
    Real-time Brickwall Limiter for mastering
*/
class MasteringLimiter {
public:
    MasteringLimiter() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate_ = spec.sampleRate;
        
        // Lookahead delay buffer (5ms lookahead)
        lookaheadSamples_ = static_cast<int>(sampleRate_ * 0.005);
        lookaheadBuffer_.setSize(static_cast<int>(spec.numChannels), lookaheadSamples_ + 1);
        lookaheadBuffer_.clear();
        lookaheadPos_ = 0;
        
        // Envelope follower
        envelope_ = 0.0f;
        attackCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.001f)); // 1ms attack
        releaseCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.100f)); // 100ms release
    }
    
    void reset() {
        lookaheadBuffer_.clear();
        lookaheadPos_ = 0;
        envelope_ = 0.0f;
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
            // Find peak across all channels
            float peak = 0.0f;
            for (size_t ch = 0; ch < numChannels; ++ch) {
                float absVal = std::abs(inputBlock.getSample(static_cast<int>(ch), static_cast<int>(sample)));
                peak = std::max(peak, absVal);
            }
            
            // Calculate gain reduction needed
            float targetGain = (peak > ceiling_) ? ceiling_ / peak : 1.0f;
            
            // Smooth the gain (envelope follower)
            if (targetGain < envelope_) {
                envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * targetGain;
            } else {
                envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * targetGain;
            }
            
            // Apply gain with lookahead
            for (size_t ch = 0; ch < numChannels; ++ch) {
                // Store current sample in lookahead buffer
                float currentSample = inputBlock.getSample(static_cast<int>(ch), static_cast<int>(sample));
                
                // Get delayed sample
                int readPos = (lookaheadPos_ + 1) % (lookaheadSamples_ + 1);
                float delayedSample = lookaheadBuffer_.getSample(static_cast<int>(ch), readPos);
                
                // Write current sample to buffer
                lookaheadBuffer_.setSample(static_cast<int>(ch), lookaheadPos_, currentSample);
                
                // Output limited signal
                outputBlock.setSample(static_cast<int>(ch), static_cast<int>(sample), 
                                      delayedSample * envelope_);
            }
            
            lookaheadPos_ = (lookaheadPos_ + 1) % (lookaheadSamples_ + 1);
        }
    }
    
private:
    double sampleRate_ = 44100.0;
    float ceiling_ = 0.99f;  // Just below 0dB
    float envelope_ = 1.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    
    juce::AudioBuffer<float> lookaheadBuffer_;
    int lookaheadSamples_ = 0;
    int lookaheadPos_ = 0;
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
        spec_ = spec;
        // High-pass filter (30Hz) to remove rumble
        highPass_.prepare(spec);
        highPass_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highPass_.setCutoffFrequency(30.0f);
        
        lowShelf_.prepare(spec);
        midCut_.prepare(spec);
        presence_.prepare(spec);
        airBand_.prepare(spec);
        
        updateFilters();
    }
    
    void reset() {
        highPass_.reset();
        lowShelf_.reset();
        midCut_.reset();
        presence_.reset();
        airBand_.reset();
    }
    
    void setSettings(const Settings& settings) {
        settings_ = settings;
        updateFilters();
    }

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
    void updateFilters() {
        if (spec_.sampleRate <= 0) return;

        *lowShelf_.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            spec_.sampleRate, 100.0f, 0.7f, juce::Decibels::decibelsToGain(settings_.lowShelfGain));
        
        *midCut_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            spec_.sampleRate, 300.0f, 1.0f, juce::Decibels::decibelsToGain(settings_.midCutGain));
        
        *presence_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            spec_.sampleRate, 3000.0f, 1.0f, juce::Decibels::decibelsToGain(settings_.presenceGain));
        
        *airBand_.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            spec_.sampleRate, 10000.0f, 0.7f, juce::Decibels::decibelsToGain(settings_.airGain));
    }

    juce::dsp::ProcessSpec spec_;
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
        updateCompressor();
    }
    
    void reset() {
        compressor_.reset();
    }
    
    void setSettings(const Settings& settings) {
        settings_ = settings;
        updateCompressor();
    }
    
    void setAmount(float amount) {
        // amount 0-1 controls mix and threshold
        settings_.threshold = -6.0f - (amount * 12.0f); // -6dB to -18dB
        settings_.ratio = 1.5f + (amount * 2.5f);       // 1.5:1 to 4:1
        updateCompressor();
    }
    
    template<typename ProcessContext>
    void process(const ProcessContext& context) {
        if (!settings_.enabled) return;
        compressor_.process(context);
    }
    
private:
    void updateCompressor() {
        compressor_.setThreshold(settings_.threshold);
        compressor_.setRatio(settings_.ratio);
        compressor_.setAttack(settings_.attack);
        compressor_.setRelease(settings_.release);
    }

    Settings settings_;
    juce::dsp::Compressor<float> compressor_;
};

//==============================================================================
/**
    Extracts audio features for AI analysis
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
    Grok AI Integration for Mastering Decisions
*/
class GrokMasteringAI {
public:
    struct EQSettings {
        float lowShelfGain = 0.0f;
        float midCutGain = 0.0f;
        float presenceGain = 0.0f;
        float airGain = 0.0f;
    };

    struct CompressionSettings {
        float threshold = 0.0f;
        float ratio = 1.0f;
        float attack = 10.0f;
        float release = 100.0f;
    };

    struct MasteringDecision {
        EQSettings eq;
        CompressionSettings compression;
        float limiterCeiling = -0.1f;
        juce::String reasoning;
        bool valid = false;
    };

    GrokMasteringAI();
    
    MasteringDecision getMasteringDecision(const AudioFeatureExtractor::Features& features, 
                                          const juce::String& userIntent, 
                                          float targetLoudness);

private:
    std::unique_ptr<GrokAPIClient> grokClient_;
    juce::String buildPrompt(const AudioFeatureExtractor::Features& features, 
                           const juce::String& userIntent, 
                           float targetLoudness);
    MasteringDecision parseGrokResponse(const juce::String& response);
};

//==============================================================================
/**
    Main AI Mastering Agent - performs actual DSP processing and AI coordination
*/
class AIMasteringAgent
{
public:
    struct Options
    {
        bool autoBalance = true;     
        bool useAI = true;     
        juce::String userIntent = "Balanced and punchy";
        float targetLoudness = -14.0f;
    };

    explicit AIMasteringAgent(Engine& engine);
    ~AIMasteringAgent();
    
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    // Process real-time audio
    void processBlock(juce::AudioBuffer<float>& buffer);

    // Analysis and Configuration
    void analyzeAndConfigure(const juce::AudioBuffer<float>& analysisBuffer, const Options& options);

private:
    Engine& engine_;
    
    // DSP processors
    MasteringEQ eq_;
    MasteringCompressor compressor_;
    MasteringLimiter limiter_;
    
    // AI Components
    AudioFeatureExtractor featureExtractor_;
    GrokMasteringAI grokAI_;
    
    // State
    std::atomic<bool> isPrepared_ { false };
    std::atomic<bool> isConfigured_ { false };
    std::atomic<bool> bypassed_ { false };
    double sampleRate_ = 44100.0;
    
    juce::CriticalSection decisionLock_;
    GrokMasteringAI::MasteringDecision lastDecision_;

    // Helpers
    void applyAIDecision(const GrokMasteringAI::MasteringDecision& decision);
    void balanceTracks(const Options& options);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIMasteringAgent)
};

} // namespace ai
} // namespace zenith
