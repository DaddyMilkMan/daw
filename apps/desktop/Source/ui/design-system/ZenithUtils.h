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
