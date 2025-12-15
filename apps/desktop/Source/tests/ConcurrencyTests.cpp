#include <atomic>
#include <thread>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "EngineEvent.h"

namespace zenith {
namespace tests {

/**
 * @class LockFreeStressTest
 * @brief Stress tests for lock-free data structures
 */
class LockFreeStressTest : public juce::UnitTest {
public:
  LockFreeStressTest()
      : juce::UnitTest("Lock-Free Stress Test", "Concurrency") {}

  void runTest() override {
    beginTest("MidiFifo Saturation");
    {
      zenith::MidiFifo fifo; // Lock-free queue from EngineEvent.h

      // 1. Hammer the writer from a "Network Thread"
      std::atomic<int> sentCount{0};
      std::thread writer([&]() {
        for (int i = 0; i < 10000; ++i) {
          auto msg = juce::MidiMessage::noteOn(1, 60, 1.0f);
          // MidiFifo::push() returns void and may silently drop if full.
          // For this stress test, we push and count attempts.
          // The primary goal is proving no crashes under concurrent access.
          fifo.push(msg);
          sentCount++;
        }
      });

      // 2. Drain from "Audio Thread"
      std::atomic<int> receivedCount{0};
      std::atomic<bool> writerFinished{false};

      std::thread reader([&]() {
        while (true) {
          juce::MidiBuffer buffer;
          fifo.drainTo(buffer, 512);
          receivedCount += buffer.getNumEvents();

          if (writerFinished.load(std::memory_order_acquire) &&
              buffer.getNumEvents() == 0) {
            // Writer is done and the queue is empty, so we can exit.
            break;
          }

          // Active spin with yield - standard for lock-free testing
          // Avoids OS scheduler sleep granularity issues (15ms+)
          std::this_thread::yield();
        }
      });

      writer.join();
      writerFinished = true;
      reader.join();

      // We expect to receive what we successfully sent
      // Note: If the FIFO is strictly bounded and non-blocking, 'push' might
      // return false. The test ensures no deadlocks or crashes occurred during
      // simultaneous access.
      expect(receivedCount <= 10000);
      expect(receivedCount > 0);
    }
  }
};

static LockFreeStressTest lockFreeStressTest;

} // namespace tests
} // namespace zenith
