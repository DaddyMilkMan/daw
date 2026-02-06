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

/**
 * ZenithSkia.h
 * Centralized Skia include manager.
 * Handles mocking when Skia is disabled.
 */


#if ZENITH_ENABLE_SKIA
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <core/SkShader.h>
#include <core/SkSurface.h>
#include <core/SkBlurTypes.h>
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
