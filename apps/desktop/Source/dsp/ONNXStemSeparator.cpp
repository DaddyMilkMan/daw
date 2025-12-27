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

  ~Impl() {
      for (auto name : inputNames) delete[] name;
      for (auto name : outputNames) delete[] name;
  }
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
    DBG("ONNXStemSeparator: ONNX Runtime environment initialized [v" + juce::String(ORT_API_VERSION) + "]");
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

  // Validate file size (model should be at least 100KB for a minimal valid model)
  auto fileSize = fileToLoad.getSize();
  if (fileSize < 100 * 1024) {
    DBG("ONNXStemSeparator: Model file too small (" + 
        juce::String(fileSize) + " bytes) - likely corrupt or incomplete");
    return false;
  }

  // Validate ONNX file header - check if file is readable
  {
    juce::FileInputStream stream(fileToLoad);
    if (!stream.openedOk()) {
      DBG("ONNXStemSeparator: Could not open model file for validation");
      return false;
    }
    
    // Read first bytes to verify file accessibility and basic structure
    char header[8];
    if (stream.read(header, 8) != 8) {
      DBG("ONNXStemSeparator: Model file unreadable - corrupt or empty");
      return false;
    }

    // ONNX protobuf header validation:
    // ONNX files typically start with protobuf field tags:
    // 0x08 = ir_version (field 1, varint)
    // 0x12 = producer_name (field 2, length-delimited)
    // 0x0A = also valid for some ONNX variants
    // If none match, log warning but let ONNX Runtime make final determination
    if (header[0] != 0x08 && header[0] != 0x12 && header[0] != 0x0A) {
      DBG("ONNXStemSeparator: Unexpected ONNX header byte 0x" + 
          juce::String::toHexString((int)(unsigned char)header[0]) +
          " - file may be corrupt or not ONNX format");
      // Continue anyway - ONNX Runtime will give definitive answer
    }
  }

  // Reset any previous state before loading new model
#ifdef ZENITH_USE_ONNX_RUNTIME
  for (auto name : pImpl->inputNames) delete[] name;
  for (auto name : pImpl->outputNames) delete[] name;
  pImpl->inputNames.clear();
  pImpl->outputNames.clear();
  pImpl->session.reset();
  pImpl->sessionOptions.reset();
  pImpl->inputShape.clear();
  pImpl->inputTensorValues.clear();
#endif
  pImpl->isLoaded = false;

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
    std::wstring wModelPath = fileToLoad.getFullPathName().toWideCharPointer();
    pImpl->session = std::make_unique<Ort::Session>(
        *pImpl->env, wModelPath.c_str(), *pImpl->sessionOptions);
#else
    // Unix systems use regular strings
    std::string sModelPath = fileToLoad.getFullPathName().toStdString();
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
      inputNameAllocated.release(); // Transfer ownership to vector (manual management for C API wrapper)
      // Actually, Ort::AllocatedStringPtr manages it, but we need it in inputNames (const char*)
      // The push_back(get()) is correct as long as we store the AllocatedStringPtr somewhere.
      // Wait, let's fix this memory management.
    }
    
    // REDO: Robust metadata loading
    pImpl->inputNames.clear();
    pImpl->outputNames.clear();
    
    for (size_t i = 0; i < pImpl->session->GetInputCount(); ++i) {
        auto name = pImpl->session->GetInputNameAllocated(i, allocator);
        char* nameStr = new char[strlen(name.get()) + 1];
        strcpy(nameStr, name.get());
        pImpl->inputNames.push_back(nameStr);
    }
    
    for (size_t i = 0; i < pImpl->session->GetOutputCount(); ++i) {
        auto name = pImpl->session->GetOutputNameAllocated(i, allocator);
        char* nameStr = new char[strlen(name.get()) + 1];
        strcpy(nameStr, name.get());
        pImpl->outputNames.push_back(nameStr);
    }

    if (!pImpl->inputNames.empty()) {
        Ort::TypeInfo inputTypeInfo = pImpl->session->GetInputTypeInfo(0);
        auto tensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
        pImpl->inputShape = tensorInfo.GetShape();

        juce::String shapeStr = "[";
        for(size_t i=0; i<pImpl->inputShape.size(); ++i) {
            shapeStr << pImpl->inputShape[i] << (i < pImpl->inputShape.size()-1 ? ", " : "]");
        }
        DBG("ONNXStemSeparator: Input shape - " + shapeStr);
    }

    pImpl->isLoaded = true;
    DBG("ONNXStemSeparator: Model loaded successfully - " +
        fileToLoad.getFileName());
    DBG("ONNXStemSeparator: Inputs: " + juce::String((int)numInputs) +
        ", Outputs: " + juce::String((int)pImpl->outputNames.size()));

    return true;
  } catch (const Ort::Exception &e) {
    juce::String errorMsg = e.what();
    DBG("ONNXStemSeparator: ONNX Runtime error - " + errorMsg);
    
    // Classify the error for better diagnostics
    if (errorMsg.containsIgnoreCase("protobuf") ||
        errorMsg.containsIgnoreCase("Invalid model") ||
        errorMsg.containsIgnoreCase("parse")) {
      DBG("ONNXStemSeparator: Model file appears corrupt or invalid ONNX format");
    } else if (errorMsg.containsIgnoreCase("version") ||
               errorMsg.containsIgnoreCase("opset")) {
      DBG("ONNXStemSeparator: Model requires different ONNX Runtime version or opset");
    } else if (errorMsg.containsIgnoreCase("operator") ||
               errorMsg.containsIgnoreCase("op type")) {
      DBG("ONNXStemSeparator: Model uses unsupported operators for this runtime");
    } else if (errorMsg.containsIgnoreCase("memory") ||
               errorMsg.containsIgnoreCase("alloc")) {
      DBG("ONNXStemSeparator: Insufficient memory to load model");
    }
    
    pImpl->isLoaded = false;
    return false;
  } catch (const std::exception& e) {
    DBG("ONNXStemSeparator: Unexpected error loading model - " + 
        juce::String(e.what()));
    pImpl->isLoaded = false;
    return false;
  } catch (...) {
    DBG("ONNXStemSeparator: Unknown exception while loading model");
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
      
      // Copy audio data to contiguous buffer (planar)
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
      auto startInference = juce::Time::getMillisecondCounterHiRes();
      auto outputTensors = pImpl->session->Run(
          Ort::RunOptions{nullptr}, pImpl->inputNames.data(), &inputTensor, 1,
          pImpl->outputNames.data(), pImpl->outputNames.size());
      auto endInference = juce::Time::getMillisecondCounterHiRes();
      double inferenceDurationMs = endInference - startInference;

      // Process outputs
      // Assuming model outputs 4 tensors: Drums, Bass, Other, Vocals (Standard Demucs Order)
      // We map them to our result structure
      if (outputTensors.size() >= 4) {
        // Output 0: Drums
        copyTensorToBuffer(outputTensors[0].GetTensorMutableData<float>(), result.drums, numChannels, numSamples);
        
        // Output 1: Bass
        copyTensorToBuffer(outputTensors[1].GetTensorMutableData<float>(), result.bass, numChannels, numSamples);
        
        // Output 2: Other
        copyTensorToBuffer(outputTensors[2].GetTensorMutableData<float>(), result.other, numChannels, numSamples);
        
        // Output 3: Vocals
        copyTensorToBuffer(outputTensors[3].GetTensorMutableData<float>(), result.vocals, numChannels, numSamples);

        result.success = true;
        result.usedONNX = true;

        DBG("ONNXStemSeparator: Inference completed in " + juce::String(inferenceDurationMs, 1) + "ms (4 stems)");
        return result;

      } else {
        DBG("ONNXStemSeparator: Unexpected number of outputs - " + juce::String((int)outputTensors.size()));
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
