#include "../ai/AIEventBus.h"
#include "../ai/AIResponseCache.h"
#include "../ai/AIStatusManager.h"
#include "../ai/AudioFitnessEvaluator.h"
#include "../network/GrokUtils.h"
#include "../utils/StemSeparationJob.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>


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
      auto *op = mgr.getOperation(opId);
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
      
      // Configure evaluator to allow sine waves (Crest factor ~3dB)
      // Default requirement is 6dB which causes sine waves to be marked as dead
      ai::FitnessConfig config;
      config.minDynamicRangeDb = 2.0f;
      evaluator.setConfig(config);

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

      juce::WaitableEvent completionEvent;
      auto eventReceived = std::make_shared<std::atomic<bool>>(false);
      auto receivedPayload = std::make_shared<juce::String>();

      // Subscribe
      int subId = bus.subscribe(ai::AIEventType::SamplesFound, "TestSubscriber",
                                [eventReceived, receivedPayload, &completionEvent](const ai::AIEvent &e) {
                                  *eventReceived = true;
                                  *receivedPayload = e.payload.toString();
                                  completionEvent.signal();
                                });

      expect(subId > 0);
      expectEquals(bus.getSubscriberCount(ai::AIEventType::SamplesFound), 1);

      // Publish (async delivery)
      juce::var payload;
      payload = "test payload";
      bus.publish(ai::AIEventType::SamplesFound, "TestAgent", payload);

      // Wait for async callback with explicit timeout (500ms)
      bool signaled = completionEvent.wait(500);
      
      // Verify
      auto stats = bus.getStats();
      expect(stats.totalPublished >= 1);

      // Unsubscribe
      bus.unsubscribe(subId);
      expectEquals(bus.getSubscriberCount(ai::AIEventType::SamplesFound), 0);
      
      expect(signaled, "Async callback should have fired within timeout");
      expect(eventReceived->load());
      expectEquals(*receivedPayload, juce::String("test payload"));
    }

    beginTest("StemSeparationJob Lifecycle");
    {
      // Create a dummy audio file for testing
      juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
      juce::File testFile = tempDir.getChildFile("zenith_test_audio.wav");
      
      juce::AudioBuffer<float> buffer(2, 44100);
      buffer.clear();
      // Add some noise
      juce::Random rng;
      for (int ch = 0; ch < 2; ++ch)
          for (int i = 0; i < 44100; ++i)
              buffer.setSample(ch, i, rng.nextFloat() * 0.1f);

      juce::WavAudioFormat wavFormat;
      auto options = juce::AudioFormatWriterOptions()
                         .withSampleRate(44100.0)
                         .withNumChannels(2)
                         .withBitsPerSample(16);
      std::unique_ptr<juce::OutputStream> fileStream(new juce::FileOutputStream(testFile));
      std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
          fileStream, options));
      if (writer) {
          writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
          writer.reset();
      }

      juce::File outputDir = tempDir.getChildFile("zenith_test_stems");
      juce::WaitableEvent completionEvent;
      bool completed = false;
      utils::StemSeparationJob::StemFiles results;

      auto job = new utils::StemSeparationJob(testFile, outputDir, [&](const utils::StemSeparationJob::StemFiles& res) {
          results = res;
          completed = true;
          completionEvent.signal();
      });

      // Run job synchronously for test
      job->runJob();
      delete job;

      // Wait for async callback with explicit timeout (2 seconds)
      bool signaled = completionEvent.wait(2000);

      // Even if results.success is false (due to missing model), the job should have finished
      expect(signaled || completed, "Job callback should have fired within timeout");
      expect(completed);
      
      // Cleanup
      testFile.deleteFile();
      outputDir.deleteRecursively();
    }

  }
};

static AITests aiTests;

} // namespace tests
} // namespace zenith
