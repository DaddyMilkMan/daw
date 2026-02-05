/*
  ==============================================================================

    ViewSwitcher.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of ViewSwitcher navigation controller.

  ==============================================================================
*/

#include "ViewSwitcher.h"
#include "../arranger/SkiaArrangementView.h"
#include "../session/SkiaSessionView.h"
#include "../ai-jam/SkiaAIJamView.h"
#include "../controllers/ArrangementController.h"
#include "../controllers/SessionController.h"
#include "../../design-system/ZenithTheme.h"
#include "../../../engine/Engine.h"
#include "../../../engine/ProjectState.h"

namespace zenith::ui {

//==============================================================================
// Construction/Destruction
//==============================================================================

ViewSwitcher::ViewSwitcher()
    : engine_(nullptr)
    , projectState_(nullptr)
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    
    // Create child views (standalone mode, no engine connection)
    arrangementView_ = std::make_unique<SkiaArrangementView>();
    sessionView_ = std::make_unique<SkiaSessionView>();
    aiJamView_ = std::make_unique<SkiaAIJamView>();
    
    // Add as children but manage visibility
    addAndMakeVisible(arrangementView_.get());
    addChildComponent(sessionView_.get());  // Hidden initially
    addChildComponent(aiJamView_.get());    // Hidden initially
    
    // Start animation coordinator
    animation::AnimationCoordinator::getInstance().registerListener(this);
}

ViewSwitcher::ViewSwitcher(zenith::Engine& engine, zenith::ProjectState& projectState)
    : engine_(&engine)
    , projectState_(&projectState)
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    
    // Create child views
    arrangementView_ = std::make_unique<SkiaArrangementView>();
    sessionView_ = std::make_unique<SkiaSessionView>();
    aiJamView_ = std::make_unique<SkiaAIJamView>();
    
    // Create controllers that wire views to engine
    if (engine_ && projectState_) {
        arrangementController_ = std::make_unique<ArrangementController>(
            *arrangementView_, *engine_, *projectState_);
        sessionController_ = std::make_unique<SessionController>(
            *sessionView_, *engine_, *projectState_);
    }
    
    // Add as children but manage visibility
    addAndMakeVisible(arrangementView_.get());
    addChildComponent(sessionView_.get());  // Hidden initially
    addChildComponent(aiJamView_.get());    // Hidden initially
    
    // Start animation coordinator
    animation::AnimationCoordinator::getInstance().registerListener(this);
}

ViewSwitcher::~ViewSwitcher() {
    animation::AnimationCoordinator::getInstance().unregisterListener(this);
    removeKeyListener(this);
}

//==============================================================================
// View Management
//==============================================================================

void ViewSwitcher::setActiveView(ViewType view, bool animate) {
    if (view == currentView_ && !transition_.isActive) {
        return;  // Already on this view
    }
    
    if (animate) {
        startTransition(view);
    } else {
        // Immediate switch
        ViewType oldView = currentView_;
        currentView_ = view;
        
        // Update visibility
        arrangementView_->setVisible(view == ViewType::Arrangement);
        sessionView_->setVisible(view == ViewType::Session);
        aiJamView_->setVisible(view == ViewType::AIJam);
        
        // Notify listeners
        listeners_.call([oldView, view](Listener& l) {
            l.viewWillChange(oldView, view);
            l.viewDidChange(view);
        });
        
        markDirty();
    }
}

void ViewSwitcher::startTransition(ViewType to) {
    if (transition_.isActive) {
        // Complete current transition first
        completeTransition();
    }
    
    // Notify will change
    listeners_.call([this, to](Listener& l) {
        l.viewWillChange(currentView_, to);
    });
    
    // Store previous main view for AI Jam return
    if (to == ViewType::AIJam && currentView_ != ViewType::AIJam) {
        previousMainView_ = currentView_;
    }
    
    // Start new transition
    transition_.from = currentView_;
    transition_.to = to;
    transition_.progress = 0.0f;
    transition_.duration = kTransitionDuration;
    transition_.isActive = true;
    
    // Make target view visible for crossfade
    if (to == ViewType::Arrangement) arrangementView_->setVisible(true);
    if (to == ViewType::Session) sessionView_->setVisible(true);
    if (to == ViewType::AIJam) aiJamView_->setVisible(true);
    
    markDirty();
}

void ViewSwitcher::updateTransition(float deltaMs) {
    if (!transition_.isActive) return;
    
    float deltaSec = deltaMs / 1000.0f;
    transition_.progress += deltaSec / transition_.duration;
    
    if (transition_.progress >= 1.0f) {
        completeTransition();
    } else {
        // Notify progress
        listeners_.call([this](Listener& l) {
            l.transitionProgress(transition_.from, transition_.to, 
                                 transition_.getEasedProgress());
        });
        markDirty();
    }
}

void ViewSwitcher::completeTransition() {
    if (!transition_.isActive) return;
    
    transition_.progress = 1.0f;
    transition_.isActive = false;
    
    ViewType oldView = currentView_;
    currentView_ = transition_.to;
    
    // Update visibility - hide old views
    if (currentView_ == ViewType::AIJam) {
        // AI Jam is overlay, keep underlying view visible but dimmed
        // (handled in draw)
    } else {
        arrangementView_->setVisible(currentView_ == ViewType::Arrangement);
        sessionView_->setVisible(currentView_ == ViewType::Session);
        aiJamView_->setVisible(false);
    }
    
    // Notify did change
    listeners_.call([this](Listener& l) {
        l.viewDidChange(currentView_);
    });
    
    markDirty();
}

//==============================================================================
// Listeners
//==============================================================================

void ViewSwitcher::addListener(Listener* listener) {
    listeners_.add(listener);
}

void ViewSwitcher::removeListener(Listener* listener) {
    listeners_.remove(listener);
}

//==============================================================================
// Drawing
//==============================================================================

void ViewSwitcher::drawSkia(SkCanvas* canvas) {
    SkAutoCanvasRestore acr(canvas, true);
    
    if (transition_.isActive) {
        float t = transition_.getEasedProgress();
        
        // Draw source view fading out
        float fromAlpha = 1.0f - t;
        // Draw target view fading in
        float toAlpha = t;
        
        // Special case: AI Jam is an overlay
        if (transition_.to == ViewType::AIJam) {
            // Draw underlying view at full opacity
            if (transition_.from == ViewType::Arrangement) {
                drawArrangementView(canvas, 1.0f);
            } else {
                drawSessionView(canvas, 1.0f);
            }
            // Draw AI Jam overlay fading in
            drawAIJamOverlay(canvas, toAlpha);
        }
        else if (transition_.from == ViewType::AIJam) {
            // Fading out of AI Jam
            if (transition_.to == ViewType::Arrangement) {
                drawArrangementView(canvas, 1.0f);
            } else {
                drawSessionView(canvas, 1.0f);
            }
            drawAIJamOverlay(canvas, fromAlpha);
        }
        else {
            // Normal crossfade between Arrangement and Session
            if (transition_.from == ViewType::Arrangement) {
                drawArrangementView(canvas, fromAlpha);
                drawSessionView(canvas, toAlpha);
            } else {
                drawSessionView(canvas, fromAlpha);
                drawArrangementView(canvas, toAlpha);
            }
        }
    }
    else {
        // No transition - draw current view
        switch (currentView_) {
            case ViewType::Arrangement:
                drawArrangementView(canvas, 1.0f);
                break;
            case ViewType::Session:
                drawSessionView(canvas, 1.0f);
                break;
            case ViewType::AIJam:
                // Draw underlying view + overlay
                if (previousMainView_ == ViewType::Arrangement) {
                    drawArrangementView(canvas, 1.0f);
                } else {
                    drawSessionView(canvas, 1.0f);
                }
                drawAIJamOverlay(canvas, 1.0f);
                break;
        }
    }
}

void ViewSwitcher::drawArrangementView(SkCanvas* canvas, float alpha) {
    if (alpha <= 0.0f) return;
    
    if (alpha < 1.0f) {
        canvas->saveLayerAlpha(nullptr, static_cast<int>(alpha * 255));
        arrangementView_->drawSkia(canvas);
        canvas->restore();
    } else {
        arrangementView_->drawSkia(canvas);
    }
}

void ViewSwitcher::drawSessionView(SkCanvas* canvas, float alpha) {
    if (alpha <= 0.0f) return;
    
    if (alpha < 1.0f) {
        canvas->saveLayerAlpha(nullptr, static_cast<int>(alpha * 255));
        sessionView_->drawSkia(canvas);
        canvas->restore();
    } else {
        sessionView_->drawSkia(canvas);
    }
}

void ViewSwitcher::drawAIJamOverlay(SkCanvas* canvas, float alpha) {
    if (alpha <= 0.0f) return;
    
    // Dim the background
    SkPaint dimPaint;
    dimPaint.setColor(SkColorSetARGB(static_cast<int>(alpha * 180), 0, 0, 0));
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), dimPaint);
    
    // Draw AI Jam panel
    if (alpha < 1.0f) {
        canvas->saveLayerAlpha(nullptr, static_cast<int>(alpha * 255));
        aiJamView_->drawSkia(canvas);
        canvas->restore();
    } else {
        aiJamView_->drawSkia(canvas);
    }
}

//==============================================================================
// Layout
//==============================================================================

void ViewSwitcher::resized() {
    auto bounds = getLocalBounds();
    
    // All views fill the entire area
    arrangementView_->setBounds(bounds);
    sessionView_->setBounds(bounds);
    
    // AI Jam has padding (it's a centered overlay)
    int padding = 40;
    aiJamView_->setBounds(bounds.reduced(padding));
}

//==============================================================================
// Keyboard Handling
//==============================================================================

bool ViewSwitcher::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    juce::ignoreUnused(origin);
    
    // Tab / Shift+Tab navigation
    if (key.getKeyCode() == juce::KeyPress::tabKey) {
        return handleTabKey(key.getModifiers().isShiftDown());
    }
    
    // Cmd/Ctrl + 1/2/3 for direct view access
    if (key.getModifiers().isCommandDown()) {
        int keyChar = key.getKeyCode();
        if (keyChar == '1') return handleNumberKey(1);
        if (keyChar == '2') return handleNumberKey(2);
        if (keyChar == '3') return handleNumberKey(3);
    }
    
    return false;
}

bool ViewSwitcher::handleTabKey(bool shiftHeld) {
    if (shiftHeld) {
        // Shift+Tab: Toggle AI Jam overlay
        if (currentView_ == ViewType::AIJam) {
            // Exit AI Jam, return to previous view
            setActiveView(previousMainView_, true);
        } else {
            // Enter AI Jam
            setActiveView(ViewType::AIJam, true);
        }
    } else {
        // Tab: Toggle between Arrangement and Session
        // (Does nothing if in AI Jam - user must exit first)
        if (currentView_ == ViewType::Arrangement) {
            setActiveView(ViewType::Session, true);
        } else if (currentView_ == ViewType::Session) {
            setActiveView(ViewType::Arrangement, true);
        }
    }
    return true;
}

bool ViewSwitcher::handleNumberKey(int number) {
    switch (number) {
        case 1: setActiveView(ViewType::Arrangement, true); return true;
        case 2: setActiveView(ViewType::Session, true); return true;
        case 3: setActiveView(ViewType::AIJam, true); return true;
        default: return false;
    }
}

//==============================================================================
// Animation
//==============================================================================

void ViewSwitcher::onAnimationTick(float deltaMs) {
    if (transition_.isActive) {
        updateTransition(deltaMs);
    }
    
    // Forward to child views
    if (arrangementView_) arrangementView_->onAnimationTick(deltaMs);
    if (sessionView_) sessionView_->onAnimationTick(deltaMs);
    if (aiJamView_) aiJamView_->onAnimationTick(deltaMs);
}

} // namespace zenith::ui
