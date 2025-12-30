/*
  ==============================================================================

    GrokGodModeHelper.h
    Created: 2025-12-27
    Author:  Zenith DAW

    Central registry for God Mode services.
    Uses WeakReferences to ensure AI background threads never access deleted
    DAW components.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "../engine/Engine.h"
#include "../browser/BrowserModel.h"
#include "../network/AudioAnalysisService.h"

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
    AudioAnalysisService* getAnalysisService() const { return analysisService; }

    void setBrowserModel(BrowserModel* m) { browserModel = m; }
    BrowserModel* getBrowserModel() const { return browserModel.get(); }

private:
    GrokGodModeHelper() = default;
    
    juce::WeakReference<Engine> engine;
    juce::WeakReference<BrowserModel> browserModel;
    
    // AudioAnalysisService is usually owned by Engine, so we keep a raw pointer
    // but check engine validity first in bindings.
    AudioAnalysisService* analysisService = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokGodModeHelper)
};

} // namespace zenith