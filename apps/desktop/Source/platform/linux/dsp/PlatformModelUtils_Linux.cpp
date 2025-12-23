/*
  ==============================================================================

    PlatformModelUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../dsp/PlatformModelUtils.h"

#ifdef __linux__
namespace zenith {

juce::File PlatformModelUtils::findDefaultModel() {
    const char* modelNames[] = { "htdemucs.onnx", "demucs.onnx" };
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    
    // 1. Check adjacent to exe and dev paths
    for (const char* name : modelNames) {
        juce::File f = exeDir.getChildFile(name);
        if (f.existsAsFile()) return f;

        f = exeDir.getChildFile("models").getChildFile(name);
        if (f.existsAsFile()) return f;
        
        f = exeDir.getChildFile("Resources/models").getChildFile(name);
        if (f.existsAsFile()) return f;
    }

    // 2. Linux Standard Paths
    juce::Array<juce::File> searchPaths;
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".local/share/zenith/models"));
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".config/ZenithDAW/models"));
    searchPaths.add(juce::File("/usr/share/zenith/models"));

    for (auto& path : searchPaths) {
        for (const char* name : modelNames) {
            juce::File f = path.getChildFile(name);
            if (f.existsAsFile()) return f;
        }
    }

    return juce::File();
}

} // namespace zenith
#endif
