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

#include "ViewSwitcher.h"
#include "../arranger/SkiaArrangementView.h"
#include "../session/SkiaSessionView.h"
#include "../project/SkiaProjectManager.h"
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
    projectManagerView_.reset();

    // Add as children but manage visibility
    addAndMakeVisible(arrangementView_.get());
    addChildComponent(sessionView_.get());  // Hidden initially
    // Project manager requires engine/project state; not available in standalone

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
    projectManagerView_ = std::make_unique<SkiaProjectManager>(*engine_, *projectState_);

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
    addChildComponent(projectManagerView_.get());  // Hidden initially

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

    if (view == ViewType::ProjectManager && projectManagerView_ == nullptr) {
        return;  // Not available in standalone mode
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
        if (projectManagerView_) {
            projectManagerView_->setVisible(view == ViewType::ProjectManager);
        }

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

    if (to == ViewType::ProjectManager && projectManagerView_ == nullptr) {
        return;
    }

    // Notify will change
    listeners_.call([this, to](Listener& l) {
        l.viewWillChange(currentView_, to);
    });

    // Start new transition
    transition_.from = currentView_;
    transition_.to = to;
    transition_.progress = 0.0f;
    transition_.duration = kTransitionDuration;
    transition_.isActive = true;

    // Make target view visible for crossfade
    if (to == ViewType::Arrangement) arrangementView_->setVisible(true);
    if (to == ViewType::Session) sessionView_->setVisible(true);
    if (to == ViewType::ProjectManager && projectManagerView_) projectManagerView_->setVisible(true);

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
    arrangementView_->setVisible(currentView_ == ViewType::Arrangement);
    sessionView_->setVisible(currentView_ == ViewType::Session);
    if (projectManagerView_) {
        projectManagerView_->setVisible(currentView_ == ViewType::ProjectManager);
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

        drawView(transition_.from, canvas, fromAlpha);
        drawView(transition_.to, canvas, toAlpha);
    }
    else {
        // No transition - draw current view
        drawView(currentView_, canvas, 1.0f);
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

void ViewSwitcher::drawProjectManagerView(SkCanvas* canvas, float alpha) {
    if (alpha <= 0.0f || projectManagerView_ == nullptr) return;

    if (alpha < 1.0f) {
        canvas->saveLayerAlpha(nullptr, static_cast<int>(alpha * 255));
        projectManagerView_->drawSkia(canvas);
        canvas->restore();
    } else {
        projectManagerView_->drawSkia(canvas);
    }
}

void ViewSwitcher::drawView(ViewType view, SkCanvas* canvas, float alpha) {
    switch (view) {
        case ViewType::Arrangement:
            drawArrangementView(canvas, alpha);
            break;
        case ViewType::Session:
            drawSessionView(canvas, alpha);
            break;
        case ViewType::ProjectManager:
            drawProjectManagerView(canvas, alpha);
            break;
        case ViewType::AIJam:
            // Not implemented yet
            break;
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
    if (projectManagerView_) {
        projectManagerView_->setBounds(bounds);
    }
}

//==============================================================================
// Keyboard Handling
//==============================================================================

bool ViewSwitcher::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    juce::ignoreUnused(origin);

    // Tab navigation
    if (key.getKeyCode() == juce::KeyPress::tabKey) {
        return handleTabKey(key.getModifiers().isShiftDown());
    }

    // Cmd/Ctrl + 1/2 for direct view access
    if (key.getModifiers().isCommandDown()) {
        int keyChar = key.getKeyCode();
        if (keyChar == '1') return handleNumberKey(1);
        if (keyChar == '2') return handleNumberKey(2);
        if (keyChar == '3') return handleNumberKey(3);
    }

    return false;
}

bool ViewSwitcher::handleTabKey(bool shiftHeld) {
    juce::ignoreUnused(shiftHeld);

    // Tab: Toggle between Arrangement and Session
    if (currentView_ == ViewType::Arrangement) {
        setActiveView(ViewType::Session, true);
    } else {
        setActiveView(ViewType::Arrangement, true);
    }
    return true;
}

bool ViewSwitcher::handleNumberKey(int number) {
    switch (number) {
        case 1: setActiveView(ViewType::Arrangement, true); return true;
        case 2: setActiveView(ViewType::Session, true); return true;
        case 3:
            if (projectManagerView_) {
                setActiveView(ViewType::ProjectManager, true);
                return true;
            }
            return false;
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
    if (projectManagerView_) projectManagerView_->onAnimationTick(deltaMs);
}

} // namespace zenith::ui
