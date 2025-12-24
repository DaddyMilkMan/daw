/*
  ==============================================================================

    PlatformONNXUtils_Mac.cpp
    Created: 2025-12-23

    Mac-specific ONNX Runtime utilities: Standard string path handling

  ==============================================================================
*/

#ifdef __APPLE__
#ifdef ZENITH_USE_ONNX_RUNTIME
#include <juce_core/juce_core.h>
#include <onnxruntime_cxx_api.h>
#include <string>

namespace zenith {

std::unique_ptr<Ort::Session>
PlatformONNXUtils::createSession(Ort::Env &env, Ort::SessionOptions &options,
                                 const juce::File &modelPath) {
  // macOS uses regular strings (like Linux)
  std::string sModelPath = modelPath.getFullPathName().toStdString();
  return std::make_unique<Ort::Session>(env, sModelPath.c_str(), options);
}

} // namespace zenith
#endif
#endif
