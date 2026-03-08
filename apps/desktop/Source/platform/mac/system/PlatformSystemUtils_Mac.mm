/*
  ==============================================================================
    PlatformSystemUtils_Mac.mm
    macOS-specific system utilities implementation.
  ==============================================================================
*/

#ifdef __APPLE__
#include "../../../utils/PlatformSystemUtils.h"
#include "../../../engine/ZenithLogger.h"
#include <juce_core/juce_core.h>

namespace zenith {

void PlatformSystemUtils::logSystemInfo() {
    ZENITH_LOG_INFO("--- System Information (macOS) ---");
    ZENITH_LOG_INFO("OS: " + juce::SystemStats::getOperatingSystemName());
    ZENITH_LOG_INFO("CPU: " + juce::SystemStats::getCpuVendor() + " " + juce::SystemStats::getCpuModel());
    ZENITH_LOG_INFO("RAM: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes() / 1024.0, 1) + " GB");
    ZENITH_LOG_INFO("----------------------------------");
}

juce::String PlatformSystemUtils::getSystemInfoString() {
    juce::String info;
    info << juce::SystemStats::getOperatingSystemName() << " | "
         << juce::SystemStats::getCpuModel() << " | "
         << (juce::SystemStats::getMemorySizeInMegabytes() / 1024) << "GB RAM";
    return info;
}

} // namespace zenith
#endif // __APPLE__
