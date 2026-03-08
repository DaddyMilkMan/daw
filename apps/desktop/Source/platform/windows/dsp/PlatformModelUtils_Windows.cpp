/*
  ==============================================================================

    PlatformModelUtils_Windows.cpp
    Created: 2026-02-17

    Windows implementation for locating AI/DSP model files.

  ==============================================================================
*/

#ifdef _WIN32
#include "dsp/PlatformModelUtils.h"

namespace zenith {

juce::File PlatformModelUtils::findDefaultModel() {
    const char* modelNames[] = { "htdemucs.onnx", "demucs.onnx" };
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    // 1. Check adjacent to exe and common dev paths
    for (const char* name : modelNames) {
        juce::File f = exeDir.getChildFile(name);
        if (f.existsAsFile()) return f;

        f = exeDir.getChildFile("models").getChildFile(name);
        if (f.existsAsFile()) return f;

        f = exeDir.getChildFile("Resources/models").getChildFile(name);
        if (f.existsAsFile()) return f;
    }

    // 2. Windows Standard Paths
    juce::Array<juce::File> searchPaths;

    // %APPDATA%\ZenithDAW\models  (roaming, per-user)
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("ZenithDAW").getChildFile("models"));

    // %LOCALAPPDATA%\ZenithDAW\models  (local, per-user)
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getSiblingFile("Local")
                        .getChildFile("ZenithDAW").getChildFile("models"));

    // Program Files\ZenithDAW\models  (system-wide install)
    searchPaths.add(juce::File("C:\\Program Files\\ZenithDAW\\models"));
    searchPaths.add(juce::File("C:\\Program Files (x86)\\ZenithDAW\\models"));

    for (auto& path : searchPaths) {
        for (const char* name : modelNames) {
            juce::File f = path.getChildFile(name);
            if (f.existsAsFile()) return f;
        }
    }

    return juce::File();
}

} // namespace zenith
#endif // _WIN32
