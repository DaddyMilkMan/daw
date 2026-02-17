/*
    PresetRandomizer.h - Intelligent Preset Randomization for Zenith

    Priority 4: Polish & Workflow

    Features:
    - Musical randomization with intelligent ranges
    - Lockable parameters for controlled randomization
    - Smart variations (Warmer, Brighter, etc.)
    - Preset templates (Conservative, Creative, Experimental)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#ifndef ZENITH_PRESET_RANDOMIZER_H
#define ZENITH_PRESET_RANDOMIZER_H

#include "../HandCraftedPresets.cpp"
#include <random>
#include <cmath>

namespace zenith {

// Randomization configuration - which parameters to lock
struct RandomizationConfig {
    // Oscillator locks
    bool lockOsc1Wave = false;
    bool lockOsc1Mix = false;
    bool lockOsc1Detune = false;
    bool lockOsc2Wave = false;
    bool lockOsc2Mix = false;
    bool lockOsc2Detune = false;
    bool lockOsc3Wave = false;
    bool lockOsc3Mix = false;
    bool lockOsc3Detune = false;

    // Filter locks
    bool lockFilterCutoff = false;
    bool lockFilterResonance = false;
    bool lockFilterDrive = false;
    bool lockFilterEnvAmount = false;

    // Envelope locks
    bool lockAmpEnv = false;
    bool lockModEnv = false;

    // LFO locks
    bool lockLFO1 = false;
    bool lockLFO2 = false;

    // Modulation locks
    bool lockFM = false;
    bool lockRingMod = false;
    bool lockGlide = false;

    // Unison lock
    bool lockUnison = false;

    // Drift lock
    bool lockDrift = false;

    // Preset templates
    static RandomizationConfig getConservativeTemplate() {
        RandomizationConfig config;
        config.lockOsc1Wave = true;
        config.lockOsc2Wave = true;
        config.lockOsc3Wave = true;
        config.lockFilterCutoff = true;
        return config;
    }

    static RandomizationConfig getCreativeTemplate() {
        RandomizationConfig config;
        // Only lock basic structure
        config.lockOsc1Wave = true;
        return config;
    }

    static RandomizationConfig getExperimentalTemplate() {
        RandomizationConfig config;
        // Unlock everything for maximum chaos
        return config;
    }
};

class PresetRandomizer {
public:
    // Generate random preset with musical randomization
    static HandCraftedPreset generateRandomPreset(
        const RandomizationConfig& config,
        float randomizationAmount = 0.5f  // 0.0 = subtle, 1.0 = extreme
    );

    // Smart variations of current preset
    static HandCraftedPreset makeWarmer(const HandCraftedPreset& preset);
    static HandCraftedPreset makeBrighter(const HandCraftedPreset& preset);
    static HandCraftedPreset makeMoreAggressive(const HandCraftedPreset& preset);
    static HandCraftedPreset makeCalm(const HandCraftedPreset& preset);
    static HandCraftedPreset makeMoreEvolving(const HandCraftedPreset& preset);

    // Randomize specific parameter group
    static HandCraftedPreset randomizeOscillators(
        const HandCraftedPreset& preset,
        float amount,
        const RandomizationConfig& config
    );

    static HandCraftedPreset randomizeFilter(
        const HandCraftedPreset& preset,
        float amount,
        const RandomizationConfig& config
    );

    static HandCraftedPreset randomizeEnvelopes(
        const HandCraftedPreset& preset,
        float amount,
        const RandomizationConfig& config
    );

private:
    static std::random_device rd;
    static std::mt19937 gen;

    // Random number generators
    static float randomFloat(float min = 0.0f, float max = 1.0f);
    static int randomInt(int min, int max);

    // Musical randomization functions
    static OscillatorWaveform randomWaveform();
    static float randomFilterCutoff(float current, float amount);
    static float randomResonance(float current, float amount);
    static float randomAttackTime(float current, float amount);
    static float randomDecayTime(float current, float amount);
    static float randomLfoRate(float current, float amount);
    static float randomDetune(float current, float amount);

    // Helper functions
    static float lerp(float a, float b, float t);
    static float randomLog(float min, float max);
};

} // namespace zenith

#endif // ZENITH_PRESET_RANDOMIZER_H
