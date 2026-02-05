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

namespace zenith {
    class Engine;
    class ProjectState;
    class CommandAPI;
}

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
    /**
     * @brief Construct with engine connection (real data mode)
     * @param engine Reference to the audio engine
     * @param projectState Reference to the project state
     * @param commandAPI Reference to the command API (optional)
     */
    ZenithMainLayout(zenith::Engine& engine, zenith::ProjectState& projectState, 
                     zenith::CommandAPI* commandAPI = nullptr);

    /**
     * @brief Default constructor (standalone/demo mode - for testing only)
     */
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
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    
    //==========================================================================
    // Callbacks (wired to MainComponent)
    //==========================================================================
    
    std::function<void()> onPlayRequest;
    std::function<void()> onStopRequest;
    std::function<void()> onRecordRequest;
    std::function<void()> onRewindRequest;
    std::function<void()> onLoopToggleRequest;
    std::function<void()> onMetronomeToggleRequest;
    std::function<void(double)> onTempoChangeRequest;
    std::function<void(int, int)> onTimeSignatureChangeRequest;
    std::function<void()> onSettingsRequest;
    std::function<void()> onWingmanToggleRequest;
    
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
    // Engine References (null in standalone mode)
    //==========================================================================

    zenith::Engine* engine_ = nullptr;
    zenith::ProjectState* projectState_ = nullptr;
    zenith::CommandAPI* commandAPI_ = nullptr;

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
    void setupEngineCallbacks();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithMainLayout)
};

} // namespace zenith::ui
