/*
  ==============================================================================

    ZenithDesignSystem.cpp
    Created: 2025-12-01
    Author:  Zenith DAW

    Implementation of global design settings.

  ==============================================================================
*/

#include "ZenithDesignSystem.h"

namespace zenith {
namespace design {

float Settings::glowIntensity = 1.0f;
float Settings::uiScale = 1.0f;
Settings::Theme Settings::currentTheme = Settings::Theme::NeonNoir;

} // namespace design
} // namespace zenith
