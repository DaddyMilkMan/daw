/*
  ==============================================================================

    PlatformModelUtils_Mac.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../dsp/PlatformModelUtils.h"

#ifdef __APPLE__
namespace zenith {

juce::File PlatformModelUtils::findDefaultModel() {
    const char* modelNames[] = { "htdemucs.onnx", "demucs.onnx" };
    
    // 1. Check Bundle Resources
    juce::File bundleRes = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                           .getChildFile("Contents/Resources/models");
    
    for (const char* name : modelNames) {
        juce::File f = bundleRes.getChildFile(name);
        if (f.existsAsFile()) return f;
    }

    // 2. Fallback to exe dir (for dev)
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    for (const char* name : modelNames) {
        juce::File f = exeDir.getChildFile(name);
        if (f.existsAsFile()) return f;
    }

    return juce::File();
}

} // namespace zenith
#endif
