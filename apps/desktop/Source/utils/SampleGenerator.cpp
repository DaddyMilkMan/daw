#include "SampleGenerator.h"
#include "../instruments/ContentPaths.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace zenith {

void SampleGenerator::generateMissingSamples() {
  auto contentRoot = ContentPaths::getInstance().getContentRoot();
  // Ensure the Samples directory exists
  // The path structure relies on Content/Examples/SampleMaps/Samples
  auto samplesDir = contentRoot.getChildFile("Examples")
                        .getChildFile("SampleMaps")
                        .getChildFile("Samples");

  if (!samplesDir.exists())
    samplesDir.createDirectory();

  // List of files derived from the known .zsamplemap.json files
  juce::StringArray requiredFiles = {
      // 808 Essentials
      "808-kick.wav", "808-snare.wav", "808-hihat-closed.wav",
      "808-hihat-open.wav", "808-clap.wav", "808-tom-low.wav",
      "808-tom-mid.wav", "808-tom-high.wav", "808-bass.wav",

      // LoFi Keys
      "lofi-piano-C3.wav",
      "lofi-piano-C4.wav", // Note: JSON in RegisterBuiltInInstruments has
                           // C3/C4/C5 without -soft suffix for first layer?
      // Checking RegisterBuiltInInstruments.cpp content again:
      // "lofi-piano-C3.wav", "lofi-piano-C4.wav", "lofi-piano-C5.wav" -> These
      // are different from what I saw in lofi-keys.zsamplemap.json in file
      // view!
      // In lofi-keys.zsamplemap.json Step 62: "lofi-piano-C3-soft.wav"
      // In RegisterBuiltInInstruments.cpp Step 104: "lofi-piano-C3.wav"
      // This is a discrepancy! I should generate ALL variants to cover both.

      "lofi-piano-C3-soft.wav", "lofi-piano-C4-soft.wav",
      "lofi-piano-C5-soft.wav", "lofi-piano-C3-hard.wav",
      "lofi-piano-C4-hard.wav",
      "lofi-piano-C5-hard.wav", // These are in both but sometimes named
                                // differently?
      // RegisterBuiltInInstruments has "lofi-piano-C3-hard.wav".
      // But it has "lofi-piano-C3.wav" for the "soft" layer.
      // While zsamplemap has "lofi-piano-C3-soft.wav".

      "lofi-piano-C3.wav", "lofi-piano-C4.wav", "lofi-piano-C5.wav",

      // Trap Pluck
      "trap-pluck-C2.wav", "trap-pluck-C3.wav", "trap-pluck-C4.wav",
      "trap-pluck-C5.wav", "trap-pluck-C6.wav",

      // Orchestral Strings (consistent)
      "strings-C2.wav", "strings-C2-loud.wav", "strings-E3.wav",
      "strings-E3-loud.wav", "strings-A4.wav", "strings-A4-loud.wav",
      "strings-C6.wav", "strings-C3.wav", "strings-C4.wav",
      "strings-C5.wav", // Added for RegisterBuiltInInstruments variance (it has
                        // C3, C4, C5)

      // FX & Impacts
      // RegisterBuiltInInstruments has: "fx-riser.wav", "fx-impact.wav",
      // "fx-reverse.wav", "fx-whoosh.wav"
      // zsamplemap has: "fx-riser-short.wav", "fx-impact-soft.wav" etc.
      "fx-riser-short.wav", "fx-riser-long.wav", "fx-impact-soft.wav",
      "fx-impact-hard.wav", "fx-reverse-cymbal.wav", "fx-whoosh-left.wav",
      "fx-whoosh-right.wav", "fx-sub-drop.wav",
      // Built-in variants
      "fx-riser.wav", "fx-impact.wav", "fx-reverse.wav", "fx-whoosh.wav"};

  juce::Array<juce::File> targetDirs;
  targetDirs.add(samplesDir); // Examples/SampleMaps/Samples

  // Add Instruments/ZenithSampler/Samples
  auto instrumentsDir = contentRoot.getChildFile("Instruments")
                            .getChildFile("ZenithSampler")
                            .getChildFile("Samples");
  if (!instrumentsDir.exists())
    instrumentsDir.createDirectory();
  targetDirs.add(instrumentsDir);

  for (const auto &dir : targetDirs) {
    for (const auto &filename : requiredFiles) {
      auto file = dir.getChildFile(filename);
      if (!file.existsAsFile()) {
        DBG("Generating missing sample: " << filename);

        // Simple heuristic to create distinct sounds
        float freq = 440.0f;
        float duration = 0.5f;
        bool isNoise = false;

        if (filename.contains("kick")) {
          freq = 60.0f;
          duration = 0.3f;
        } else if (filename.contains("snare")) {
          freq = 200.0f;
          isNoise = true;
          duration = 0.2f;
        } else if (filename.contains("hihat")) {
          freq = 8000.0f;
          isNoise = true;
          duration = 0.1f;
        } else if (filename.contains("bass")) {
          freq = 100.0f;
          duration = 1.0f;
        } else if (filename.contains("piano")) {
          freq = 261.63f; // C4
          if (filename.contains("C3"))
            freq = 130.81f;
          if (filename.contains("C5"))
            freq = 523.25f;
          duration = 1.0f;
        } else if (filename.contains("riser") || filename.contains("long")) {
          freq = 300.0f; // Sweep?
          duration = 2.0f;
        }

        createWavFile(file, freq, duration, isNoise);
      }
    }
  }
}

void SampleGenerator::createWavFile(const juce::File &file, float freq,
                                    float durationSecs, bool isNoise) {
  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::OutputStream> outStream(file.createOutputStream());
  if (!outStream)
    return;

  auto options = juce::AudioFormatWriterOptions()
                     .withSampleRate(44100.0)
                     .withNumChannels(1)
                     .withBitsPerSample(16);

  std::unique_ptr<juce::AudioFormatWriter> writer(
      wavFormat.createWriterFor(outStream, options));

  if (writer) {
    outStream.release(); // Writer takes ownership

    int numSamples = (int)(44100.0 * durationSecs);
    juce::AudioBuffer<float> buffer(1, numSamples);

    auto *ch = buffer.getWritePointer(0);
    double phase = 0.0;
    double phaseInc = freq * juce::MathConstants<double>::twoPi / 44100.0;

    for (int i = 0; i < numSamples; ++i) {
      // Simple decay
      float env = 1.0f - ((float)i / numSamples);
      env = env * env; // Exponential-ish

      float sample = 0.0f;
      if (isNoise || freq > 2000.0f) { // High freq noise-like or explicit noise
        sample =
            (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * env;
      } else {
        sample = (float)std::sin(phase) * env;
      }

      ch[i] = sample;
      phase += phaseInc;
    }

    writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
  }
}

} // namespace zenith
