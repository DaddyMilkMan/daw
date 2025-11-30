#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {
class SkiaContextManager {
public:
    static SkiaContextManager& getInstance() {
        static SkiaContextManager instance;
        return instance;
    }
};
}
