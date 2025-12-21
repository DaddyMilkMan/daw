#pragma once

#include <include/core/SkRect.h>
#include <juce_gui_basics/juce_gui_basics.h>

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
