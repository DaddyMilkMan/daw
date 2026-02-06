/*
  ==============================================================================

    AudioRendererSimpleTest.cpp
    Created: 2026-02-05
    Author:  Zenith DAW

    Simple test for AudioRenderer functionality without full test suite dependencies.

  ==============================================================================
*/

#include "../external/JUCE/modules/juce_core/juce_core.h"
#include "../modules/zenith_core/engine/core/AudioRenderer.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting AudioRenderer Simple Test..." << std::endl;

    try {
        // Test 1: Basic initialization
        std::cout << "Test 1: Basic initialization" << std::endl;
        zenith::AudioRenderer renderer;

        // Initialize with typical configuration
        renderer.initialize(2, 2, 44100.0);
        assert(renderer.isInitialized());
        std::cout << "✓ Renderer initialized successfully" << std::endl;

        // Test 2: Volume controls
        std::cout << "Test 2: Volume controls" << std::endl;
        renderer.setMasterVolume(0.5f);
        assert(renderer.getMasterVolume() == 0.5f);
        std::cout << "✓ Volume control working" << std::endl;

        // Test 3: Mute controls
        std::cout << "Test 3: Mute controls" << std::endl;
        renderer.setMasterMute(true);
        assert(renderer.isMasterMuted());
        renderer.setMasterMute(false);
        assert(!renderer.isMasterMuted());
        std::cout << "✓ Mute control working" << std::endl;

        // Test 4: Test tone
        std::cout << "Test 4: Test tone" << std::endl;
        renderer.enableTestTone(true);
        assert(renderer.isTestToneEnabled());
        renderer.enableTestTone(false);
        assert(!renderer.isTestToneEnabled());
        std::cout << "✓ Test tone control working" << std::endl;

        // Test 5: Master limiter
        std::cout << "Test 5: Master limiter" << std::endl;
        renderer.setMasterLimiterEnabled(true);
        assert(renderer.isMasterLimiterEnabled());
        renderer.setMasterLimiterCeiling(-3.0f);
        assert(renderer.getMasterLimiterCeiling() == -3.0f);
        std::cout << "✓ Master limiter working" << std::endl;

        // Test 6: PDC
        std::cout << "Test 6: PDC" << std::endl;
        renderer.setPDCEnabled(true);
        assert(renderer.isPDCEnabled());
        renderer.setPDCEnabled(false);
        assert(!renderer.isPDCEnabled());
        std::cout << "✓ PDC control working" << std::endl;

        // Test 7: Meter reset
        std::cout << "Test 7: Meter reset" << std::endl;
        renderer.resetPeakMeters();
        float initialLevel = renderer.getMasterLevel();
        float initialPeak = renderer.getMasterPeakLevel();
        assert(initialLevel == 0.0f);
        assert(initialPeak == 0.0f);
        std::cout << "✓ Meter reset working" << std::endl;

        // Test 8: Sample rate and buffer size
        std::cout << "Test 8: Sample rate and buffer size" << std::endl;
        renderer.setSampleRate(48000.0);
        assert(renderer.getSampleRate() == 48000.0);
        renderer.setBufferSize(1024);
        // Note: bufferSize is private, so we can't directly test it
        std::cout << "✓ Sample rate and buffer size working" << std::endl;

        // Test 9: Shutdown
        std::cout << "Test 9: Shutdown" << std::endl;
        renderer.shutdown();
        assert(!renderer.isInitialized());
        std::cout << "✓ Shutdown working" << std::endl;

        // Test 10: Reinitialization
        std::cout << "Test 10: Reinitialization" << std::endl;
        renderer.initialize(0, 2, 48000.0);  // No inputs, stereo output
        assert(renderer.isInitialized());
        std::cout << "✓ Reinitialization working" << std::endl;

        std::cout << "\n🎉 All AudioRenderer tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}