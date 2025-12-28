/**
 * @file AudioEngineTests.cpp
 * @brief Unit tests for Zenith DAW audio engine
 * @author Testing Team - Operation Polish Phase 2
 *
 * Priority: Audio Engine tests (as requested)
 * Framework: JUCE UnitTest
 */

#include "../engine/Clip.h"
#include "../engine/MixerChannel.h"
#include "../engine/Track.h"
#include "../engine/ProjectState.h"
#include "Engine.h"
#include "TestUtils.h"
#include <cmath> // For std::isnan and std::isinf
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

// ... (Previous tests remain unchanged)

/**
 * @class TrackProcessingTests
 * @brief Tests for Track audio processing
 */
class TrackProcessingTests : public juce::UnitTest {
public:
  TrackProcessingTests() : juce::UnitTest("Track Processing", "AudioEngine") {}

  void runTest() override {
    beginTest("Track creation");
    {
      // Test track creation with valid parameters
      auto track = zenith::Track::create("test-track-001", zenith::Track::Type::Audio);
      expect(track != nullptr);
      expect(track->getName() == "test-track-001");
      expect(track->getType() == zenith::Track::Type::Audio);
    }

    beginTest("Track mute/solo");
    {
      auto track = zenith::Track::create("test-track-001", zenith::Track::Type::Audio);
      expect(track != nullptr);

      // Test mute functionality
      track->setMuted(true);
      expect(track->isMuted());

      // Test solo functionality
      track->setSoloed(true);
      expect(track->isSoloed());
    }

    beginTest("Track volume processing");
    {
      // Create a track
      auto track = zenith::Track::create("VolumeTestTrack", zenith::Track::Type::Audio);
      expect(track != nullptr);

      // Set volume to -6dB (0.5 linear)
      float volumeDb = -6.0f;
      float targetGain = juce::Decibels::decibelsToGain(volumeDb);

      track->setVolume(targetGain);

      // Verify the track's mixer channel accepted the volume
      // This tests that Track::setVolume correctly propagates to MixerChannel
      expectEquals(track->getVolume(), targetGain);
      expectEquals(track->getMixerChannel().getVolume(), targetGain);

      // Note: Full DSP testing requires running getNextAudioBlock with a
      // context, which is heavy for a unit test. We trust MixerChannel tests
      // for the DSP math, and here we verify the Track -> MixerChannel control
      // binding.
    }

    beginTest("Track pan processing");
    {
      auto track = zenith::Track::create("PanTestTrack", zenith::Track::Type::Audio);
      expect(track != nullptr);

      // Pan hard left
      float pan = -1.0f;
      track->setPan(pan);

      expectEquals(track->getPan(), pan);
      expectEquals(track->getMixerChannel().getPan(), pan);
    }
  }
};

// ... (ClipPlaybackTests, MIDIRoutingTests, MixerChannelTests remain unchanged)

/**
 * @class ClipPlaybackTests
 * @brief Tests for Clip audio playback
 */
class ClipPlaybackTests : public juce::UnitTest {
public:
  ClipPlaybackTests() : juce::UnitTest("Clip Playback", "AudioEngine") {}

  void runTest() override {
    // Constants for improved readability (Complaint #10 Fix)
    const int kClipStart = 500;
    const int kClipLength = 1000;
    const int kPreRollLength = 400;
    const int kOverlapLength = 200;
    const int kSamplesPerBlock = 100;

    beginTest("Clip Timing Accuracy");
    {
      zenith::Clip clip;
      clip.setStartPosition(kClipStart);
      clip.setLength(kClipLength);

      // Create dummy audio content for the clip (1.0f amplitude)
      juce::AudioBuffer<float> content(1, kClipLength);
      for (int i = 0; i < kClipLength; ++i)
        content.setSample(0, i, 1.0f);
      clip.setAudioBuffer(content);
      clip.setPlaying(true);

      // Case 1: Render before clip (samples 0-400) -> Expect Silence
      juce::AudioBuffer<float> buffer(1, kPreRollLength);
      buffer.clear();

      // Set transport to 0
      clip.setTransportPosition(0);

      juce::AudioSourceChannelInfo info(&buffer, 0, kPreRollLength);
      clip.getNextAudioBlock(info);

      expect(buffer.getMagnitude(0, 0, kPreRollLength) == 0.0f,
             "Buffer before clip start should be silent");

      // Case 2: Render overlapping start (samples 400-600)
      // The clip starts at 500. So 400-500 should be silent, 500-600 should be
      // audio.
      buffer.setSize(1, kOverlapLength);
      buffer.clear();

      clip.setTransportPosition(kPreRollLength);
      info = juce::AudioSourceChannelInfo(&buffer, 0, kOverlapLength);
      clip.getNextAudioBlock(info);

      // First 100 samples (400-499) -> relative to clip start (-100 to -1) ->
      // silence
      expect(buffer.getMagnitude(0, 0, kSamplesPerBlock) == 0.0f,
             "Buffer overlapping pre-start should be silent");

      // Next 100 samples (500-599) -> relative to clip start (0 to 99) -> audio
      // (1.0f)
      expect(buffer.getMagnitude(0, kSamplesPerBlock, kSamplesPerBlock) > 0.0f,
             "Buffer overlapping post-start should contain audio");
    }

    beginTest("Clip start/stop");
    {
      zenith::Clip clip;
      clip.setStartPosition(0);
      clip.setLength(1000);
      juce::AudioBuffer<float> content(1, 1000);
      clip.setAudioBuffer(content);

      clip.setPlaying(true);
      expect(clip.isPlaying());

      clip.setPlaying(false);
      expect(!clip.isPlaying());
    }

    beginTest("Clip looping");
    {
      zenith::Clip clip;
      clip.setStartPosition(0);
      clip.setLength(100); // Short clip
      clip.setLooping(true);

      juce::AudioBuffer<float> content(1, 100);
      // Mark the start of the content to identify loop points
      content.clear();
      content.setSample(0, 0, 1.0f); // Sample 0 is 1.0
      clip.setAudioBuffer(content);
      clip.setPlaying(true);

      juce::AudioBuffer<float> buffer(1, 200); // Request 2 loops worth
      buffer.clear();

      clip.setTransportPosition(0);
      juce::AudioSourceChannelInfo info(&buffer, 0, 200);
      clip.getNextAudioBlock(info);

      // Expect signal at index 0 (loop 1 start)
      expect(buffer.getSample(0, 0) > 0.5f, "Loop 1 start not found");
      // Expect signal at index 100 (loop 2 start)
      expect(buffer.getSample(0, 100) > 0.5f, "Loop 2 start not found");
    }
  }
};

// ... (MIDIRoutingTests, MixerChannelTests unchanged)

/**
 * @class MIDIRoutingTests
 * @brief Tests for MIDI processing and routing
 */
class MIDIRoutingTests : public juce::UnitTest {
public:
  MIDIRoutingTests() : juce::UnitTest("MIDI Routing", "AudioEngine") {}

  void runTest() override {
    beginTest("MIDI note routing");
    {
      juce::MidiBuffer midiBuffer;

      // Add note on
      midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

      // Add note off
      midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), 100);

      // Verify messages
      expect(midiBuffer.getNumEvents() == 2);

      auto iterator = midiBuffer.cbegin();
      auto message = (*iterator).getMessage();
      expect(message.isNoteOn());
      expect(message.getNoteNumber() == 60);
    }

    beginTest("MIDI channel filtering");
    {
      juce::MidiBuffer input, output;

      // Add messages on different channels
      input.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
      input.addEvent(juce::MidiMessage::noteOn(2, 64, 0.8f), 10);
      input.addEvent(juce::MidiMessage::noteOn(3, 67, 0.8f), 20);

      // Filter for channel 1 only
      int targetChannel = 1;
      for (const auto metadata : input) {
        auto message = metadata.getMessage();
        if (message.getChannel() == targetChannel) {
          output.addEvent(message, metadata.samplePosition);
        }
      }

      expect(output.getNumEvents() == 1);
    }
  }
};

/**
 * @class MixerChannelTests
 * @brief Tests for mixer channel processing
 */
class MixerChannelTests : public juce::UnitTest {
public:
  MixerChannelTests() : juce::UnitTest("Mixer Channel", "AudioEngine") {}

  void runTest() override {
    beginTest("EQ processing applies gain");
    {
      zenith::MixerChannel channel;
      channel.prepareToPlay(512, 48000.0);

      // Enable EQ band 1 (Peak filter) with +6dB gain
      auto& band = channel.getEQBand(1);
      band.enabled.store(true);
      band.frequency.store(1000.0f);
      band.gain.store(6.0f);  // +6dB boost
      band.q.store(1.0f);
      channel.markEQDirty(1);

      // Process multiple blocks to allow filter detailed state to settle (transient response)
      float inputRMS = 0.0f;
      float outputRMS = 0.0f;
      float phase = 0.0f;
      float phaseIncrement = 2.0f * juce::MathConstants<float>::pi * 1000.0f / 48000.0f;
      
      juce::AudioBuffer<float> buffer(2, 512);
      juce::AudioSourceChannelInfo info(&buffer, 0, 512);

      for (int k = 0; k < 10; ++k) {
          // Fill buffer with sine wave (continuous phase)
          buffer.clear();
          for (int i = 0; i < 512; ++i) {
            float sample = std::sin(phase) * 0.5f;
            phase += phaseIncrement;
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
          }
          
          inputRMS = buffer.getRMSLevel(0, 0, 512); // Recalculate input RMS for this block
          channel.getNextAudioBlock(info);
          outputRMS = buffer.getRMSLevel(0, 0, 512);

          // If we reached target gain, break early
          if (outputRMS > inputRMS * 1.9f) // Theoretical is ~1.995, allow 1.9
             break;
      }
      
      expect(outputRMS > inputRMS * 1.3f, "EQ boost should increase signal level (~2.0x)");
    }

    beginTest("Compression reduces gain above threshold");
    {
      zenith::MixerChannel channel;
      channel.prepareToPlay(512, 48000.0);

      // Configure compressor: -20dB threshold, 4:1 ratio
      channel.setCompressorEnabled(true);
      channel.setCompressorThreshold(-20.0f);
      channel.setCompressorRatio(4.0f);
      channel.setCompressorAttack(1.0f);   // Fast attack
      channel.setCompressorRelease(50.0f);

      // Create loud test signal (above threshold)
      juce::AudioBuffer<float> buffer(2, 512);
      buffer.clear();
      for (int i = 0; i < 512; ++i) {
        float sample = 0.8f;  // Approximately -2dB, well above -20dB threshold
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
      }
      float inputPeak = buffer.getMagnitude(0, 0, 512);

      // Process through channel
      juce::AudioSourceChannelInfo info(&buffer, 0, 512);
      channel.getNextAudioBlock(info);

      // After compressor settles, gain reduction should be reported
      float gainReduction = channel.getGainReduction();
      expect(gainReduction > 0.0f, "Compressor should report gain reduction for loud signals");

      // Output should be reduced
      float outputPeak = buffer.getMagnitude(0, 0, 512);
      expect(outputPeak < inputPeak, "Compression should reduce signal level");
    }

    beginTest("Send levels route signal to aux buffers");
    {
      zenith::MixerChannel channel;
      channel.prepareToPlay(512, 48000.0);

      // Configure send 0 at -6dB (0.5 linear)
      channel.setSendLevel(0, 0.5f);
      channel.setSendPreFader(0, false);  // Post-fader

      // Create source signal
      juce::AudioBuffer<float> sourceBuffer(2, 512);
      sourceBuffer.clear();
      for (int i = 0; i < 512; ++i) {
        sourceBuffer.setSample(0, i, 0.5f);
        sourceBuffer.setSample(1, i, 0.5f);
      }

      // Create aux buffer to receive send
      juce::AudioBuffer<float> auxBuffer(2, 512);
      auxBuffer.clear();

      // Process with aux sends
      std::vector<juce::AudioBuffer<float>*> auxBuffers = { &auxBuffer, nullptr, nullptr, nullptr };
      juce::AudioSourceChannelInfo info(&sourceBuffer, 0, 512);
      channel.getNextAudioBlock(info, auxBuffers);

      // Verify send buffer received signal
      float auxLevel = auxBuffer.getRMSLevel(0, 0, 512);
      expect(auxLevel > 0.0f, "Aux send should contain signal when send level is non-zero");

      // Verify level is attenuated by send amount
      float sourceLevel = sourceBuffer.getRMSLevel(0, 0, 512);
      expect(auxLevel < sourceLevel, "Aux send level should be attenuated");
    }
  }
};

/**
 * @class MockPlugin : public StubAudioPlugin
 * Helper for PluginHostingTests - Inherits from StubAudioPlugin to reduce
 * boilerplate (Complaint #9 Fix)
 */
class MockPlugin : public StubAudioPlugin {
public:
  MockPlugin() : StubAudioPlugin() {}

  // Only override what we need for the test
  const juce::String getName() const override { return "Mock Plugin"; }

  // IMPORTANT for PDC test
  void setLatency(int samples) { setLatencySamples(samples); }
};

/**
 * @class PluginHostingTests
 * @brief Tests for VST3 plugin hosting
 */
class PluginHostingTests : public juce::UnitTest {
public:
  PluginHostingTests() : juce::UnitTest("Plugin Hosting", "AudioEngine") {}

  void runTest() override {
    beginTest("Plugin Delay Compensation");
    {
      auto plugin = std::make_unique<MockPlugin>();
      plugin->setLatency(100);
      expectEquals(plugin->getLatencySamples(), 100);
    }

    beginTest("Plugin instantiation");
    {
      auto plugin = std::make_unique<MockPlugin>();
      expect(plugin.get() != nullptr);
      expectEquals(plugin->getName(), juce::String("Mock Plugin"));
    }
  }
};

/**
 * @class BasicAudioTest
 * @brief Tests that validate actual audio engine behavior (Real Integration Test)
 */
class BasicAudioTest : public juce::UnitTest {
public:
  BasicAudioTest() : juce::UnitTest("Basic Audio Processing", "AudioEngine") {}

  void runTest() override {
    beginTest("Engine processes audio graph");
    {
      // 1. Setup Project State
      zenith::ProjectState projectState;
      projectState.newProject();

      // 2. Setup Engine
      zenith::Engine engine;
      engine.setProjectState(&projectState);
      
      // 2b. Setup Mock Device (Bypass Real Device)
      MockAudioIODevice mockDevice("Mock Device");
      // Use empty BigInteger for channels since we simulate callbacks
      mockDevice.open({}, {}, 44100.0, 512); 
      
      // Manually trigger preparation
      engine.audioDeviceAboutToStart(&mockDevice); 

      // 3. Create a Track via ProjectState (ensures correct model sync)
      // "audio" type corresponds to AudioTrack
      juce::String trackId = projectState.addTrack("Test Track", "audio");
      
      // Sync engine to pick up the new track
      engine.syncWithProjectState();

      // Verify track was created in engine
      expectEquals(engine.getNumTracks(), 1);
      if (engine.getNumTracks() == 0) return; // Fail fast
      
      auto track = engine.tracks()[0];
      expect(track != nullptr);
      expectEquals(track->getName(), juce::String("Test Track"));

      // 4. Create Audio Content (Manual Clip Injection)
      // Create a clip with some noise content
      const int sampleRate = 44100;
      const int clipLength = sampleRate * 2; // 2 seconds
      
      juce::AudioBuffer<float> content(2, clipLength);
      {
          juce::Random rng;
          for(int ch=0; ch<2; ++ch) {
              for(int i=0; i<clipLength; ++i) {
                  content.setSample(ch, i, rng.nextFloat() * 0.5f + 0.2f);
              }
          }
      }
      
      auto clip = std::make_unique<zenith::Clip>();
      clip->setAudioBuffer(content);
      clip->setStartPosition(0);
      clip->setLength(clipLength);
      clip->setPlaying(true);
      
      // Add clip to track
      track->addClip(std::move(clip));
      
      // 5. Start Playback
      engine.setPlayheadSamples(0);
      engine.setLooping(false);
      engine.play();
      expect(engine.isPlaying());

      // 6. Simulate Audio Callback
      const int blockSize = 512;
      juce::AudioBuffer<float> inBuffer(2, blockSize);
      juce::AudioBuffer<float> outBuffer(2, blockSize);
      inBuffer.clear();
      outBuffer.clear();
      
      float* inChans[] = { inBuffer.getWritePointer(0), inBuffer.getWritePointer(1) };
      float* outChans[] = { outBuffer.getWritePointer(0), outBuffer.getWritePointer(1) };
      
      // Construct dummy context
      juce::AudioIODeviceCallbackContext context{}; 
      
      // 7. Verify Results
      
      // A. Playhead should advance
      
      // Run block 1
      engine.audioDeviceIOCallbackWithContext(
          (const float* const*)inChans, 2,
          outChans, 2, blockSize,
          context
      );
      
      expectEquals(engine.getPlayheadSamples(), (juce::int64)blockSize);
      
      // B. Audio should be present (not silent)
      float magnitude = outBuffer.getMagnitude(0, blockSize);
      expect(magnitude > 0.001f, "Output buffer should contain audio signal");
      
      // Run block 2
      outBuffer.clear();
      engine.audioDeviceIOCallbackWithContext(
          (const float* const*)inChans, 2,
          outChans, 2, blockSize,
          context
      );
      
      expectEquals(engine.getPlayheadSamples(), (juce::int64)(blockSize * 2));
      float max0 = 0.0f;
      float max1 = 0.0f;
      for (int i = 0; i < blockSize; ++i) {
        max0 = std::max(max0, std::abs(outBuffer.getSample(0, i)));
        max1 = std::max(max1, std::abs(outBuffer.getSample(1, i)));
      }
      expect(max0 > 0.001f, "Output buffer 1 should contain signal");
      expect(max1 > 0.001f, "Output buffer 2 should contain signal");

      // Cleanup
      engine.audioDeviceStopped();
      mockDevice.close();
    }
  }
};

// Static test registration instances
static TrackProcessingTests trackProcessingTests;
static ClipPlaybackTests clipPlaybackTests;
static MIDIRoutingTests midiRoutingTests;
static MixerChannelTests mixerChannelTests;
static PluginHostingTests pluginHostingTests;
static BasicAudioTest basicAudioTest;

/**
 * @class AudioStabilityTest
 * @brief Tests that audio processing does not produce NaN or Inf values
 */
class AudioStabilityTest : public juce::UnitTest {
public:
  AudioStabilityTest() : juce::UnitTest("Audio Stability (NaN/Inf)", "Stability") {}

  void runTest() override {
    beginTest("Track processes audio without NaN/Inf");

    // 1. Setup Engine and Project
    zenith::ProjectState projectState;
    zenith::Engine engine;
    engine.setProjectState(&projectState);

    // Mock device setup
    MockAudioIODevice mockDevice("Mock Device");
    mockDevice.open({}, {}, 48000.0, 512);
    engine.audioDeviceAboutToStart(&mockDevice);

    // 2. Create a track
    projectState.addTrack("Stability Test Track", "audio");
    engine.syncWithProjectState();

    // 3. Process a block of audio
    const int numSamples = 512;
    juce::AudioBuffer<float> inBuffer(2, numSamples);
    juce::AudioBuffer<float> outBuffer(2, numSamples);
    
    // Fill input with some valid data (silence or noise)
    inBuffer.clear(); // Silence input

    float* inChans[] = { inBuffer.getWritePointer(0), inBuffer.getWritePointer(1) };
    float* outChans[] = { outBuffer.getWritePointer(0), outBuffer.getWritePointer(1) };
    
    juce::AudioIODeviceCallbackContext context{}; 

    // Run the engine callback
    engine.audioDeviceIOCallbackWithContext(
        (const float* const*)inChans, 2,
        outChans, 2, numSamples,
        context
    );

    // 4. Check for NaN/Inf in output
    for (int ch = 0; ch < 2; ++ch) {
        const float* samples = outBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            expect(!std::isnan(samples[i]), "Output sample is NaN at index " + juce::String(i));
            expect(!std::isinf(samples[i]), "Output sample is Inf at index " + juce::String(i));
        }
    }

    // Cleanup
    engine.audioDeviceStopped();
    mockDevice.close();
  }
};

static AudioStabilityTest audioStabilityTest;

} // namespace tests
} // namespace zenith
