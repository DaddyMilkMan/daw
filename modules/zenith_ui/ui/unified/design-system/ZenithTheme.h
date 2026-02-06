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

#pragma once

// ZenithTheme.h


#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace zenith {

/**
 * @class ZenithTheme
 * @brief Design system theme and styling for unified UI components
 *
 * This class provides color schemes, fonts, and visual constants
 * for the unified UI framework.
 */
class ZenithTheme {
public:
    //==============================================================================
    // Color Scheme
    //==============================================================================

    /**
     * @brief Primary color palette
     */
    enum class Color {
        Background,
        Surface,
        SurfaceVariant,
        Primary,
        PrimaryVariant,
        Secondary,
        SecondaryVariant,
        Accent,
        Error,
        Success,
        Warning,
        TextPrimary,
        TextSecondary,
        TextDisabled,
        Divider,
        Shadow
    };

    //==============================================================================
    // Typography
    //==============================================================================

    /**
     * @brief Font sizes
     */
    enum class FontSize {
        Caption,
        Body,
        Subtitle,
        Title,
        Headline
    };

    //==============================================================================
    // Spacing
    //==============================================================================

    /**
     * @brief Spacing constants
     */
    enum class Spacing {
        XS = 4,    // 4px
        SM = 8,    // 8px
        MD = 16,   // 16px
        LG = 24,   // 24px
        XL = 32,   // 32px
        XXL = 48   // 48px
    };

    //==============================================================================
    // Border Radius
    //==============================================================================

    /**
     * @brief Border radius constants
     */
    enum class BorderRadius {
        None = 0,
        SM = 4,
        MD = 8,
        LG = 12,
        XL = 16,
        Full = 9999
    };

    //==============================================================================
    // Shadows
    //==============================================================================

    /**
     * @brief Shadow levels
     */
    enum class Shadow {
        None,
        SM,
        MD,
        LG,
        XL
    };

    //==============================================================================
    // Methods
    //==============================================================================

    /**
     * @brief Get color by type
     * @param color Color type
     * @return juce::Color for the specified color
     */
    static juce::Color getColor(Color color);

    /**
     * @brief Get font by size
     * @param size Font size
     * @return juce::Font for the specified size
     */
    static juce::Font getFont(FontSize size);

    /**
     * @brief Get spacing value
     * @param spacing Spacing type
     * @return Spacing value in pixels
     */
    static float getSpacing(Spacing spacing);

    /**
     * @brief Get border radius
     * @param radius Border radius type
     * @return Border radius value in pixels
     */
    static float getBorderRadius(BorderRadius radius);

    /**
     * @brief Get shadow parameters
     * @param shadow Shadow level
     * @return Shadow color and blur radius
     */
    static std::pair<juce::Color, float> getShadow(Shadow shadow);

    //==============================================================================
    // Theme Variants
    //==============================================================================

    /**
     * @brief Set theme variant
     * @param variant Theme variant ("light" or "dark")
     */
    static void setThemeVariant(const juce::String& variant);

    /**
     * @brief Get current theme variant
     * @return Current theme variant
     */
    static juce::String getThemeVariant();

private:
    //==============================================================================
    // Members
    //==============================================================================

    static juce::String currentThemeVariant_;
};

} // namespace zenith