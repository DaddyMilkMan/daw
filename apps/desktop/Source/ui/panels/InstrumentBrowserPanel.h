#pragma once
#include "../framework/SkiaComponent.h"
#include "Engine.h"
#include "ProjectState.h"
#include "../instruments/InstrumentRegistry.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkFont.h>

namespace zenith {

class InstrumentBrowserPanel : public SkiaComponent {
public:
    InstrumentBrowserPanel(Engine& engine, ProjectState& state) 
        : engine_(engine) 
    {
        juce::ignoreUnused(state);
        refreshInstruments();
    }

    void drawSkia(SkCanvas* canvas) override {
        SkPaint bg;
        bg.setColor(zenith::design::unified::bg_00());
        canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), bg);

        SkPaint headerBg;
        headerBg.setColor(zenith::design::unified::bg_02());
        canvas->drawRect(SkRect::MakeXYWH(0, 0, (float)getWidth(), 30.0f), headerBg);

        SkPaint text;
        text.setAntiAlias(true);
        text.setColor(zenith::design::unified::text_primary());
        SkFont titleFont = zenith::design::getSkFont(16.0f, zenith::design::FontWeight::Bold);
        canvas->drawString("Instruments", 10.0f, 20.0f, titleFont, text);

        int y = 40;
        SkFont rowFont = zenith::design::getSkFont(14.0f, zenith::design::FontWeight::Regular);
        if (instrumentIds_.isEmpty()) {
             text.setColor(zenith::design::unified::text_secondary());
             canvas->drawString("No instruments found.", 20.0f, 64.0f, rowFont, text);
             return;
        }

        for (const auto& id : instrumentIds_) {
            text.setColor(zenith::design::unified::text_primary());
            canvas->drawString(id.toRawUTF8(), 20.0f, (float)y + 16.0f, rowFont, text);

            SkPaint sep;
            sep.setColor(zenith::design::unified::border_subtle());
            canvas->drawRect(SkRect::MakeXYWH(10.0f, (float)y + 24.0f, (float)getWidth() - 20.0f, 1.0f), sep);
            y += 28;
        }
    }
    
    void refreshInstruments() {
        // Fetch available instruments from the registry
        instrumentIds_ = engine_.getInstrumentRegistry().getInstrumentIds();
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        juce::ignoreUnused(e);
        // Simple click handling to refresh or select (future)
        refreshInstruments();
    }

private:
    Engine& engine_;
    juce::StringArray instrumentIds_;
};

} // namespace zenith
