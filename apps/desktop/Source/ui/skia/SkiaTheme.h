#pragma once
#include <juce_graphics/juce_graphics.h>
#include "ZenithDesignSystem.h"

namespace zenith {

class SkiaTheme {
public:
    static SkiaTheme& getInstance() {
        static SkiaTheme instance;
        return instance;
    }

    struct Colors {
        SkColor bg2 = design::colors::BG_DARKER;
        SkColor bg3 = design::colors::BG_LIGHT;
        SkColor borderSubtle = design::colors::BORDER_SUBTLE;
        SkColor borderStrong = design::colors::BORDER_STRONG;
        SkColor accentMain = design::colors::CYAN;
        SkColor waveformMidi = design::colors::NEON_GREEN;
        SkColor waveformAudio = design::colors::BLUE;
        SkColor textMuted = design::colors::TEXT_SECONDARY;
        SkColor textStrong = design::colors::TEXT_PRIMARY;
    };

    struct FontStyle {
        float size = 14.0f;
        bool bold = false;
    };

    struct Typography {
        FontStyle header{16.0f, true};
        FontStyle body{14.0f, false};
        FontStyle small{12.0f, false};
    };

    struct Interaction {
        SkColor hoverOverlay = design::colors::GLASS_10;
    };

    const Colors& getColors() const { return colors_; }
    const Typography& getTypography() const { return typography_; }
    const Interaction& getInteraction() const { return interaction_; }

private:
    SkiaTheme() {}
    Colors colors_;
    Typography typography_;
    Interaction interaction_;
};

}
