#include <atomic>
#include <thread>
#include <vector>
#include <memory>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "Engine.h"
#include "TestUtils.h"
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
      zenith::MidiFifo fifo; 

      std::atomic<int> sentCount{0};
      std::thread writer([&]() {
        for (int i = 0; i < 10000; ++i) {
          auto msg = juce::MidiMessage::noteOn(1, 60, 1.0f);
          fifo.push(msg);
          sentCount++;
        }
      });

      std::atomic<int> receivedCount{0};
      std::atomic<bool> writerFinished{false};

      std::thread reader([&]() {
        while (true) {
          juce::MidiBuffer buffer;
          fifo.drainTo(buffer, 512);
          receivedCount += buffer.getNumEvents();

          if (writerFinished.load(std::memory_order_acquire) &&
              buffer.getNumEvents() == 0) {
            break;
          }
          std::this_thread::yield();
        }
      });

      writer.join();
      writerFinished = true;
      reader.join();

      expect(receivedCount <= 10000);
      expect(receivedCount > 0);
    }
  }
};

/**
 * @class MasterPluginRCUTest
 * @brief Stress tests for the Master Plugin RCU mechanism
 */
class MasterPluginRCUTest : public juce::UnitTest {
public:
  MasterPluginRCUTest()
      : juce::UnitTest("Master Plugin RCU Stress Test", "Concurrency") {}

  struct Snapshot {
    std::vector<void*> dummyPlugins;
  };

  void runTest() override {
    beginTest("Concurrent Snapshot Update and Access");

    std::atomic<Snapshot *> activeSnapshot{nullptr};
    std::vector<std::shared_ptr<Snapshot>> trash;
    std::shared_ptr<Snapshot> currentHolder;

    std::atomic<bool> shouldStop{false};
    std::atomic<int> updateCount{0};
    std::atomic<int> renderCount{0};

    // 1. "Message Thread" Hammer (updates)
    std::thread messageThread([&]() {
      while (!shouldStop.load()) {
        auto newSnapshot = std::make_shared<Snapshot>();
        for (int i = 0; i < 5; ++i) {
          newSnapshot->dummyPlugins.push_back(nullptr);
        }

        activeSnapshot.store(newSnapshot.get(), std::memory_order_release);

        trash.push_back(currentHolder);
        currentHolder = newSnapshot;

        if (trash.size() > 10) {
          trash.erase(trash.begin());
        }

        updateCount.fetch_add(1);
        std::this_thread::yield();
      }
    });

    // 2. "Audio Thread" Hammer (access)
    std::thread audioThread([&]() {
      while (!shouldStop.load()) {
        auto *snap = activeSnapshot.load(std::memory_order_acquire);
        if (snap) {
          for (auto* p : snap->dummyPlugins) {
            (void)p; 
          }
          renderCount.fetch_add(1);
        }
        std::this_thread::yield();
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    shouldStop.store(true);

    messageThread.join();
    audioThread.join();

    expect(updateCount.load() > 0, "No updates happened");
    expect(renderCount.load() > 0, "No renders happened");
  }
};

/**
 * @class EnginePluginManagementStressTest
 * @brief Real-world stress test using the actual Engine to add/remove plugins during processing
 */
class EnginePluginManagementStressTest : public juce::UnitTest {
public:
  EnginePluginManagementStressTest()
      : juce::UnitTest("Engine Plugin Management Stress Test", "Concurrency") {}

  void runTest() override {
    beginTest("Concurrent Plugin Add/Remove with Audio Callback");

    zenith::Engine engine;
    
    // Simulate audio thread with a simple loop
    std::atomic<bool> shouldStop{false};
    std::atomic<int> renderCycles{0};
    
    std::thread audioThread([&]() {
        const int numChannels = 2;
        const int numSamples = 512;
        float* inputs[numChannels];
        float* outputs[numChannels];
        juce::AudioBuffer<float> inBuffer(numChannels, numSamples);
        juce::AudioBuffer<float> outBuffer(numChannels, numSamples);
        
        for (int i = 0; i < numChannels; ++i) {
            inputs[i] = inBuffer.getWritePointer(i);
            outputs[i] = outBuffer.getWritePointer(i);
        }

        while (!shouldStop.load()) {
            engine.audioDeviceIOCallbackWithContext((const float**)inputs, numChannels,
                                        outputs, numChannels, numSamples, {});
            renderCycles.fetch_add(1);
            std::this_thread::yield();
        }
    });

    // Simulate message thread hammering plugin additions/removals
    std::atomic<int> updateCycles{0};
    std::thread messageThread([&]() {
        for (int i = 0; i < 100; ++i) {
            // Add a few plugins
            for (int p = 0; p < 3; ++p) {
                engine.addMasterPlugin(std::make_shared<StubAudioPlugin>());
            }
            
            // Wait slightly
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            
            // Remove them (indices will change as we remove, but removeMasterPlugin handles it)
            // Note: Engine::removeMasterPlugin(int index)
            // We'll just remove from index 0 a few times
            for (int p = 0; p < 3; ++p) {
                // We should check the number of plugins first to avoid out-of-bounds if necessary,
                // but the goal is to stress test. 
                // However, removeMasterPlugin usually has an internal bounds check or use snapshot size.
                engine.removeMasterPlugin(0);
            }
            
            updateCycles.fetch_add(1);
            std::this_thread::yield();
        }
    });

    messageThread.join();
    // Allow audio thread to run a bit longer to clean up any pending RCU trash if it were being processed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    shouldStop.store(true);
    audioThread.join();

    expect(updateCycles.load() > 0, "No updates happened");
    expect(renderCycles.load() > 0, "No audio callbacks happened");
    
    // Test implicitly passes if it doesn't crash
    logMessage("Stress test completed: " + juce::String(updateCycles.load()) + " update cycles, " + 
               juce::String(renderCycles.load()) + " audio cycles.");
  }
};

static LockFreeStressTest lockFreeStressTest;
static MasterPluginRCUTest masterPluginRCUTest;
static EnginePluginManagementStressTest enginePluginManagementStressTest;

} // namespace tests
} // namespace zenith
