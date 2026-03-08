/*
  ==============================================================================

    PlatformSystemUtils_Windows.cpp
    Created: 2026-02-17

    Windows-specific system utilities implementation.

  ==============================================================================
*/

#ifdef _WIN32
#include "../../../utils/PlatformLogUtils.h"
#include "../../../utils/PlatformSystemUtils.h"
#include <juce_core/juce_core.h>

namespace zenith {

void PlatformSystemUtils::logSystemInfo() {
    ZENITH_LOG_INFO("--- System Information (Windows) ---");
    ZENITH_LOG_INFO("OS: " + juce::SystemStats::getOperatingSystemName());
    ZENITH_LOG_INFO("CPU: " + juce::SystemStats::getCpuVendor() + " " + juce::SystemStats::getCpuModel());
    ZENITH_LOG_INFO("Cores: " + juce::String(juce::SystemStats::getNumCpus()) + " (" + juce::String(juce::SystemStats::getNumPhysicalCpus()) + " physical)");
    ZENITH_LOG_INFO("RAM: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes() / 1024.0, 1) + " GB");
    ZENITH_LOG_INFO("Screen: " + juce::String(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea.getWidth()) + "x" + juce::String(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea.getHeight()));
    ZENITH_LOG_INFO("------------------------------------");
}

juce::String PlatformSystemUtils::getSystemInfoString() {
    juce::String info;
    info << juce::SystemStats::getOperatingSystemName() << " | "
         << juce::SystemStats::getCpuModel() << " | "
         << (juce::SystemStats::getMemorySizeInMegabytes() / 1024) << "GB RAM";
    return info;
}

} // namespace zenith
#endif // _WIN32
