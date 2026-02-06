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

#include <juce_core/juce_core.h>
#include "Engine.h"
#include "browser/BrowserModel.h"
#include "zenith_network/network/AudioAnalysisService.h"

namespace zenith {

class GrokGodModeHelper {
public:
    static GrokGodModeHelper& getInstance() {
        static GrokGodModeHelper instance;
        return instance;
    }

    void setEngine(Engine* e) { engine = e; }
    Engine* getEngine() const { return engine.get(); }

    void setAnalysisService(AudioAnalysisService* s) { analysisService = s; }
    AudioAnalysisService* getAnalysisService() const { return analysisService.get(); }

    void setBrowserModel(BrowserModel* m) { browserModel = m; }
    BrowserModel* getBrowserModel() const { return browserModel.get(); }

private:
    GrokGodModeHelper() = default;
    
    juce::WeakReference<Engine> engine;
    juce::WeakReference<BrowserModel> browserModel;
    juce::WeakReference<AudioAnalysisService> analysisService;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokGodModeHelper)
};

} // namespace zenith