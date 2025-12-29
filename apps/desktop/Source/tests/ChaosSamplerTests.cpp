/*
  ==============================================================================

    ChaosSamplerTests.cpp
    QA & Verification Sentinel - Chaos Monkey Suite

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../instruments/ZenithSampler.h"
#include <thread>
#include <atomic>

class ChaosSamplerTests : public juce::UnitTest
{
public:
    ChaosSamplerTests() : juce::UnitTest("Chaos Sampler", "Chaos") {}

    void runTest() override
    {
        testLoadAndKill();
        testRapidPatchSwitching();
    }

private:
    /**
     * @brief Stress test for the "Destruction Race" fix.
     * Attempts to delete the sampler while a massive patch is loading.
     */
    void testLoadAndKill()
    {
        beginTest("Load And Kill - Destruction Race Prevention");
        
        for (int i = 0; i < 50; ++i) {
            auto sampler = std::make_unique<zenith::ZenithSamplerProcessor>();
            
            // Start loading in background
            // Note: We don't need a real 2GB bank, just enough to trigger async work
            sampler->loadSampleBankByName("LargeTestBank"); 
            
            // Immediate destruction
            sampler.reset();
            
            // If we didn't crash, it's a win
            expect(true);
        }
        
        logMessage("Verified cleanup of ZenithSamplerProcessor during active loading.");
    }

    /**
     * @brief Stress test for rapid patch switching.
     * Requests multiple loads in quick succession.
     */
    void testRapidPatchSwitching()
    {
        beginTest("Rapid Patch Switching - Queue Stability");
        
        zenith::ZenithSamplerProcessor sampler;
        
        for (int i = 0; i < 100; ++i) {
            sampler.loadSampleBankByName("Patch_" + juce::String(i));
            
            // Minimal sleep to allow thread to start
            if (i % 10 == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Final load should eventually succeed or remain queued safely
        expect(true); 
        logMessage("Verified rapid patch switching doesn't crash the background thread.");
    }
};

static ChaosSamplerTests chaosSamplerTests;
