/*
    MultiModeDelay.h - Professional Multi-Mode Delay Effect

    Complete delay suite featuring:
    - Tape delay with wow/flutter/age modeling
    - BBD (Bucket Brigade Device) emulation
    - Digital delay with sub-sample precision
    - Ping-pong stereo delay
    - Reverse delay (NEW)
    - Filter delay with advanced filtering (NEW)
    - Ducking delay (sidechain)
    - Slapback delay

    Designed to compete with:
    - Soundtoys EchoBoy ($199)
    - Valhalla Delay ($50)
    - Waves H-Delay ($129)
    - FabFilter Timeless 3 ($169)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#pragma once

#include "../engine/EffectProcessor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <array>

namespace zenith {
namespace engine {

//==============================================================================
// DELAY MODE ENUM
//==============================================================================

/**
 * @enum DelayMode
 * @brief Complete collection of professional delay modes
 */
enum class DelayMode
{
    //==========================================================================
    // Classic Modes (Existing)
    //==========================================================================
    Tape,           // Analog tape with wow/flutter
    BBD,            // Bucket Brigade Device (warmer, filtered)
    Digital,        // Clean digital delay
    PingPong,       // Stereo ping-pong

    //==========================================================================
    // Advanced Modes (NEW)
    //==========================================================================
    Reverse,        // Reverse playback (psychedelic)
    Filter,         // Filter delay with separate input/feedback filters
    Ducking,        // Auto-ducking based on input (slapback)
    Slapback,       // Short single echo (rockabilly)
    TapeVintage,    // Aged tape with wear/noise
    MultiTap,       // Multi-tap rhythmic delay
    PingPongFilter  // Ping-pong with filtering
};

//==============================================================================
// PROFESSIONAL DELAY SUITE
//==============================================================================

/**
 * @class MultiModeDelay
 * @brief Complete professional delay effect
 *
 * Features:
 * - 8 delay modes including tape, BBD, digital, reverse, filter
 * - Tape emulation: wow, flutter, wear, age, saturation
 * - Advanced filtering: separate input/feedback filters (LP, HP, BP)
 * - Ducking delay (sidechain compression for clean slapback)
 * - LFO modulation for chorus effects
 * - Ping-pong stereo with pan position
 * - Sub-sample delay precision (Lagrange3rd interpolation)
 * - Tempo sync (quarter, eighth, triplet, dotted)
 * - Tap tempo
 * - Presets for classic sounds
 *
 * Designed to compete with:
 * - Soundtoys EchoBoy ($199)
 * - Valhalla Delay ($50)
 * - Waves H-Delay ($129)
 * - FabFilter Timeless 3 ($169)
 */
class MultiModeDelay : public EffectProcessor
{
public:
    //==============================================================================
    MultiModeDelay();
    ~MultiModeDelay() override;

    //==============================================================================
    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr) override;

    //==============================================================================
    juce::String getName() const override { return "Multi-Mode Delay"; }
    EffectType getType() const override { return EffectType::Delay; }

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    //==============================================================================
    // Delay mode
    void setDelayMode(DelayMode mode);
    DelayMode getDelayMode() const { return delayMode_.load(); }

    //==============================================================================
    // Main controls
    void setTime(float timeSeconds);  // 0.0 to 2.0 seconds
    float getTime() const { return delayTime_.load(); }

    void setFeedback(float feedback);  // 0.0 to 0.95
    float getFeedback() const { return feedback_.load(); }

    void setMix(float mix);  // 0.0 to 1.0
    float getMix() const { return mix_.load(); }

    //==============================================================================
    // Tape emulation controls
    void setWow(float wow);  // 0.0 to 1.0 (tape speed variation)
    float getWow() const { return wow_.load(); }

    void setFlutter(float flutter);  // 0.0 to 1.0 (high-frequency speed variation)
    float getFlutter() const { return flutter_.load(); }

    void setTapeAge(float age);  // 0.0 to 1.0 (deterioration)
    float getTapeAge() const { return tapeAge_.load(); }

    void setTapeWear(float wear);  // 0.0 to 1.0 (high-frequency loss)
    float getTapeWear() const { return tapeWear_.load(); }

    void setSaturation(float saturation);  // 0.0 to 1.0
    float getSaturation() const { return saturation_.load(); }

    //==============================================================================
    // Filter controls (for FilterDelay mode)
    enum class FilterType { LowPass, HighPass, BandPass, Off };

    void setInputFilterType(FilterType type);
    FilterType getInputFilterType() const { return inputFilterType_.load(); }

    void setInputFilterFrequency(float Hz);
    float getInputFilterFrequency() const { return inputFilterFreq_.load(); }

    void setInputFilterResonance(float Q);
    float getInputFilterResonance() const { return inputFilterResonance_.load(); }

    void setFeedbackFilterType(FilterType type);
    FilterType getFeedbackFilterType() const { return feedbackFilterType_.load(); }

    void setFeedbackFilterFrequency(float Hz);
    float getFeedbackFilterFrequency() const { return feedbackFilterFreq_.load(); }

    void setFeedbackFilterResonance(float Q);
    float getFeedbackFilterResonance() const { return feedbackFilterResonance_.load(); }

    //==============================================================================
    // Ducking (for DuckingDelay mode)
    void setDuckingEnabled(bool enabled);
    bool isDuckingEnabled() const { return duckingEnabled_.load(); }

    void setDuckingThreshold(float dB);  // Threshold to trigger ducking
    float getDuckingThreshold() const { return duckingThreshold_.load(); }

    void setDuckingRatio(float ratio);  // How much to duck
    float getDuckingRatio() const { return duckingRatio_.load(); }

    void setDuckingAttack(float ms);
    float getDuckingAttack() const { return duckingAttack_.load(); }

    void setDuckingRelease(float ms);
    float getDuckingRelease() const { return duckingRelease_.load(); }

    //==============================================================================
    // LFO modulation
    void setModulationEnabled(bool enabled);
    bool isModulationEnabled() const { return modulationEnabled_.load(); }

    void setModulationRate(float Hz);  // 0.1 to 10 Hz
    float getModulationRate() const { return modulationRate_.load(); }

    void setModulationDepth(float depth);  // 0.0 to 1.0
    float getModulationDepth() const { return modulationDepth_.load(); }

    //==============================================================================
    // Stereo controls
    void setPingPongEnabled(bool enabled);
    bool isPingPongEnabled() const { return pingPongEnabled_.load(); }

    void setStereoWidth(float width);  // 0.0 (mono) to 1.0 (wide)
    float getStereoWidth() const { return stereoWidth_.load(); }

    //==============================================================================
    // Tempo sync
    void setTempoSyncEnabled(bool enabled);
    bool isTempoSyncEnabled() const { return tempoSyncEnabled_.load(); }

    enum class TempoSync {
        Quarter,      // 1/4 note
        DottedQuarter,// 1/4 dotted
        Eighth,       // 1/8 note
        DottedEighth, // 1/8 dotted
        TripletEighth,// 1/8 triplet
        Sixteenth,    // 1/16 note
        ThirtySecond  // 1/32 note
    };

    void setTempoSync(TempoSync sync);
    TempoSync getTempoSync() const { return tempoSync_.load(); }

    void setTempo(double bpm);  // For tempo sync calculation
    double getTempo() const { return tempo_.load(); }

    //==============================================================================
    // Tap tempo
    void tapTempo();
    double getCalculatedTempo() const;

    //==============================================================================
    // Presets
    void loadPreset(const juce::String& presetName);
    static juce::StringArray getPresetNames();

    //==============================================================================
    // Metering
    float getInputLevel() const { return inputLevel_.load(); }
    float getOutputLevel() const { return outputLevel_.load(); }

private:
    //==============================================================================
    // Delay processing per channel
    float processChannel(float input, int channel);
    float processDelaySample(float input, int channel);

    // Mode-specific processing
    float processTapeDelay(float input, int channel);
    float processBBDDelay(float input, int channel);
    float processDigitalDelay(float input, int channel);
    float processPingPongDelay(float input, int channel);
    float processReverseDelay(float input, int channel);
    float processFilterDelay(float input, int channel);
    float processDuckingDelay(float input, int channel);
    float processSlapbackDelay(float input, int channel);
    float processTapeVintageDelay(float input, int channel);
    float processMultiTapDelay(float input, int channel);
    float processPingPongFilterDelay(float input, int channel);

    // Tape emulation
    float applyTapeSaturation(float sample);
    float applyWowFlutter(float sample, int channel);
    float applyTapeAge(float sample);
    float applyTapeWear(float sample);

    // Filter processing
    float applyInputFilter(float sample);
    float applyFeedbackFilter(float sample);
    void updateFilters();

    // Ducking
    float applyDucking(float sample);
    void updateDuckingEnvelope(float inputLevel);

    // Modulation
    void updateModulation();

    // Tempo sync calculation
    float calculateSyncedTime();
    void updateDelayTimeFromSync();

    //==============================================================================
    // Parameters (atomic for thread safety)
    std::atomic<DelayMode> delayMode_{DelayMode::Tape};
    std::atomic<float> delayTime_{0.5f};
    std::atomic<float> feedback_{0.5f};
    std::atomic<float> mix_{0.3f};

    // Tape parameters
    std::atomic<float> wow_{0.0f};
    std::atomic<float> flutter_{0.0f};
    std::atomic<float> tapeAge_{0.0f};
    std::atomic<float> tapeWear_{0.0f};
    std::atomic<float> saturation_{0.3f};

    // Filter parameters
    std::atomic<FilterType> inputFilterType_{FilterType::LowPass};
    std::atomic<float> inputFilterFreq_{4000.0f};
    std::atomic<float> inputFilterResonance_{0.7f};

    std::atomic<FilterType> feedbackFilterType_{FilterType::LowPass};
    std::atomic<float> feedbackFilterFreq_{2000.0f};
    std::atomic<float> feedbackFilterResonance_{0.7f};

    // Ducking parameters
    std::atomic<bool> duckingEnabled_{false};
    std::atomic<float> duckingThreshold_{-20.0f};
    std::atomic<float> duckingRatio_{0.5f};
    std::atomic<float> duckingAttack_{10.0f};
    std::atomic<float> duckingRelease_{100.0f};
    std::atomic<float> duckingEnvelope_{0.0f};

    // Modulation parameters
    std::atomic<bool> modulationEnabled_{false};
    std::atomic<float> modulationRate_{0.5f};
    std::atomic<float> modulationDepth_{0.1f};
    float lfoPhase_[2] = {0.0f, 0.0f};

    // Stereo parameters
    std::atomic<bool> pingPongEnabled_{false};
    std::atomic<float> stereoWidth_{1.0f};

    // Tempo sync parameters
    std::atomic<bool> tempoSyncEnabled_{false};
    std::atomic<TempoSync> tempoSync_{TempoSync::Eighth};
    std::atomic<double> tempo_{120.0};

    // Metering
    std::atomic<float> inputLevel_{-100.0f};
    std::atomic<float> outputLevel_{-100.0f};

    //==============================================================================
    // DSP components

    // Delay lines (stereo)
    struct DelayLine
    {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
        std::vector<float> buffer;

        void prepare(double sampleRate, int maxSamples)
        {
            buffer.resize(maxSamples);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            this->delayLine.prepare({sampleRate, static_cast<juce::uint32>(maxSamples), 1});
        }

        void reset()
        {
            this->delayLine.reset();
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
    };

    std::array<DelayLine, 2> delayLines_;

    // Additional delay line for reverse processing
    DelayLine reverseBuffer_;

    // Multi-tap delays (8 taps)
    struct MultiTapDelay
    {
        float time = 0.25f;
        float gain = 0.5f;
        float pan = 0.5f;  // 0 = left, 1 = right
    };
    std::array<MultiTapDelay, 8> multiTaps_;

    // Filters
    struct FilterPair
    {
        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> low;
        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> high;
        juce::dsp::ProcessorDuplicator<
            juce::dsp::IIR::Filter<float>,
            juce::dsp::IIR::Coefficients<float>> band;

        void prepare(double sampleRate, int maxSamples)
        {
            juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(maxSamples), 2};
            low.prepare(spec);
            high.prepare(spec);
            band.prepare(spec);
        }

        void reset()
        {
            low.reset();
            high.reset();
            band.reset();
        }
    };

    FilterPair inputFilters_;
    FilterPair feedbackFilters_;

    // LFO for modulation
    juce::dsp::Oscillator<float> lfo_;

    // Sample rate
    double sampleRate_ = 44100.0;
    int maxDelaySamples_ = 0;

    // Tap tempo
    struct TapTempo
    {
        static constexpr int maxTaps = 4;
        std::array<double, maxTaps> tapTimes_{};
        int currentTap_ = 0;
        double calculatedTempo_ = 120.0;
    } tapTempo_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiModeDelay)
};

} // namespace engine
} // namespace zenith
