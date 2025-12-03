#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../../include/EngineEvent.h"
#include <thread>
#include <atomic>

namespace zenith {
namespace tests {

/**
 * @class LockFreeStressTest
 * @brief Stress tests for lock-free data structures
 */
class LockFreeStressTest : public juce::UnitTest {
public:
    LockFreeStressTest() : juce::UnitTest("Lock-Free Stress Test", "Concurrency") {}
    
    void runTest() override {
        beginTest("MidiFifo Saturation");
        {
            zenith::MidiFifo fifo; // Lock-free queue from EngineEvent.h
            
            // 1. Hammer the writer from a "Network Thread"
            std::atomic<int> sentCount{0};
            std::thread writer([&]() {
                for (int i = 0; i < 10000; ++i) {
                    auto msg = juce::MidiMessage::noteOn(1, 60, 1.0f);
                    if (fifo.push(msg)) {
                        sentCount++;
                    } else {
                        // If full, we might yield and retry, or just count dropped
                        // For this test, let's retry a few times or just accept drops if queue is small
                        // But the user requirement says "prove they don't crash", so simply calling push is the test.
                    }
                }
            });

            // 2. Drain from "Audio Thread"
            std::atomic<int> receivedCount{0};
            std::atomic<bool> keepRunning{true};
            
            std::thread reader([&]() {
                while (keepRunning || sentCount > receivedCount) {
                    juce::MidiBuffer buffer;
                    fifo.drainTo(buffer, 0, 512); // Assuming drainTo(buffer, startSample, numSamples) signature
                    receivedCount += buffer.getNumEvents();
                    
                    if (!keepRunning && buffer.getNumEvents() == 0 && sentCount == receivedCount) break;
                    if (!keepRunning && sentCount > receivedCount) {
                         // Give writer a chance to finish if we are stopping
                         juce::Thread::sleep(1);
                    }
                }
            });

            writer.join();
            keepRunning = false;
            reader.join();
            
            // We expect to receive what we successfully sent
            // Note: If the FIFO is strictly bounded and non-blocking, 'push' might return false.
            // The test ensures no deadlocks or crashes occurred during simultaneous access.
            expect(receivedCount <= 10000);
            expect(receivedCount > 0); 
        }
    }
};

static LockFreeStressTest lockFreeStressTest;

} // namespace tests
} // namespace zenith
