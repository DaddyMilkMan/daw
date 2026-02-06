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


// ZenithSkia.h is used throughout the UI codebase as the single include point
// for Skia types. Using "mock" types when Skia is disabled breaks the build
// because other headers (e.g. SkiaComponent) include real Skia headers
// unconditionally.
//
// Keep this header as a thin wrapper around the real Skia headers. When Skia
// rendering is disabled, the project should disable *usage* of Skia contexts
// and peers via compile-time flags, not by redefining core Skia types.
#include <core/SkBlurTypes.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <core/SkShader.h>
#include <core/SkSurface.h>
