/*
  ==============================================================================

    ZenithMainLayout.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Main layout container that assembles all UI components.
    This is the root component for the new Zenith UI.

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "core/ViewSwitcher.h"
#include "common/SkiaTransportBar.h"
#include <memory>

namespace zenith::ui {

/**
 * @class ZenithMainLayout
 * @brief Root layout component for Zenith DAW
 * 
 * Layout structure:
 * ┌─────────────────────────────────────────┐
 * │           Transport Bar (48px)          │
 * ├─────────────────────────────────────────┤
 * │                                         │
 * │         ViewSwitcher (fills)            │
 * │   (Arrangement/Session/AI Jam views)    │
 * │                                         │
 * ├─────────────────────────────────────────┤
 * │          Status Bar (24px)              │
 * └─────────────────────────────────────────┘
 */
class ZenithMainLayout : public SkiaComponent,
                         public ViewSwitcher::Listener {
public:
    ZenithMainLayout();
    ~ZenithMainLayout() override;

    //==========================================================================
    // Access to Sub-Components
    //==========================================================================
    
    SkiaTransportBar* getTransportBar() { return transportBar_.get(); }
    ViewSwitcher* getViewSwitcher() { return viewSwitcher_.get(); }
    
    //==========================================================================
    // Transport Integration
    //==========================================================================
    
    void setPlayState(TransportState state);
    void setPosition(double beats);
    void setTempo(float bpm);
    void setLoopEnabled(bool enabled);
    
    //==========================================================================
    // View Control
    //==========================================================================
    
    void setActiveView(ViewType view);
    ViewType getActiveView() const;
    
    //==========================================================================
    // Status Bar
    //==========================================================================
    
    void setCPULoad(float load);
    void setMIDIActivity(bool active);
    void setAudioLatency(float ms);
    void setZoomLevel(float zoom);
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
    //==========================================================================
    // ViewSwitcher::Listener
    //==========================================================================
    
    void viewDidChange(ViewType newView) override;

private:
    //==========================================================================
    // Layout Constants
    //==========================================================================
    
    static constexpr float kTransportHeight = 48.0f;
    static constexpr float kStatusBarHeight = 24.0f;
    
    //==========================================================================
    // Components
    //==========================================================================
    
    std::unique_ptr<SkiaTransportBar> transportBar_;
    std::unique_ptr<ViewSwitcher> viewSwitcher_;
    
    //==========================================================================
    // Status Bar State
    //==========================================================================
    
    float cpuLoad_ = 0.0f;
    bool midiActive_ = false;
    float audioLatency_ = 0.0f;
    float zoomLevel_ = 1.0f;
    
    //==========================================================================
    // Drawing
    //==========================================================================
    
    void drawStatusBar(SkCanvas* canvas);
    
    //==========================================================================
    // Internal
    //==========================================================================
    
    void setupCallbacks();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithMainLayout)
};

} // namespace zenith::ui
