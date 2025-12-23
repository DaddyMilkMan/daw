#include "ONNXStemSeparator.h"
#include "PlatformModelUtils.h"
#include "DSPStemSeparator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>


// ONNX Runtime headers (conditional compilation)
// When ONNX Runtime is linked, define ZENITH_USE_ONNX_RUNTIME in CMake
#ifdef ZENITH_USE_ONNX_RUNTIME
#include <cpu_provider_factory.h>
#include <onnxruntime_cxx_api.h>

#endif

namespace zenith {

struct ONNXStemSeparator::Impl {
  bool isLoaded = false;
  juce::File modelPath;

#ifdef ZENITH_USE_ONNX_RUNTIME
  // ONNX Runtime session and environment
  std::unique_ptr<Ort::Env> env;
  std::unique_ptr<Ort::Session> session;
  std::unique_ptr<Ort::SessionOptions> sessionOptions;

  // Model metadata
  std::vector<const char *> inputNames;
  std::vector<const char *> outputNames;
  std::vector<int64_t> inputShape;
  std::vector<int64_t> outputShape;

  // Memory info for tensor allocation
  std::unique_ptr<Ort::MemoryInfo> memoryInfo;

  // Reuse buffer for input tensor
  std::vector<float> inputTensorValues;
#endif
};

ONNXStemSeparator::ONNXStemSeparator() : pImpl(std::make_unique<Impl>()) {
#ifdef ZENITH_USE_ONNX_RUNTIME
  try {
    // Initialize ONNX Runtime environment
    pImpl->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING,
                                            "ZenithStemSeparator");
    pImpl->memoryInfo = std::make_unique<Ort::MemoryInfo>(
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
    DBG("ONNXStemSeparator: ONNX Runtime environment initialized");
  } catch (const Ort::Exception &e) {
    DBG("ONNXStemSeparator: Failed to initialize ONNX Runtime - " +
        juce::String(e.what()));
  }
#else
  DBG("ONNXStemSeparator: ONNX Runtime not linked - DSP fallback only");
#endif
}

ONNXStemSeparator::~ONNXStemSeparator() = default;

bool ONNXStemSeparator::isAvailable() const {
#ifdef ZENITH_USE_ONNX_RUNTIME
  // Check if runtime is properly initialized
  return pImpl->env != nullptr && pImpl->memoryInfo != nullptr;
#else
  // ONNX Runtime not compiled in
  return false;
#endif
}

bool ONNXStemSeparator::initialize(const juce::File &modelPath) {
  juce::File fileToLoad = modelPath;

  // If provided path is invalid or empty, try to find default
  if (!fileToLoad.existsAsFile()) {
    DBG("ONNXStemSeparator: Provided path not found or empty, searching for default...");
    fileToLoad = findDefaultModel();
  }

  if (!fileToLoad.existsAsFile()) {
    DBG("ONNXStemSeparator: Model file not found - " +
        modelPath.getFullPathName());
    return false;
  }

  pImpl->modelPath = fileToLoad;

#ifdef ZENITH_USE_ONNX_RUNTIME
  try {
    // Create session options
    pImpl->sessionOptions = std::make_unique<Ort::SessionOptions>();

    // Set optimization level
    pImpl->sessionOptions->SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);

    // Set thread count for inference (audio processing typically uses 2-4
    // threads)
    pImpl->sessionOptions->SetIntraOpNumThreads(2);
    pImpl->sessionOptions->SetInterOpNumThreads(2);

    // Enable CPU memory arena for better performance
    pImpl->sessionOptions->EnableCpuMemArena();

    // Load model
#ifdef _WIN32
    // Windows uses wide strings for file paths
    std::wstring wModelPath = modelPath.getFullPathName().toWideCharPointer();
    pImpl->session = std::make_unique<Ort::Session>(
        *pImpl->env, wModelPath.c_str(), *pImpl->sessionOptions);
#else
    // Unix systems use regular strings
    std::string sModelPath = modelPath.getFullPathName().toStdString();
    pImpl->session = std::make_unique<Ort::Session>(
        *pImpl->env, sModelPath.c_str(), *pImpl->sessionOptions);
#endif

    // Get input/output metadata
    Ort::AllocatorWithDefaultOptions allocator;

    // Input metadata (typically [batch, channels, samples] for audio models)
    size_t numInputs = pImpl->session->GetInputCount();
    if (numInputs > 0) {
      Ort::AllocatedStringPtr inputNameAllocated =
          pImpl->session->GetInputNameAllocated(0, allocator);
      pImpl->inputNames.push_back(inputNameAllocated.get());

      Ort::TypeInfo inputTypeInfo = pImpl->session->GetInputTypeInfo(0);
      auto tensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
      pImpl->inputShape = tensorInfo.GetShape();

      DBG("ONNXStemSeparator: Input shape - " +
          juce::String(pImpl->inputShape[0]) + "x" +
          juce::String(pImpl->inputShape[1]) + "x" +
          juce::String(pImpl->inputShape[2]));
    }

    // Output metadata (typically 4 outputs for vocals, drums, bass, other)
    size_t numOutputs = pImpl->session->GetOutputCount();
    for (size_t i = 0; i < numOutputs; ++i) {
      Ort::AllocatedStringPtr outputNameAllocated =
          pImpl->session->GetOutputNameAllocated(i, allocator);
      pImpl->outputNames.push_back(outputNameAllocated.get());
    }

    pImpl->isLoaded = true;
    DBG("ONNXStemSeparator: Model loaded successfully - " +
        modelPath.getFileName());
    DBG("ONNXStemSeparator: Inputs: " + juce::String((int)numInputs) +
        ", Outputs: " + juce::String((int)numOutputs));

    return true;
  } catch (const Ort::Exception &e) {
    DBG("ONNXStemSeparator: Failed to load model - " + juce::String(e.what()));
    pImpl->isLoaded = false;
    return false;
  }
#else
  // Without ONNX Runtime, we can't load the model but we still track the path
  // for potential future use or informational purposes
  pImpl->isLoaded = false;
  DBG("ONNXStemSeparator: Model path stored, but ONNX Runtime not available - "
      "using DSP fallback");
  return false; // Return false because actual ONNX inference won't work
#endif
}

ONNXStemSeparator::SeparationResult
ONNXStemSeparator::separate(const juce::AudioBuffer<float> &input,
                            double sampleRate) {
  SeparationResult result;

  // Initialize result buffers
  int numSamples = input.getNumSamples();
  int numChannels =
      juce::jmin(2, input.getNumChannels()); // Force stereo for stem separation

  result.vocals.setSize(numChannels, numSamples);
  result.drums.setSize(numChannels, numSamples);
  result.bass.setSize(numChannels, numSamples);
  result.other.setSize(numChannels, numSamples);

  // Clear buffers
  result.vocals.clear();
  result.drums.clear();
  result.bass.clear();
  result.other.clear();

#ifdef ZENITH_USE_ONNX_RUNTIME
  // Try ONNX inference if available and model is loaded
  if (isAvailable() && pImpl->isLoaded && pImpl->session) {
    try {
      // Prepare input tensor
      // Most stem separation models expect [batch=1, channels=2, samples=N]
      std::vector<int64_t> inputShape = {1, static_cast<int64_t>(numChannels),
                                         static_cast<int64_t>(numSamples)};
      // Copy audio data to contiguous buffer (interleaved -> planar if needed)
      // Reuse buffer to avoid allocation
      size_t inputTensorSize = numChannels * numSamples;
      pImpl->inputTensorValues.resize(inputTensorSize);

      for (int ch = 0; ch < numChannels; ++ch) {
        const float *channelData = input.getReadPointer(ch);
        float *destData = pImpl->inputTensorValues.data() + (ch * numSamples);
        juce::FloatVectorOperations::copy(destData, channelData, numSamples);
      }

      // Create input tensor
      Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
          *pImpl->memoryInfo, pImpl->inputTensorValues.data(), inputTensorSize,
          inputShape.data(), inputShape.size());
      // Run inference
      auto outputTensors = pImpl->session->Run(
          Ort::RunOptions{nullptr}, pImpl->inputNames.data(), &inputTensor, 1,
          pImpl->outputNames.data(), pImpl->outputNames.size());

      // Process outputs
      // Assuming model outputs 4 tensors: vocals, drums, bass, other
      if (outputTensors.size() >= 4) {
        // Extract vocals
        float *vocalsData = outputTensors[0].GetTensorMutableData<float>();
        copyTensorToBuffer(vocalsData, result.vocals, numChannels, numSamples);

        // Extract drums
        float *drumsData = outputTensors[1].GetTensorMutableData<float>();
        copyTensorToBuffer(drumsData, result.drums, numChannels, numSamples);

        // Extract bass
        float *bassData = outputTensors[2].GetTensorMutableData<float>();
        copyTensorToBuffer(bassData, result.bass, numChannels, numSamples);

        // Extract other
        float *otherData = outputTensors[3].GetTensorMutableData<float>();
        copyTensorToBuffer(otherData, result.other, numChannels, numSamples);

        result.success = true;
        result.usedONNX = true;

        DBG("ONNXStemSeparator: Inference completed successfully");
        return result;
      } else {
        DBG("ONNXStemSeparator: Unexpected number of outputs - " +
            juce::String((int)outputTensors.size()));
        // Fall through to DSP fallback
      }
    } catch (const Ort::Exception &e) {
      DBG("ONNXStemSeparator: Inference failed - " + juce::String(e.what()) +
          " - falling back to DSP");
      // Fall through to DSP fallback
    }
  }
#endif

  // Fallback to DSP-based stem separation
  DBG("ONNXStemSeparator: Using DSP fallback");
  DSPStemSeparator dsp;
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(numSamples);
  spec.numChannels = static_cast<juce::uint32>(numChannels);

  dsp.prepare(spec);

  // Create AudioBlocks
  juce::dsp::AudioBlock<const float> inputBlock(input);

  juce::dsp::AudioBlock<float> vocalsBlock(result.vocals);
  juce::dsp::AudioBlock<float> drumsBlock(result.drums);
  juce::dsp::AudioBlock<float> bassBlock(result.bass);
  juce::dsp::AudioBlock<float> otherBlock(result.other);

  // Process stems
  dsp.process(inputBlock, vocalsBlock, DSPStemSeparator::StemType::Vocals);
  dsp.process(inputBlock, drumsBlock, DSPStemSeparator::StemType::Drums);
  dsp.process(inputBlock, bassBlock, DSPStemSeparator::StemType::Bass);
  dsp.process(inputBlock, otherBlock, DSPStemSeparator::StemType::Other);

  result.success = true;
  result.usedONNX = false;
  return result;
}

#ifdef ZENITH_USE_ONNX_RUNTIME
void ONNXStemSeparator::copyTensorToBuffer(const float *tensorData,
                                           juce::AudioBuffer<float> &buffer,
                                           int numChannels, int numSamples) {
  // ONNX tensor is typically planar: [channel0_samples...][channel1_samples...]
  // JUCE buffer is also planar, so direct copy per channel
  for (int ch = 0; ch < numChannels; ++ch) {
    float *channelData = buffer.getWritePointer(ch);
    const float *sourceData = tensorData + (ch * numSamples);
    juce::FloatVectorOperations::copy(channelData, sourceData, numSamples);
  }
}
#endif

juce::String ONNXStemSeparator::getModelInfo() const {
  if (!pImpl->isLoaded) {
    return "No model loaded";
  }

  juce::String info;
  info << "Model: " << pImpl->modelPath.getFileName() << "\n";
  info << "Path: " << pImpl->modelPath.getFullPathName() << "\n";

#ifdef ZENITH_USE_ONNX_RUNTIME
  if (pImpl->session) {
    info << "ONNX Runtime: Available\n";
    info << "Inputs: " << (int)pImpl->inputNames.size() << "\n";
    info << "Outputs: " << (int)pImpl->outputNames.size() << "\n";
    if (!pImpl->inputShape.empty()) {
      info << "Input Shape: [";
      for (size_t i = 0; i < pImpl->inputShape.size(); ++i) {
        info << pImpl->inputShape[i];
        if (i < pImpl->inputShape.size() - 1)
          info << ", ";
      }
      info << "]\n";
    }
  } else {
    info << "ONNX Runtime: Session not initialized\n";
  }
#else
  info << "ONNX Runtime: Not compiled\n";
  info << "Using: DSP fallback only\n";
#endif

  return info;
}

juce::File ONNXStemSeparator::findDefaultModel() {
    return PlatformModelUtils::findDefaultModel();
}

} // namespace zenith
