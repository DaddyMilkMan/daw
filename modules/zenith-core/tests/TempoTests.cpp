#include <iostream>
#include <cmath>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../include/ProjectState.h"

namespace
{
/**
 * @brief Test tempo clamping behavior
 * Tempo should be clamped to range [20.0, 999.0]
 */
bool testTempoClamping()
{
    ProjectState state;

    // Test lower bound clamping
    state.setTempo(10.0);  // Below minimum
    if (state.getTempo() != 20.0)
    {
        std::cerr << "Failed: Tempo below minimum not clamped to 20.0, got: "
                  << state.getTempo() << std::endl;
        return false;
    }

    // Test upper bound clamping
    state.setTempo(1000.0);  // Above maximum
    if (state.getTempo() != 999.0)
    {
        std::cerr << "Failed: Tempo above maximum not clamped to 999.0, got: "
                  << state.getTempo() << std::endl;
        return false;
    }

    // Test valid range
    state.setTempo(120.0);
    if (state.getTempo() != 120.0)
    {
        std::cerr << "Failed: Valid tempo not preserved, expected 120.0, got: "
                  << state.getTempo() << std::endl;
        return false;
    }

    // Test edge cases
    state.setTempo(20.0);  // Exactly at minimum
    if (state.getTempo() != 20.0)
    {
        std::cerr << "Failed: Tempo at minimum boundary, expected 20.0, got: "
                  << state.getTempo() << std::endl;
        return false;
    }

    state.setTempo(999.0);  // Exactly at maximum
    if (state.getTempo() != 999.0)
    {
        std::cerr << "Failed: Tempo at maximum boundary, expected 999.0, got: "
                  << state.getTempo() << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Test BPM to beat position calculation
 * Formula: beats = (samples / sampleRate) * (tempo / 60.0)
 * Inverse: samplesPerBeat = sampleRate * 60.0 / tempo
 */
bool testBPMToBeatsCalculation()
{
    const double sampleRate = 48000.0;

    // Test 120 BPM
    {
        const double tempo = 120.0;
        const double samplesPerBeat = sampleRate * 60.0 / tempo;
        const double expectedSamplesPerBeat = 48000.0 * 60.0 / 120.0;  // 24000 samples

        if (std::abs(samplesPerBeat - expectedSamplesPerBeat) > 0.01)
        {
            std::cerr << "Failed: Samples per beat at 120 BPM incorrect, expected: "
                      << expectedSamplesPerBeat << ", got: " << samplesPerBeat << std::endl;
            return false;
        }

        // Verify: at 24000 samples, we should be at exactly 1 beat
        const double samples = 24000.0;
        const double beats = (samples / sampleRate) * (tempo / 60.0);
        if (std::abs(beats - 1.0) > 0.01)
        {
            std::cerr << "Failed: Beat position at 24000 samples (120 BPM) incorrect, expected: 1.0, got: "
                      << beats << std::endl;
            return false;
        }
    }

    // Test 60 BPM (slower)
    {
        const double tempo = 60.0;
        const double samplesPerBeat = sampleRate * 60.0 / tempo;
        const double expectedSamplesPerBeat = 48000.0;  // 1 beat per second at 60 BPM

        if (std::abs(samplesPerBeat - expectedSamplesPerBeat) > 0.01)
        {
            std::cerr << "Failed: Samples per beat at 60 BPM incorrect, expected: "
                      << expectedSamplesPerBeat << ", got: " << samplesPerBeat << std::endl;
            return false;
        }
    }

    // Test 240 BPM (faster)
    {
        const double tempo = 240.0;
        const double samplesPerBeat = sampleRate * 60.0 / tempo;
        const double expectedSamplesPerBeat = 48000.0 * 60.0 / 240.0;  // 12000 samples

        if (std::abs(samplesPerBeat - expectedSamplesPerBeat) > 0.01)
        {
            std::cerr << "Failed: Samples per beat at 240 BPM incorrect, expected: "
                      << expectedSamplesPerBeat << ", got: " << samplesPerBeat << std::endl;
            return false;
        }
    }

    // Test conversion from samples to beats and back
    {
        const double tempo = 120.0;
        const juce::int64 inputSamples = 96000;  // 2 seconds at 48kHz = 4 beats at 120 BPM

        // Convert to beats
        const double beats = (inputSamples / sampleRate) * (tempo / 60.0);
        const double expectedBeats = 4.0;

        if (std::abs(beats - expectedBeats) > 0.01)
        {
            std::cerr << "Failed: Samples to beats conversion incorrect, expected: "
                      << expectedBeats << " beats, got: " << beats << std::endl;
            return false;
        }

        // Convert back to samples
        const double samplesPerBeat = sampleRate * 60.0 / tempo;
        const double calculatedSamples = beats * samplesPerBeat;

        if (std::abs(calculatedSamples - static_cast<double>(inputSamples)) > 0.01)
        {
            std::cerr << "Failed: Round-trip samples->beats->samples incorrect, expected: "
                      << inputSamples << ", got: " << calculatedSamples << std::endl;
            return false;
        }
    }

    return true;
}

/**
 * @brief Test tempo persistence (save/load)
 */
bool testTempoPersistence()
{
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile("zenith_tempo_test", ".zth", false);

    // Create and save
    {
        ProjectState original;
        original.setTempo(140.0);

        if (!original.saveToFile(tempFile))
        {
            std::cerr << "Failed to save project state with tempo" << std::endl;
            return false;
        }
    }

    // Load and verify
    ProjectState loaded;
    if (!loaded.loadFromFile(tempFile))
    {
        std::cerr << "Failed to load project state with tempo" << std::endl;
        tempFile.deleteFile();
        return false;
    }

    if (loaded.getTempo() != 140.0)
    {
        std::cerr << "Failed: Loaded tempo incorrect, expected 140.0, got: "
                  << loaded.getTempo() << std::endl;
        tempFile.deleteFile();
        return false;
    }

    tempFile.deleteFile();
    return true;
}

/**
 * @brief Test metronome interval calculation
 * At 120 BPM and 44.1kHz sample rate:
 * - 1 beat = 0.5 seconds
 * - Samples per beat = 44100 * 0.5 = 22050 samples
 */
bool testMetronomeIntervals()
{
    const double sampleRate = 44100.0;

    // Test 120 BPM metronome
    {
        const double tempo = 120.0;
        const double beatsPerSecond = tempo / 60.0;  // 2 beats per second
        const double samplesPerBeat = sampleRate / beatsPerSecond;
        const double expectedSamplesPerBeat = 22050.0;

        if (std::abs(samplesPerBeat - expectedSamplesPerBeat) > 0.01)
        {
            std::cerr << "Failed: Metronome interval at 120 BPM incorrect, expected: "
                      << expectedSamplesPerBeat << " samples, got: " << samplesPerBeat << std::endl;
            return false;
        }
    }

    // Test 60 BPM metronome (1 beat per second)
    {
        const double tempo = 60.0;
        const double beatsPerSecond = tempo / 60.0;  // 1 beat per second
        const double samplesPerBeat = sampleRate / beatsPerSecond;
        const double expectedSamplesPerBeat = 44100.0;  // Exactly 1 second

        if (std::abs(samplesPerBeat - expectedSamplesPerBeat) > 0.01)
        {
            std::cerr << "Failed: Metronome interval at 60 BPM incorrect, expected: "
                      << expectedSamplesPerBeat << " samples, got: " << samplesPerBeat << std::endl;
            return false;
        }
    }

    return true;
}

} // anonymous namespace

int main()
{
    std::cout << "Running Tempo/BPM Logic Tests..." << std::endl;
    std::cout << "================================" << std::endl;

    if (!testTempoClamping())
    {
        std::cerr << "❌ Tempo clamping test failed" << std::endl;
        return 1;
    }
    std::cout << "✓ Tempo clamping test passed" << std::endl;

    if (!testBPMToBeatsCalculation())
    {
        std::cerr << "❌ BPM to beats calculation test failed" << std::endl;
        return 1;
    }
    std::cout << "✓ BPM to beats calculation test passed" << std::endl;

    if (!testTempoPersistence())
    {
        std::cerr << "❌ Tempo persistence test failed" << std::endl;
        return 1;
    }
    std::cout << "✓ Tempo persistence test passed" << std::endl;

    if (!testMetronomeIntervals())
    {
        std::cerr << "❌ Metronome interval test failed" << std::endl;
        return 1;
    }
    std::cout << "✓ Metronome interval test passed" << std::endl;

    std::cout << "================================" << std::endl;
    std::cout << "All tempo/BPM tests passed! ✓" << std::endl;

    return 0;
}

