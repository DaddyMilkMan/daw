#pragma once

/**
 * ZenithSkia.h
 * Centralized Skia include manager.
 * Handles mocking when Skia is disabled.
 */

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <core/SkShader.h>
#include <core/SkSurface.h>
#include <include/core/SkBlurTypes.h>
#else
#include "design-system/ZenithDesignSystem.h"
// Additional mocks if needed
class SkPath {
public:
  void reset() {}
};
class SkShader;
enum SkBlurStyle {
  kNormal_SkBlurStyle,
  kSolid_SkBlurStyle,
  kOuter_SkBlurStyle,
  kInner_SkBlurStyle
};

class SkMaskFilter {
public:
  static sk_sp<SkMaskFilter> MakeBlur(SkBlurStyle, float) { return nullptr; }
};

struct SkPoint {
  float fX, fY;
};
struct SkSamplingOptions {};
enum class SkTileMode { kClamp };

class SkGradientShader {
public:
  static sk_sp<SkShader> MakeLinear(const SkPoint *, const SkColor *,
                                    const float *, int, SkTileMode) {
    return nullptr;
  }
};
#endif
