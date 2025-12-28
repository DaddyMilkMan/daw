/*
  ==============================================================================

    PlatformModelUtils_Windows.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "dsp/PlatformModelUtils.h"

#ifdef _WIN32
namespace zenith {

juce::File PlatformModelUtils::findDefaultModel() {
    const char* modelNames[] = { "htdemucs.onnx", "demucs.onnx" };
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    
    for (const char* name : modelNames) {
        // adjacent to exe
        juce::File f = exeDir.getChildFile(name);
        if (f.existsAsFile()) return f;

        // models/
        f = exeDir.getChildFile("models").getChildFile(name);
        if (f.existsAsFile()) return f;
        
        // Resources/models/
        f = exeDir.getChildFile("Resources/models").getChildFile(name);
        if (f.existsAsFile()) return f;
        
        // AppData
        f = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("ZenithDAW/models").getChildFile(name);
        if (f.existsAsFile()) return f;
    }

    return juce::File();
}

} // namespace zenith
#endif
