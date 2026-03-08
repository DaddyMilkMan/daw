#pragma once

/*
  Legacy synth-specific theme retained for the IndustrialUI editor.
  It is separate from the application's primary matte-black /
  restrained-blue visual direction.
*/

#include "../../ZenithSkia.h"
#include "../layout/ModeManager.h"
#include <array>
#include <string>

#ifdef ZENITH_USE_SKIA
#include <core/SkTypeface.h>
#endif

namespace zenith::industrial {

struct IndustrialTheme {
  static constexpr SkColor CARBON_BLACK = 0xFF0A0A0A;
  static constexpr SkColor GRAPHITE = 0xFF141414;
  static constexpr SkColor STEEL_DARK = 0xFF1E1E1E;
  static constexpr SkColor STEEL = 0xFF2A2A2A;
  static constexpr SkColor ALUMINUM = 0xFF3A3A3A;
  static constexpr SkColor SILVER = 0xFF6A6A6A;
  static constexpr SkColor LIGHT_GRAY = 0xFFAAAAAA;
  static constexpr SkColor WHITE = 0xFFFFFFFF;
  static constexpr SkColor AMBER = 0xFFFF8800;
  static constexpr SkColor RED = 0xFF444444;
  static constexpr SkColor GREEN = 0xFF44CC88;
  static constexpr SkColor CYAN = 0xFF44CCFF;

  enum class FontWeight {
    Regular,
    Medium,
    SemiBold,
    Bold
  };

  void initializeFonts();

#ifdef ZENITH_USE_SKIA
  sk_sp<SkTypeface> getTypeface(FontWeight weight) const;
#endif

  float carbonContrastForMode(UIMode mode) const;

#ifdef ZENITH_USE_SKIA
  void drawMetallicGradient(SkCanvas* canvas, const SkRect& rect, float angleDegrees) const;
  void drawHexScrew(SkCanvas* canvas, float cx, float cy, float radius) const;
#endif

private:
#ifdef ZENITH_USE_SKIA
  std::array<sk_sp<SkTypeface>, 4> typefaces_{};
#endif
};

} // namespace zenith::industrial
