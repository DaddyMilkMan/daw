/*
  ==============================================================================

    PlatformONNXUtils.h
    Created: 2025-12-23

    Platform-agnostic ONNX Runtime utilities interface

  ==============================================================================
*/

#pragma once

#ifdef ZENITH_USE_ONNX_RUNTIME
#include <juce_core/juce_core.h>
#include <memory>
#include <onnxruntime_cxx_api.h>

namespace zenith {

class PlatformONNXUtils {
public:
  /**
   * @brief Create ONNX Runtime session with platform-specific path handling
   * @param env ONNX Runtime environment
   * @param options Session options
   * @param modelPath Path to the ONNX model file
   * @return Unique pointer to created session
   * @note Windows uses wide-string paths, Unix uses standard strings
   */
  static std::unique_ptr<Ort::Session>
  createSession(Ort::Env &env, Ort::SessionOptions &options,
                const juce::File &modelPath);
};

} // namespace zenith
#endif
