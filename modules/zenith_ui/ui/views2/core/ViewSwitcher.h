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

#include "../../framework/SkiaComponent.h"
#include "../../framework/Animation.h"
#include "../controllers/ArrangementController.h"
#include "../controllers/SessionController.h"
#include <memory>
#include <functional>

// Include for std::unique_ptr member (must be outside namespace)
#include "../ai-jam/SkiaAIJamView.h"

// Forward declarations
namespace zenith {
    class Engine;
    class ProjectState;
}

namespace zenith::ui {

// Use base SkiaComponent from zenith namespace
using SkiaComponent = zenith::SkiaComponent;

// Forward declarations
class SkiaArrangementView;
class SkiaSessionView;
class SkiaProjectManager;

/**
 * @brief Active view type enumeration
 */
enum class ViewType {
    Arrangement,  ///< Timeline-based arrangement (Logic/Pro Tools style)
    Session,      ///< Clip launcher grid (Ableton style)
    ProjectManager, ///< Project management view
    AIJam         ///< AI Jam overlay view
};

/**
 * @brief View transition animation state
 */
struct ViewTransition {
    ViewType from;
    ViewType to;
    float progress = 0.0f;      ///< 0.0 = start, 1.0 = complete
    float duration = 0.25f;     ///< seconds
    bool isActive = false;
    
    float getEasedProgress() const {
        // Ease-out cubic
        float t = progress;
        return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    }
};

/**
 * @class ViewSwitcher
 * @brief Central controller for view navigation
 *
 * Handles:
 * - Tab key: Toggle Arrangement ↔ Session
 * - Cmd/Ctrl+1/2: Direct view access
 * - Smooth crossfade transitions
 * - Engine/ProjectState wiring via controllers
 */
class ViewSwitcher : public SkiaComponent {
public:
    /**
     * @brief Default constructor (standalone mode)
     */
    ViewSwitcher();

    /**
     * @brief Construct with engine dependencies
     * @param engine Reference to the audio engine
     * @param projectState Reference to the project state
     */
    ViewSwitcher(zenith::Engine& engine, zenith::ProjectState& projectState);
    ~ViewSwitcher() override;

    //==========================================================================
    // View Management
    //==========================================================================

    /**
     * @brief Set the active view with optional animation
     * @param view Target view type
     * @param animate Whether to animate the transition
     */
    void setActiveView(ViewType view, bool animate = true);

    /**
     * @brief Get currently active view
     */
    ViewType getActiveView() const { return currentView_; }

    /**
     * @brief Check if transition is in progress
     */
    bool isTransitioning() const { return transition_.isActive; }

    //==========================================================================
    // View Access
    //==========================================================================

    SkiaArrangementView* getArrangementView() { return arrangementView_.get(); }
    SkiaSessionView* getSessionView() { return sessionView_.get(); }
    SkiaProjectManager* getProjectManagerView() { return projectManagerView_.get(); }

    ArrangementController* getArrangementController() { return arrangementController_.get(); }
    SessionController* getSessionController() { return sessionController_.get(); }

    //==========================================================================
    // Listener Interface
    //==========================================================================
    
    struct Listener {
        virtual ~Listener() = default;
        virtual void viewWillChange(ViewType from, ViewType to) {}
        virtual void viewDidChange(ViewType newView) {}
        virtual void transitionProgress(ViewType from, ViewType to, float progress) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
    //==========================================================================
    // Animation
    //==========================================================================
    
    void onAnimationTick(float deltaMs) override;

private:
    //==========================================================================
    // State
    //==========================================================================

    ViewType currentView_ = ViewType::Arrangement;
    ViewTransition transition_;

    //==========================================================================
    // Child Views
    //==========================================================================

    std::unique_ptr<SkiaArrangementView> arrangementView_;
    std::unique_ptr<SkiaSessionView> sessionView_;
    std::unique_ptr<SkiaProjectManager> projectManagerView_;

    //==========================================================================
    // Controllers (bridge views to engine)
    //==========================================================================

    std::unique_ptr<ArrangementController> arrangementController_;
    std::unique_ptr<SessionController> sessionController_;
    
    //==========================================================================
    // Engine References (nullable for standalone mode)
    //==========================================================================
    
    zenith::Engine* engine_ = nullptr;
    zenith::ProjectState* projectState_ = nullptr;
    
    //==========================================================================
    // Listeners
    //==========================================================================
    
    juce::ListenerList<Listener> listeners_;
    
    //==========================================================================
    // Internal Methods
    //==========================================================================
    
    void startTransition(ViewType to);
    void updateTransition(float deltaMs);
    void completeTransition();
    
    void drawArrangementView(SkCanvas* canvas, float alpha);
    void drawSessionView(SkCanvas* canvas, float alpha);
    void drawProjectManagerView(SkCanvas* canvas, float alpha);
    void drawView(ViewType view, SkCanvas* canvas, float alpha);
    
    bool handleTabKey(bool shiftHeld);
    bool handleNumberKey(int number);
    
    static constexpr float kTransitionDuration = 0.25f;  // seconds

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewSwitcher)
};

} // namespace zenith::ui
