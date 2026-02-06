/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
