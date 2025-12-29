/*
  ==============================================================================

    PlatformONNXUtils_Linux.cpp
    Created: 2025-12-23

    Linux-specific ONNX Runtime utilities: Standard string path handling

  ==============================================================================
*/

#ifdef __linux__
#ifdef ZENITH_USE_ONNX_RUNTIME
#include "../../../dsp/PlatformONNXUtils.h"

#include <juce_core/juce_core.h>
#include <onnxruntime_cxx_api.h>
#include <string>

namespace zenith {

std::unique_ptr<Ort::Session>
PlatformONNXUtils::createSession(Ort::Env &env, Ort::SessionOptions &options,
                                 const juce::File &modelPath) {
  // Unix systems use regular strings
  std::string sModelPath = modelPath.getFullPathName().toStdString();
  return std::make_unique<Ort::Session>(env, sModelPath.c_str(), options);
}

} // namespace zenith
#endif
#endif
