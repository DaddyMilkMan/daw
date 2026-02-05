/*
  ==============================================================================

    ViewSwitcher.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Navigation controller for Tab/Shift+Tab view switching.
    Manages Arrangement, Session, and AI Jam views.
    
    Now wired to Engine and ProjectState for real data.

  ==============================================================================
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../framework/Animation.h"
#include "../controllers/ArrangementController.h"
#include "../controllers/SessionController.h"
#include <memory>
#include <functional>

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
class SkiaAIJamView;

/**
 * @brief Active view type enumeration
 */
enum class ViewType {
    Arrangement,  ///< Timeline-based arrangement (Logic/Pro Tools style)
    Session,      ///< Clip launcher grid (Ableton style)
    AIJam         ///< AI-powered jamming overlay (unique to Zenith)
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
 * - Shift+Tab: Toggle AI Jam overlay
 * - Cmd/Ctrl+1/2/3: Direct view access
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
     * @brief Get previous main view (for returning from AI Jam)
     */
    ViewType getPreviousMainView() const { return previousMainView_; }
    
    /**
     * @brief Check if AI Jam overlay is visible
     */
    bool isAIJamVisible() const { return currentView_ == ViewType::AIJam; }
    
    /**
     * @brief Check if transition is in progress
     */
    bool isTransitioning() const { return transition_.isActive; }
    
    //==========================================================================
    // View Access
    //==========================================================================
    
    SkiaArrangementView* getArrangementView() { return arrangementView_.get(); }
    SkiaSessionView* getSessionView() { return sessionView_.get(); }
    SkiaAIJamView* getAIJamView() { return aiJamView_.get(); }
    
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
    ViewType previousMainView_ = ViewType::Arrangement;  // For returning from AI Jam
    ViewTransition transition_;
    
    //==========================================================================
    // Child Views
    //==========================================================================
    
    std::unique_ptr<SkiaArrangementView> arrangementView_;
    std::unique_ptr<SkiaSessionView> sessionView_;
    std::unique_ptr<SkiaAIJamView> aiJamView_;
    
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
    void drawAIJamOverlay(SkCanvas* canvas, float alpha);
    
    bool handleTabKey(bool shiftHeld);
    bool handleNumberKey(int number);
    
    static constexpr float kTransitionDuration = 0.25f;  // seconds

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewSwitcher)
};

} // namespace zenith::ui
