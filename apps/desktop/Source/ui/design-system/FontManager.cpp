/*
    FontManager.cpp - Font management for Zenith DAW

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

#include "ui/design-system/FontManager.h"

namespace zenith {

juce::Font FontManager::getFont(float fontSize, bool bold) {
    juce::Font font(juce::jmax(10.0f, fontSize));
    if (bold) {
        font = font.boldened();
    }
    return font;
}

juce::Font FontManager::getMonospaceFont(float fontSize) {
    return juce::Font(juce::Typeface::createSystemTypefaceFor(juce::Font::getDefaultMonospaceFontName())
                      .get(), juce::jmax(10.0f, fontSize));
}

juce::Font FontManager::getDefaultUIFont() {
    return juce::Font(14.0f);
}

} // namespace zenith