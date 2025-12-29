/*
  ==============================================================================

    PlatformModelUtils.cpp
    Created: 2025-12-22
    Author:  Zenith DAW

  ==============================================================================
*/

#include "PlatformModelUtils.h"

namespace zenith {

juce::File PlatformModelUtils::findDefaultModel() {
    // Check common locations
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW").getChildFile("Models");
    
    auto modelFile = appDataDir.getChildFile("htdemucs.onnx");
    if (modelFile.existsAsFile()) return modelFile;

    // Check executable directory
    auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    modelFile = exeDir.getChildFile("htdemucs.onnx");
    if (modelFile.existsAsFile()) return modelFile;
    
    modelFile = exeDir.getChildFile("Resources").getChildFile("htdemucs.onnx");
    if (modelFile.existsAsFile()) return modelFile;

    return juce::File();
}

} // namespace zenith
