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
    MasteringEQ() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) {
        // High-pass filter (30Hz) to remove rumble
        highPass_.prepare(spec);
        highPass_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highPass_.setCutoffFrequency(30.0f);
        
        // Low shelf for warmth (100Hz, +1dB)
        lowShelf_.prepare(spec);
        *lowShelf_.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            spec.sampleRate, 100.0f, 0.7f, juce::Decibels::decibelsToGain(1.0f));
        
        // Mid cut for clarity (300Hz, -1dB)
        midCut_.prepare(spec);
        *midCut_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            spec.sampleRate, 300.0f, 1.0f, juce::Decibels::decibelsToGain(-1.0f));
        
        // Presence boost (3kHz, +1.5dB)
        presence_.prepare(spec);
        *presence_.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            spec.sampleRate, 3000.0f, 1.0f, juce::Decibels::decibelsToGain(1.5f));
        
        // Air band (10kHz, +2dB high shelf)
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
    
    template<typename ProcessContext>
    void process(const ProcessContext& context) {
        highPass_.process(context);
        lowShelf_.process(context);
        midCut_.process(context);
        presence_.process(context);
        airBand_.process(context);
    }
    
private:
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
class GlueCompressor {
public:
    GlueCompressor() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec) {
        compressor_.prepare(spec);
        
        // Gentle glue compression settings
        compressor_.setThreshold(-12.0f);  // -12dB threshold
        compressor_.setRatio(2.0f);        // 2:1 ratio (gentle)
        compressor_.setAttack(30.0f);      // 30ms attack (slow, lets transients through)
        compressor_.setRelease(200.0f);    // 200ms release (musical)
    }
    
    void reset() {
        compressor_.reset();
    }
    
    void setAmount(float amount) {
        // amount 0-1 controls mix and threshold
        compressor_.setThreshold(-6.0f - (amount * 12.0f)); // -6dB to -18dB
        compressor_.setRatio(1.5f + (amount * 2.5f));       // 1.5:1 to 4:1
    }
    
    template<typename ProcessContext>
    void process(const ProcessContext& context) {
        compressor_.process(context);
    }
    
private:
    juce::dsp::Compressor<float> compressor_;
};

//==============================================================================
/**
    Main AI Mastering Agent - performs actual DSP processing
*/
class AIMasteringAgent
{
public:
    struct MasteringOptions
    {
        bool autoLevelMix = true;     // Balance track volumes
        bool applyEq = true;          // Apply mastering EQ
        bool applyCompression = true; // Apply glue compression
        bool applyLimiter = true;     // Brickwall limit
        bool target8Bit = false;      // Optimize for 8-bit export
        float targetLufs = -14.0f;    // Target loudness (streaming standard)
        float compressionAmount = 0.5f; // 0-1
    };

    explicit AIMasteringAgent(Engine& engine) : engine_(engine) {}
    
    /**
     * @brief Prepare DSP processors
     */
    void prepare(double sampleRate, int samplesPerBlock, int numChannels) {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
        spec.numChannels = static_cast<juce::uint32>(numChannels);
        
        eq_.prepare(spec);
        compressor_.prepare(spec);
        limiter_.prepare(spec);
        
        isPrepared_ = true;
        DBG("AIMasteringAgent: DSP chain prepared");
    }
    
    /**
     * @brief Run the AI Mastering process on project tracks
     */
    void runMasteringPass(const MasteringOptions& options) {
        DBG("AIMasteringAgent: Starting mastering pass...");

        if (options.autoLevelMix) {
            performAutoMixing();
        }
        
        // Store options for real-time processing
        currentOptions_ = options;
        
        // Configure processors
        if (options.applyCompression) {
            compressor_.setAmount(options.compressionAmount);
        }
        
        if (options.applyLimiter) {
            // Set ceiling based on target
            float ceiling = options.target8Bit ? -0.5f : -0.1f;
            limiter_.setCeiling(ceiling);
        }
        
        masteredSuccessfully_ = true;
        DBG("AIMasteringAgent: Mastering pass complete");
    }
    
    /**
     * @brief Process audio buffer through mastering chain
     * 
     * Call this from the audio callback to apply real-time mastering.
     */
    void processBlock(juce::AudioBuffer<float>& buffer) {
        if (!isPrepared_ || !masteredSuccessfully_) return;
        
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        
        // Apply EQ
        if (currentOptions_.applyEq) {
            eq_.process(context);
        }
        
        // Apply compression
        if (currentOptions_.applyCompression) {
            compressor_.process(context);
        }
        
        // Apply limiting
        if (currentOptions_.applyLimiter) {
            limiter_.process(context);
        }
    }
    
    /**
     * @brief Process audio buffer and return mastered version
     * 
     * For offline/export processing.
     */
    juce::AudioBuffer<float> masterOffline(const juce::AudioBuffer<float>& input,
                                            double sampleRate,
                                            const MasteringOptions& options) {
        juce::AudioBuffer<float> output(input);
        
        // Prepare if needed
        if (!isPrepared_) {
            prepare(sampleRate, input.getNumSamples(), input.getNumChannels());
        }
        
        // Configure for this pass
        runMasteringPass(options);
        
        // Process
        processBlock(output);
        
        // Apply loudness normalization
        if (options.targetLufs != 0.0f) {
            normalizeLoudness(output, options.targetLufs);
        }
        
        return output;
    }
    
    void reset() {
        eq_.reset();
        compressor_.reset();
        limiter_.reset();
    }

private:
    Engine& engine_;
    
    // DSP processors
    MasteringEQ eq_;
    GlueCompressor compressor_;
    MasteringLimiter limiter_;
    
    bool isPrepared_ = false;
    bool masteredSuccessfully_ = false;
    MasteringOptions currentOptions_;

    void performAutoMixing() {
        // Analyze tracks and balance levels
        DBG("AIMasteringAgent: Analyzing track levels...");
        
        auto& tracks = engine_.tracks();
        if (tracks.empty()) return;
        
        // Phase 1: Set all tracks to -6dB as baseline
        for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
            auto track = tracks[i];
            if (!track) continue;
            
            // Get current peak from meter (if available)
            float currentLevel = engine_.getTrackLevel(i);
            
            // Target: -18dBFS peak with headroom
            // If track is too hot, reduce it
            if (currentLevel > 0.5f) {
                engine_.setTrackVolume(i, 0.5f);  // -6dB
                DBG("AIMasteringAgent: Reduced track " + juce::String(i) + " to -6dB");
            }
            // If track is too quiet, bring it up
            else if (currentLevel < 0.1f && currentLevel > 0.0f) {
                engine_.setTrackVolume(i, 0.7f);  // Boost
                DBG("AIMasteringAgent: Boosted track " + juce::String(i));
            }
        }
        
        DBG("AIMasteringAgent: Auto-mix complete");
    }
    
    void normalizeLoudness(juce::AudioBuffer<float>& buffer, float targetLufs) {
        // Simple RMS-based loudness estimation
        float rms = 0.0f;
        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                rms += data[i] * data[i];
            }
        }
        rms = std::sqrt(rms / (numSamples * numChannels));
        
        // Convert RMS to approximate LUFS (rough estimation)
        float currentLufs = 20.0f * std::log10(rms) - 10.0f;
        float gainDb = targetLufs - currentLufs;
        
        // Clamp gain adjustment to reasonable range
        gainDb = juce::jlimit(-12.0f, 12.0f, gainDb);
        
        float gainLinear = juce::Decibels::decibelsToGain(gainDb);
        
        // Apply gain
        buffer.applyGain(gainLinear);
        
        DBG("AIMasteringAgent: Normalized loudness by " + juce::String(gainDb, 1) + "dB");
    }
};

} // namespace ai
} // namespace zenith
