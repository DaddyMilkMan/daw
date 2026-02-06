/*
    FontManager.h - Font management for Zenith DAW

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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class FontManager
 * @brief Font management for Zenith DAW
 *
 * This class provides font management for the UI.
 */
class FontManager {
public:
    /**
     * @brief Get font by size and weight
     * @param fontSize Font size
     * @param bold Whether the font should be bold
     * @return juce::Font for the specified parameters
     */
    static juce::Font getFont(float fontSize = 14.0f, bool bold = false);

    /**
     * @brief Get monospace font
     * @param fontSize Font size
     * @return juce::Font for monospace text
     */
    static juce::Font getMonospaceFont(float fontSize = 12.0f);

    /**
     * @brief Get the default font for UI elements
     * @return juce::Font for UI elements
     */
    static juce::Font getDefaultUIFont();

private:
    FontManager() = delete;  // Static class - prevent instantiation
};

} // namespace zenith