/*
  ==============================================================================

    ZenithDesignSystem.cpp
    Created: 2025-11-30
    Authors: Sarah Chen

    Implementation of global design system settings.
    
  ==============================================================================
*/

#include "ZenithDesignSystem.h"

namespace zenith {
namespace design {

// Initialize global settings
float Settings::glowIntensity = 1.0f;
float Settings::uiScale = 1.0f;
Settings::Theme Settings::currentTheme = Settings::Theme::NeonNoir;

} // namespace design
} // namespace zenith
