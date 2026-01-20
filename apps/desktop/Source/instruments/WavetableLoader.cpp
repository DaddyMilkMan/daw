/*
  ==============================================================================

    WavetableLoader.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

    Implementation of wavetable file loading.

  ==============================================================================
*/

#include "WavetableLoader.h"
#include <cmath>
#include <vector>

namespace zenith {

WavetableLoader::WavetableLoader() { formatManager_.registerBasicFormats(); }

WavetableLoadResult WavetableLoader::loadFromFile(const juce::File &file) {
  if (!file.existsAsFile()) {
    return WavetableLoadResult::error("File not found: " +
                                      file.getFullPathName());
  }

  juce::String extension = file.getFileExtension().toLowerCase();

  if (extension == ".wav" || extension == ".aif" || extension == ".aiff") {
    return loadWavFile(file);
  } else if (extension == ".wt") {
    return loadWtFile(file);
  } else {
    return WavetableLoadResult::error("Unsupported file format: " + extension);
  }
}

WavetableLoadResult WavetableLoader::loadWavFile(const juce::File &file) {
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager_.createReaderFor(file));

  if (!reader) {
    return WavetableLoadResult::error("Failed to read audio file: " +
                                      file.getFileName());
  }

  // Read all samples
  int numSamples = static_cast<int>(reader->lengthInSamples);
  if (numSamples < 64) {
    return WavetableLoadResult::error("Audio file too short (min 64 samples)");
  }

  // Read to buffer (mono)
  juce::AudioBuffer<float> buffer(1, numSamples);
  reader->read(&buffer, 0, numSamples, 0, true, false);

  const float *data = buffer.getReadPointer(0);

  // Determine if this is a single-cycle or multi-frame wavetable
  if (numSamples <= WAVETABLE_FRAME_SIZE * 2) {
    // Single cycle - resample to frame size
    std::vector<float> resampled = resampleToFrameSize(data, numSamples);

    auto wavetable = std::make_unique<Wavetable>(1);
    wavetable->setFrameData(0, resampled.data());
    wavetable->setName(file.getFileNameWithoutExtension());

    return WavetableLoadResult::ok(std::move(wavetable));
  } else {
    // Multi-frame wavetable
    int numFrames = numSamples / WAVETABLE_FRAME_SIZE;
    numFrames = juce::jlimit(1, MAX_WAVETABLE_FRAMES, numFrames);

    auto wavetable = std::make_unique<Wavetable>(numFrames);
    wavetable->setName(file.getFileNameWithoutExtension());

    for (int i = 0; i < numFrames; ++i) {
      wavetable->setFrameData(i, data + (i * WAVETABLE_FRAME_SIZE));
    }

    return WavetableLoadResult::ok(std::move(wavetable));
  }
}

WavetableLoadResult WavetableLoader::loadWtFile(const juce::File &file) {
  // Serum .wt format structure:
  // Header (varies by version):
  //   - "WT01" or similar magic number
  //   - Frame size (typically 2048)
  //   - Number of frames
  // Data:
  //   - Raw float32 samples, frame by frame

  juce::FileInputStream stream(file);
  if (!stream.openedOk()) {
    return WavetableLoadResult::error("Failed to open WT file: " +
                                      file.getFileName());
  }

  // Read header (simplified - real Serum format has more fields)
  char magic[4];
  stream.read(magic, 4);

  if (std::strncmp(magic, "wt", 2) != 0 && std::strncmp(magic, "WT", 2) != 0) {
    // Fallback: treat as raw float data
    stream.setPosition(0);
    int64_t fileSize = stream.getTotalLength();
    int numSamples = static_cast<int>(fileSize / sizeof(float));

    if (numSamples < WAVETABLE_FRAME_SIZE) {
      return WavetableLoadResult::error("WT file too small");
    }

    int numFrames = numSamples / WAVETABLE_FRAME_SIZE;
    numFrames = juce::jlimit(1, MAX_WAVETABLE_FRAMES, numFrames);

    auto wavetable = std::make_unique<Wavetable>(numFrames);
    wavetable->setName(file.getFileNameWithoutExtension());

    std::vector<float> frameBuffer(WAVETABLE_FRAME_SIZE);
    for (int i = 0; i < numFrames; ++i) {
      stream.read(frameBuffer.data(), WAVETABLE_FRAME_SIZE * sizeof(float));
      wavetable->setFrameData(i, frameBuffer.data());
    }

    return WavetableLoadResult::ok(std::move(wavetable));
  }

  // Parse header
  int frameSize = stream.readInt();
  int numFrames = stream.readInt();

  if (frameSize != WAVETABLE_FRAME_SIZE) {
    // Need to resample - for now, just warn
    DBG("Warning: WT frame size is " << frameSize << ", expected "
                                     << WAVETABLE_FRAME_SIZE);
  }

  numFrames = juce::jlimit(1, MAX_WAVETABLE_FRAMES, numFrames);

  auto wavetable = std::make_unique<Wavetable>(numFrames);
  wavetable->setName(file.getFileNameWithoutExtension());

  std::vector<float> frameBuffer(frameSize);
  std::vector<float> resampledBuffer(WAVETABLE_FRAME_SIZE);

  for (int i = 0; i < numFrames; ++i) {
    stream.read(frameBuffer.data(), frameSize * sizeof(float));

    if (frameSize == WAVETABLE_FRAME_SIZE) {
      wavetable->setFrameData(i, frameBuffer.data());
    } else {
      // Resample to WAVETABLE_FRAME_SIZE
      resampleToFrameSize(frameBuffer.data(), resampledBuffer.data(), frameSize);
      wavetable->setFrameData(i, resampledBuffer.data());
    }
  }

  return WavetableLoadResult::ok(std::move(wavetable));
}

WavetableLoadResult WavetableLoader::loadFromBuffer(const float *data,
                                                    int numSamples,
                                                    int samplesPerFrame) {
  if (!data || numSamples < samplesPerFrame) {
    return WavetableLoadResult::error("Invalid buffer data");
  }

  int numFrames = numSamples / samplesPerFrame;
  numFrames = juce::jlimit(1, MAX_WAVETABLE_FRAMES, numFrames);

  auto wavetable = std::make_unique<Wavetable>(numFrames);

  if (samplesPerFrame == WAVETABLE_FRAME_SIZE) {
    for (int i = 0; i < numFrames; ++i) {
      wavetable->setFrameData(i, data + (i * WAVETABLE_FRAME_SIZE));
    }
  } else {
    // Resample each frame
    std::vector<float> resampledBuffer(WAVETABLE_FRAME_SIZE);
    for (int i = 0; i < numFrames; ++i) {
      const float *frameData = data + (i * samplesPerFrame);
      resampleToFrameSize(frameData, resampledBuffer.data(), samplesPerFrame);
      wavetable->setFrameData(i, resampledBuffer.data());
    }
  }

  return WavetableLoadResult::ok(std::move(wavetable));
}

std::unique_ptr<Wavetable>
WavetableLoader::generateBasicWavetable(int type, int numFrames) {
  auto wavetable = std::make_unique<Wavetable>(numFrames);
  std::vector<float> frame(WAVETABLE_FRAME_SIZE);

  // Hoist PWM vectors to avoid re-allocation in loop
  std::vector<float> pwmX, pwmY;

  for (int f = 0; f < numFrames; ++f) {
    float morphAmount =
        (numFrames > 1) ? static_cast<float>(f) / (numFrames - 1) : 0.0f;

    // Precompute coefficients for PWM to hoist math out of the sample loop
    if (type == 4) {
      int maxHarmonic = 32;
      if (pwmX.size() != static_cast<size_t>(maxHarmonic + 1)) {
        pwmX.resize(static_cast<size_t>(maxHarmonic + 1));
        pwmY.resize(static_cast<size_t>(maxHarmonic + 1));
      }
      float pw = 0.1f + morphAmount * 0.8f;

      for (int h = 1; h <= maxHarmonic; ++h) {
        float b = h * pw * juce::MathConstants<float>::pi;
        float term = 2.0f / (h * juce::MathConstants<float>::pi) * std::sin(b);
        pwmX[static_cast<size_t>(h)] = term * std::cos(b);
        pwmY[static_cast<size_t>(h)] = term * std::sin(b);
      }
    }

    for (int i = 0; i < WAVETABLE_FRAME_SIZE; ++i) {
      float phase = static_cast<float>(i) / WAVETABLE_FRAME_SIZE;
      float sample = 0.0f;

      // Common recurrence setup for additive types
      if (type >= 1 && type <= 4) {
        float angle = phase * juce::MathConstants<float>::twoPi;
        float s1 = std::sin(angle);
        float c1 = std::cos(angle);
        float currentSin = s1;
        float currentCos = c1;

        if (type == 1) { // Saw
          int maxHarmonic = 64;
          sample += currentSin;
          for (int h = 2; h <= maxHarmonic; ++h) {
            float nextSin = currentSin * c1 + currentCos * s1;
            float nextCos = currentCos * c1 - currentSin * s1;
            currentSin = nextSin;
            currentCos = nextCos;
            sample += currentSin / static_cast<float>(h);
          }
          sample *= 0.5f;
        } else if (type == 2) { // Square
          int maxHarmonic = 32;
          sample += currentSin;

          // Step 2 recurrence
          float angle2 = 2.0f * angle;
          float s2 = std::sin(angle2);
          float c2 = std::cos(angle2);

          for (int h = 3; h <= maxHarmonic; h += 2) {
            float nextSin = currentSin * c2 + currentCos * s2;
            float nextCos = currentCos * c2 - currentSin * s2;
            currentSin = nextSin;
            currentCos = nextCos;
            sample += currentSin / static_cast<float>(h);
          }
          sample *= 0.6f;
        } else if (type == 3) { // Triangle
          int maxHarmonic = 32;
          sample += currentSin;
          int sign = -1;

          // Step 2 recurrence
          float angle2 = 2.0f * angle;
          float s2 = std::sin(angle2);
          float c2 = std::cos(angle2);

          for (int h = 3; h <= maxHarmonic; h += 2) {
            float nextSin = currentSin * c2 + currentCos * s2;
            float nextCos = currentCos * c2 - currentSin * s2;
            currentSin = nextSin;
            currentCos = nextCos;
            sample += sign * currentSin / static_cast<float>(h * h);
            sign = -sign;
          }
          sample *= 0.8f;
        } else if (type == 4) { // PWM
          int maxHarmonic = 32;
          // h=1
          sample += pwmX[1] * currentCos + pwmY[1] * currentSin;

          for (int h = 2; h <= maxHarmonic; ++h) {
            float nextSin = currentSin * c1 + currentCos * s1;
            float nextCos = currentCos * c1 - currentSin * s1;
            currentSin = nextSin;
            currentCos = nextCos;
            sample += pwmX[static_cast<size_t>(h)] * currentCos +
                      pwmY[static_cast<size_t>(h)] * currentSin;
          }
          sample *= 0.6f;
        }
      } else if (type == 0) { // Sine
        sample = std::sin(phase * juce::MathConstants<float>::twoPi);
      } else if (type == 5) { // Formant morph (basic vowel-ish)
        // Mix of harmonics emphasizing different formants
        float f1 = 3.0f + morphAmount * 5.0f;  // Formant 1: 3-8
        float f2 = 8.0f + morphAmount * 12.0f; // Formant 2: 8-20
        sample = std::sin(phase * juce::MathConstants<float>::twoPi);
        sample +=
            0.5f * std::sin(f1 * phase * juce::MathConstants<float>::twoPi);
        sample +=
            0.3f * std::sin(f2 * phase * juce::MathConstants<float>::twoPi);
        sample *= 0.5f;
      } else {
        sample = std::sin(phase * juce::MathConstants<float>::twoPi);
      }

      frame[i] = sample;
    }

    wavetable->setFrameData(f, frame.data());
  }

  // Name based on type
  const char *names[] = {"Sine", "Saw", "Square", "Triangle", "PWM", "Formant"};
  if (type >= 0 && type < 6) {
    wavetable->setName(names[type]);
  }

  return wavetable;
}

juce::StringArray WavetableLoader::getBundledWavetableNames() const {
  juce::StringArray names;

  if (contentDirectory_.isDirectory()) {
    juce::Array<juce::File> files;
    contentDirectory_.findChildFiles(files, juce::File::findFiles, false,
                                     "*.wav;*.wt");
    for (const auto &file : files) {
      names.add(file.getFileNameWithoutExtension());
    }
  }

  // Always include basic procedural wavetables
  names.addIfNotAlreadyThere("Basic Sine");
  names.addIfNotAlreadyThere("Basic Saw");
  names.addIfNotAlreadyThere("Basic Square");
  names.addIfNotAlreadyThere("Basic Triangle");
  names.addIfNotAlreadyThere("Basic PWM");
  names.addIfNotAlreadyThere("Basic Formant");

  return names;
}

WavetableLoadResult
WavetableLoader::loadBundledWavetable(const juce::String &name) {
  // Check for procedural wavetables first
  if (name.startsWith("Basic ")) {
    juce::String typeName = name.substring(6);
    int type = 0;
    if (typeName == "Sine")
      type = 0;
    else if (typeName == "Saw")
      type = 1;
    else if (typeName == "Square")
      type = 2;
    else if (typeName == "Triangle")
      type = 3;
    else if (typeName == "PWM")
      type = 4;
    else if (typeName == "Formant")
      type = 5;

    auto wt = generateBasicWavetable(type, (type == 4 || type == 5) ? 64 : 1);
    return WavetableLoadResult::ok(std::move(wt));
  }

  // Look for file
  if (contentDirectory_.isDirectory()) {
    juce::File wavFile = contentDirectory_.getChildFile(name + ".wav");
    if (wavFile.existsAsFile()) {
      return loadFromFile(wavFile);
    }

    juce::File wtFile = contentDirectory_.getChildFile(name + ".wt");
    if (wtFile.existsAsFile()) {
      return loadFromFile(wtFile);
    }
  }

  return WavetableLoadResult::error("Wavetable not found: " + name);
}

void WavetableLoader::setContentDirectory(const juce::File &path) {
  contentDirectory_ = path;
}

std::vector<float> WavetableLoader::resampleToFrameSize(const float *data,
                                                        int numSamples) {
  std::vector<float> result(WAVETABLE_FRAME_SIZE);
  resampleToFrameSize(data, result.data(), numSamples);
  return result;
}

void WavetableLoader::resampleToFrameSize(const float *data, float *dst,
                                          int numSamples) {
  for (int i = 0; i < WAVETABLE_FRAME_SIZE; ++i) {
    float srcPos = static_cast<float>(i) / WAVETABLE_FRAME_SIZE * numSamples;
    int idx0 = static_cast<int>(srcPos) % numSamples;
    int idx1 = (idx0 + 1) % numSamples;
    float frac = srcPos - std::floor(srcPos);
    dst[i] = data[idx0] * (1.0f - frac) + data[idx1] * frac;
  }
}

} // namespace zenith
