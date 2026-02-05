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

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Engine.h"
#include "ProjectState.h"
#include "../instruments/InstrumentRegistry.h"
#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"

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
        if (!canvas) {
            return;
        }

        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());

        // Background
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        bgPaint.setColor(design::colors::BG_00);
        canvas->drawRect(SkRect::MakeWH(width, height), bgPaint);

        // Header
        SkPaint headerPaint;
        headerPaint.setAntiAlias(true);
        headerPaint.setColor(design::colors::BG_02);
        canvas->drawRect(SkRect::MakeXYWH(0.0f, 0.0f, width, 30.0f), headerPaint);

        SkPaint titlePaint;
        titlePaint.setAntiAlias(true);
        titlePaint.setColor(design::colors::TEXT_PRIMARY);
        SkFont titleFont = design::typography::getSkFont(16.0f, design::FontWeight::Bold);
        canvas->drawString("Instruments", 10.0f, 20.0f, titleFont, titlePaint);

        // List
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(design::colors::TEXT_PRIMARY);
        SkFont listFont = design::typography::getSkFont(14.0f, design::FontWeight::Regular);

        float y = 40.0f;

        if (instrumentIds_.isEmpty()) {
            SkPaint emptyPaint = textPaint;
            emptyPaint.setColor(design::colors::TEXT_SECONDARY);
            canvas->drawString("No instruments found.", 20.0f, y + 16.0f, listFont, emptyPaint);
            return;
        }

        for (const auto& id : instrumentIds_) {
            canvas->drawString(id.toStdString().c_str(), 20.0f, y + 16.0f, listFont, textPaint);

            // Separator
            SkPaint separatorPaint;
            separatorPaint.setAntiAlias(false);
            separatorPaint.setColor(design::colors::BORDER_SUBTLE);
            canvas->drawRect(SkRect::MakeXYWH(10.0f, y + 24.0f, width - 20.0f, 1.0f), separatorPaint);

            y += 28.0f;
        }
    }
    
    void refreshInstruments() {
        // Fetch available instruments from the registry
        instrumentIds_ = engine_.getInstrumentRegistry().getInstrumentIds();
        markDirty();
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
