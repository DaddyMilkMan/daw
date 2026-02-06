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

#include "FontManager.h"

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
#include <core/SkFontMgr.h>
#if ZENITH_ENABLE_SKIA
#include <core/SkRefCnt.h>
#endif
#endif

namespace zenith {
namespace design {

class PlatformFontUtils {
public:
    /**
     * Creates a new platform-native font manager.
     */
    static sk_sp<SkFontMgr> createDefaultFontManager();
};

} // namespace design
} // namespace zenith
