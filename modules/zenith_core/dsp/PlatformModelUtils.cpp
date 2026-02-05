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

/*
    ==============================================================================
    Original file header:
*/

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
