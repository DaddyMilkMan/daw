/*
  ==============================================================================

    PlatformONNXUtils_Windows.cpp
    Created: 2025-12-23

    Windows-specific ONNX Runtime utilities: Wide-string path handling

  ==============================================================================
*/

#ifdef _WIN32
#ifdef ZENITH_USE_ONNX_RUNTIME
#include <juce_core/juce_core.h>
#include <onnxruntime_cxx_api.h>
#include <string>

namespace zenith {

std::unique_ptr<Ort::Session>
PlatformONNXUtils::createSession(Ort::Env &env, Ort::SessionOptions &options,
                                 const juce::File &modelPath) {
  // Windows uses wide strings for file paths
  std::wstring wModelPath = modelPath.getFullPathName().toWideCharPointer();
  return std::make_unique<Ort::Session>(env, wModelPath.c_str(), options);
}

} // namespace zenith
#endif
#endif
