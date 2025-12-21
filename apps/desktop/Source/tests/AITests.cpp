#include "../ai/AIEventBus.h"
#include "../ai/AIResponseCache.h"
#include "../ai/AIStatusManager.h"
#include "../ai/AudioFitnessEvaluator.h"
#include "../network/GrokUtils.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>


namespace zenith {
namespace tests {

/**
 * @class AITests
 * @brief Tests for AI integration logic (parsing, etc.)
 */
class AITests : public juce::UnitTest {
public:
  AITests() : juce::UnitTest("AI Integration", "AI") {}

  void runTest() override {
    beginTest("Grok Malformed JSON Handling");
    {
      // 1. Clean JSON
      juce::String cleanJson = "{ \"cutoff\": 500.0 }";
      auto params = GrokUtils::parseJSONResponse(cleanJson);
      expect(params.isObject());
      expectEquals((double)params["cutoff"], 500.0);

      // 2. Markdown wrapped JSON
      juce::String markdownJson = "Here is the preset:\n```json\n{\n  "
                                  "\"cutoff\": 1000.0\n}\n```\nEnjoy!";
      params = GrokUtils::parseJSONResponse(markdownJson);
      expect(params.isObject());
      expectEquals((double)params["cutoff"], 1000.0);

      // 3. Generic code block
      juce::String genericCode = "```\n{ \"resonance\": 0.5 }\n```";
      params = GrokUtils::parseJSONResponse(genericCode);
      expect(params.isObject());
      expectEquals((double)params["resonance"], 0.5);

      // 4. Conversational garbage with JSON inside
      juce::String garbage = "Sure! I made a bass for you. { \"waveform\": "
                             "\"saw\" } is the patch.";
      params = GrokUtils::parseJSONResponse(garbage);
      expect(params.isObject());
      expectEquals(params["waveform"].toString(), juce::String("saw"));

      // 5. Invalid JSON
      juce::String invalid = "This is just text.";
      params = GrokUtils::parseJSONResponse(invalid);
      expect(!params.isObject());
    }

    beginTest("AIStatusManager Operations");
    {
      auto &mgr = ai::AIStatusManager::getInstance();

      // Begin operation
      auto opId = mgr.beginOperation("TestAgent", "Testing operation");
      expect(opId.isNotEmpty());
      expect(mgr.hasActiveOperations());
      expectEquals(mgr.getActiveOperationCount(), 1);

      // Update progress
      mgr.updateProgress(opId, 0.5f, "Halfway done");
      auto op = mgr.getOperation(opId);
      expect(op.has_value());
      expectWithinAbsoluteError(op->progress, 0.5f, 0.01f);

      // Complete operation
      mgr.completeOperation(opId, true, "Success!");
      expect(!mgr.hasActiveOperations());

      // Check stats
      auto stats = mgr.getStats();
      expect(stats.totalOperations >= 1);
      expect(stats.successfulOperations >= 1);
    }

    beginTest("AIResponseCache Put/Get");
    {
      auto &cache = ai::AIResponseCache::getInstance();
      cache.clear();

      // Generate hash
      juce::String hash = ai::AIResponseCache::generateHash("system", "prompt");
      expect(hash.length() == 64); // SHA256 hex = 64 chars

      // Put and get
      cache.put(hash, "cached response", 60); // 60 second TTL
      expect(cache.has(hash));

      auto result = cache.get(hash);
      expect(result.has_value());
      expectEquals(*result, juce::String("cached response"));

      // Cache stats
      auto stats = cache.getStats();
      expect(stats.hits >= 1);
      expect(stats.totalEntries >= 1);

      // Clear
      cache.clear();
      expect(!cache.has(hash));
    }

    beginTest("AudioFitnessEvaluator Scoring");
    {
      ai::AudioFitnessEvaluator evaluator;

      // Create a simple test buffer (sine wave)
      juce::AudioBuffer<float> buffer(2, 4410); // 100ms at 44100
      for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 4410; ++i) {
          float sample =
              std::sin(2.0f * 3.14159f * 440.0f * i / 44100.0f) * 0.5f;
          buffer.setSample(ch, i, sample);
        }
      }

      auto result = evaluator.evaluate(buffer, 44100.0);
      expect(!result.isDead);
      expect(result.totalScore > 0.0f);
      expect(result.totalScore <= 1.0f);

      // Test silent buffer = death
      juce::AudioBuffer<float> silentBuffer(2, 4410);
      silentBuffer.clear();
      auto silentResult = evaluator.evaluate(silentBuffer, 44100.0);
      expect(silentResult.isDead);

      // Test clipping buffer = death
      juce::AudioBuffer<float> clippingBuffer(2, 4410);
      for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 4410; ++i) {
          clippingBuffer.setSample(ch, i, 1.0f); // DC offset at max
        }
      }
      auto clipResult = evaluator.evaluate(clippingBuffer, 44100.0);
      expect(clipResult.isDead);
    }

    beginTest("AIEventBus Pub/Sub");
    {
      auto &bus = ai::AIEventBus::getInstance();
      bus.resetStats();

      bool eventReceived = false;
      juce::String receivedPayload;

      juce::WaitableEvent eventDone;

      // Subscribe
      int subId = bus.subscribe(ai::AIEventType::SamplesFound, "TestSubscriber",
                                [&](const ai::AIEvent &e) {
                                  eventReceived = true;
                                  receivedPayload = e.payload.toString();
                                  eventDone.signal();
                                });

      expect(subId > 0);
      expectEquals(bus.getSubscriberCount(ai::AIEventType::SamplesFound), 1);

      // Publish (sync for test)
      juce::var payload;
      payload = "test payload";
      bus.publish(ai::AIEventType::SamplesFound, "TestAgent", payload);

      // Wait for async delivery
      expect(eventDone.wait(2000)); // Wait up to 2 seconds

      // Verify
      auto stats = bus.getStats();
      expect(stats.totalPublished >= 1);

      // Unsubscribe
      bus.unsubscribe(subId);
      expectEquals(bus.getSubscriberCount(ai::AIEventType::SamplesFound), 0);
    }
  }
};

static AITests aiTests;

} // namespace tests
} // namespace zenith
