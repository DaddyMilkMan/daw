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

    PlatformFontUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-specific font utility functions.

  ==============================================================================

*/

#include "PlatformFontUtils.h"

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA

#if defined(WIN32) || defined(_WIN32)
#include <ports/SkTypeface_win.h>
#elif defined(__APPLE__)
#include <ports/SkTypeface_mac.h>
#elif defined(__linux__)
#include <ports/SkFontMgr_fontconfig.h>
#include <core/SkFontScanner.h>
#include <ports/SkFontScanner_FreeType.h>
#else
#include <ports/SkFontMgr_empty.h>
#endif

#endif

namespace zenith {
namespace design {

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
sk_sp<SkFontMgr> PlatformFontUtils::createDefaultFontManager()
{
#if defined(WIN32) || defined(_WIN32)
    return SkFontMgr_New_DirectWrite();
#elif defined(__APPLE__)
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(__linux__)
    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    return SkFontMgr_New_Custom_Empty();
#endif
}
#endif

} // namespace design
} // namespace zenith
