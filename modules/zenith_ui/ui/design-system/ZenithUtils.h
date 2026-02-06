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

#include "ZenithDesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
#include "ZenithSkia.h"
#endif

namespace zenith {

namespace design {

class ZenithUtils {
public:
    static SkRect toSkRect(const juce::Rectangle<int>& rect) {
        return SkRect::MakeXYWH((float)rect.getX(), (float)rect.getY(),
                                (float)rect.getWidth(), (float)rect.getHeight());
    }

    static SkRect toSkRect(const juce::Rectangle<float>& rect) {
        return SkRect::MakeXYWH(rect.getX(), rect.getY(),
                                rect.getWidth(), rect.getHeight());
    }
};

} // namespace design
} // namespace zenith
