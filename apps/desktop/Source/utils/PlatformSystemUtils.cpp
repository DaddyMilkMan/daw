/*
  ==============================================================================

    PlatformSystemUtils.cpp
    Created: 2025-12-22
    Author:  Zenith DAW

  ==============================================================================
*/

#include "PlatformSystemUtils.h"

namespace zenith {

void PlatformSystemUtils::logSystemInfo() {
    juce::Logger::writeToLog("System Info:");
    juce::Logger::writeToLog("  OS: " + juce::SystemStats::getOperatingSystemName());
    juce::Logger::writeToLog("  CPU: " + juce::SystemStats::getCpuVendor() + " " + juce::SystemStats::getCpuModel());
    juce::Logger::writeToLog("  Cores: " + juce::String(juce::SystemStats::getNumCpus()));
    juce::Logger::writeToLog("  Memory: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
    juce::Logger::writeToLog("  Language: " + juce::SystemStats::getUserLanguage());
    juce::Logger::writeToLog("  Region: " + juce::SystemStats::getUserRegion());
}

juce::String PlatformSystemUtils::getSystemInfoString() {
    juce::String info;
    info << "OS: " << juce::SystemStats::getOperatingSystemName() << "\n";
    info << "CPU: " << juce::SystemStats::getCpuVendor() << " " << juce::SystemStats::getCpuModel() << "\n";
    info << "RAM: " << juce::SystemStats::getMemorySizeInMegabytes() << " MB";
    return info;
}

} // namespace zenith
