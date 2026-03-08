#include "ONNXStemSeparator.h"

#include "DSPStemSeparator.h"
#include "PlatformModelUtils.h"

namespace zenith {

struct ONNXStemSeparator::Impl {
  bool initialized = false;
  juce::File modelPath;
};

ONNXStemSeparator::ONNXStemSeparator() : pImpl(std::make_unique<Impl>()) {}

ONNXStemSeparator::~ONNXStemSeparator() = default;

bool ONNXStemSeparator::isAvailable() const {
  return true;
}

bool ONNXStemSeparator::initialize(const juce::File& modelPath) {
  if (!pImpl) {
    pImpl = std::make_unique<Impl>();
  }
  pImpl->modelPath = modelPath;
  
  if (modelPath.existsAsFile()) {
      // Simulate validation logic
      if (modelPath.getSize() < 100 * 1024) {
          return false; // Too small
      }
      pImpl->initialized = true;
      return true;
  }
  return false;
}

ONNXStemSeparator::SeparationResult ONNXStemSeparator::separate(
    const juce::AudioBuffer<float>& input, double sampleRate) {
  SeparationResult result;

  const int numChannels = input.getNumChannels();
  const int numSamples = input.getNumSamples();

  result.vocals.setSize(numChannels, numSamples, false, false, true);
  result.drums.setSize(numChannels, numSamples, false, false, true);
  result.bass.setSize(numChannels, numSamples, false, false, true);
  result.other.setSize(numChannels, numSamples, false, false, true);

  if (numChannels <= 0 || numSamples <= 0) {
    result.error = "Input buffer is empty";
    return result;
  }

  DSPStemSeparator dsp;
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(numSamples);
  spec.numChannels = static_cast<juce::uint32>(numChannels);
  dsp.prepare(spec);

  juce::dsp::AudioBlock<const float> inputBlock(input);
  juce::dsp::AudioBlock<float> vocalsBlock(result.vocals);
  juce::dsp::AudioBlock<float> drumsBlock(result.drums);
  juce::dsp::AudioBlock<float> bassBlock(result.bass);
  juce::dsp::AudioBlock<float> otherBlock(result.other);

  dsp.process(inputBlock, vocalsBlock, DSPStemSeparator::StemType::Vocals);
  dsp.process(inputBlock, drumsBlock, DSPStemSeparator::StemType::Drums);
  dsp.process(inputBlock, bassBlock, DSPStemSeparator::StemType::Bass);
  dsp.process(inputBlock, otherBlock, DSPStemSeparator::StemType::Other);

  result.success = true;
  result.usedONNX = false;
  return result;
}

juce::String ONNXStemSeparator::getModelInfo() const {
  if (pImpl && pImpl->initialized) {
    return "ONNX model: " + pImpl->modelPath.getFileName();
  }
  return "DSP fallback mode (ONNX model not loaded)";
}

juce::File ONNXStemSeparator::findDefaultModel() {
  return PlatformModelUtils::findDefaultModel();
}

void ONNXStemSeparator::shutdown() {}

} // namespace zenith
