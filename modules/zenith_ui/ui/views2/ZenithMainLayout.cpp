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

    ZenithMainLayout.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of main layout container.

  ==============================================================================

*/

#include "ZenithMainLayout.h"
#include "../design-system/ZenithTheme.h"
#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"
#include "../../commands/CommandAPI.h"

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

ZenithMainLayout::ZenithMainLayout(zenith::Engine& engine, zenith::ProjectState& projectState,
                                   zenith::CommandAPI* commandAPI)
    : engine_(&engine), projectState_(&projectState), commandAPI_(commandAPI) {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Create transport bar
    transportBar_ = std::make_unique<SkiaTransportBar>();
    addAndMakeVisible(transportBar_.get());

    // Create view switcher WITH engine connection (real data mode)
    viewSwitcher_ = std::make_unique<ViewSwitcher>(engine, projectState);
    viewSwitcher_->addListener(this);
    addAndMakeVisible(viewSwitcher_.get());

    // Wire up callbacks
    setupCallbacks();
    setupEngineCallbacks();
}


ZenithMainLayout::~ZenithMainLayout() {
    if (viewSwitcher_) {
        viewSwitcher_->removeListener(this);
    }
    removeKeyListener(this);
}

//==============================================================================
// Callbacks Setup
//==============================================================================

void ZenithMainLayout::setupCallbacks() {
    // Transport -> View Switcher sync
    transportBar_->onViewChange = [this](SkiaTransportBar::ActiveView view) {
        switch (view) {
            case SkiaTransportBar::ActiveView::Arrangement:
                viewSwitcher_->setActiveView(ViewType::Arrangement);
                break;
            case SkiaTransportBar::ActiveView::Session:
                viewSwitcher_->setActiveView(ViewType::Session);
                break;
        }
    };

    // Default placeholder transport actions (for demo mode)
    // In engine mode, these are overridden by setupEngineCallbacks()
    transportBar_->onPlay = [this]() {
        if (engine_) {
            engine_->play();
        }
    };

    transportBar_->onStop = [this]() {
        if (engine_) {
            engine_->stop();
        }
    };

    transportBar_->onRecord = [this]() {
        if (engine_) {
            if (engine_->isRecording()) {
                engine_->stopRecording();
            } else {
                engine_->record();
            }
        }
    };

    transportBar_->onLoopToggle = [this]() {
        if (engine_) {
            engine_->setLooping(!engine_->isLooping());
        }
        transportBar_->setLoopEnabled(!transportBar_->isLoopEnabled());
    };

    transportBar_->onMetronomeToggle = [this]() {
        if (engine_) {
            engine_->toggleMetronome();
        }
        transportBar_->setMetronomeEnabled(!transportBar_->isMetronomeEnabled());
    };
}

void ZenithMainLayout::setupEngineCallbacks() {
    if (!engine_ || !projectState_) {
        return;  // Should not happen with single constructor
    }

    // Wire tempo changes to project state
    transportBar_->onTempoChange = [this](float bpm) {
        if (projectState_) {
            projectState_->setTempo(bpm);
        }
    };

    // Wire time signature changes to project state
    transportBar_->onTimeSignatureChange = [this](int numerator, int denominator) {
        if (projectState_) {
            projectState_->setTimeSignature(numerator, denominator);
        }
    };

    // Set initial tempo from project state
    transportBar_->setTempo(projectState_->getTempo());

    // Set initial time signature from project state
    transportBar_->setTimeSignature(
        projectState_->getTimeSignatureNumerator(),
        projectState_->getTimeSignatureDenominator()
    );
}

//==============================================================================
// Transport Integration
//==============================================================================

void ZenithMainLayout::setPlayState(TransportState state) {
    transportBar_->setState(state);
}

void ZenithMainLayout::setPosition(double beats) {
    transportBar_->setPosition(beats);
    
    // Update arrangement view playhead
    if (auto* arr = viewSwitcher_->getArrangementView()) {
        arr->setPlayheadPosition(beats);
    }
}

void ZenithMainLayout::setTempo(float bpm) {
    transportBar_->setTempo(bpm);
}

void ZenithMainLayout::setLoopEnabled(bool enabled) {
    transportBar_->setLoopEnabled(enabled);
}

//==============================================================================
// View Control
//==============================================================================

void ZenithMainLayout::setActiveView(ViewType view) {
    viewSwitcher_->setActiveView(view);
}

ViewType ZenithMainLayout::getActiveView() const {
    return viewSwitcher_->getActiveView();
}

//==============================================================================
// Status Bar
//==============================================================================

void ZenithMainLayout::setCPULoad(float load) {
    cpuLoad_ = load;
    markDirty();
}

void ZenithMainLayout::setMIDIActivity(bool active) {
    midiActive_ = active;
    markDirty();
}

void ZenithMainLayout::setAudioLatency(float ms) {
    audioLatency_ = ms;
    markDirty();
}

void ZenithMainLayout::setZoomLevel(float zoom) {
    zoomLevel_ = zoom;
    markDirty();
}

void ZenithMainLayout::zoomIn() {
    if (!viewSwitcher_ || viewSwitcher_->getActiveView() != ViewType::Arrangement) {
        return;
    }
    if (auto* arranger = viewSwitcher_->getArrangementView()) {
        auto zoom = arranger->getZoom();
        arranger->setZoom(zoom.x * 1.1f, zoom.y);
        setZoomLevel(arranger->getZoom().x);
    }
}

void ZenithMainLayout::zoomOut() {
    if (!viewSwitcher_ || viewSwitcher_->getActiveView() != ViewType::Arrangement) {
        return;
    }
    if (auto* arranger = viewSwitcher_->getArrangementView()) {
        auto zoom = arranger->getZoom();
        arranger->setZoom(zoom.x / 1.1f, zoom.y);
        setZoomLevel(arranger->getZoom().x);
    }
}

void ZenithMainLayout::zoomToFit() {
    if (!viewSwitcher_ || viewSwitcher_->getActiveView() != ViewType::Arrangement) {
        return;
    }
    if (auto* arranger = viewSwitcher_->getArrangementView()) {
        arranger->setZoom(1.0f, 1.0f);
        setZoomLevel(arranger->getZoom().x);
    }
}

//==============================================================================
// ViewSwitcher::Listener
//==============================================================================

void ZenithMainLayout::viewDidChange(ViewType newView) {
    // Sync transport bar view toggle
    switch (newView) {
        case ViewType::Arrangement:
            transportBar_->setActiveView(SkiaTransportBar::ActiveView::Arrangement);
            if (auto* arranger = viewSwitcher_->getArrangementView()) {
                setZoomLevel(arranger->getZoom().x);
            }
            break;
        case ViewType::Session:
            transportBar_->setActiveView(SkiaTransportBar::ActiveView::Session);
            break;
        case ViewType::ProjectManager:
            // Keep transport bar visible; no specific transport state for this view.
            break;
        case ViewType::AIJam:
            break;
    }
}

//==============================================================================
// Layout
//==============================================================================

void ZenithMainLayout::resized() {
    auto bounds = getLocalBounds();
    
    // Transport bar at top
    transportBar_->setBounds(0, 0, bounds.getWidth(), static_cast<int>(kTransportHeight));
    
    // View switcher fills middle
    int viewTop = static_cast<int>(kTransportHeight);
    int viewHeight = bounds.getHeight() - static_cast<int>(kTransportHeight + kStatusBarHeight);
    viewSwitcher_->setBounds(0, viewTop, bounds.getWidth(), viewHeight);
    
    // Status bar is drawn in drawSkia, no separate component
}

//==============================================================================
// Drawing
//==============================================================================

void ZenithMainLayout::drawSkia(SkCanvas* canvas) {
    SkAutoCanvasRestore acr(canvas, true);
    
    // Background (in case there are gaps)
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
    
    // Transport bar draws itself
    canvas->save();
    transportBar_->drawSkia(canvas);
    canvas->restore();
    
    // View switcher draws itself
    canvas->save();
    canvas->translate(0, kTransportHeight);
    viewSwitcher_->drawSkia(canvas);
    canvas->restore();
    
    // Status bar at bottom
    drawStatusBar(canvas);
}

void ZenithMainLayout::drawStatusBar(SkCanvas* canvas) {
    float y = getHeight() - kStatusBarHeight;

    if (viewSwitcher_ && viewSwitcher_->getActiveView() == ViewType::Arrangement) {
        if (auto* arranger = viewSwitcher_->getArrangementView()) {
            zoomLevel_ = arranger->getZoom().x;
        }
    }
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_01.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(0, y, getWidth(), kStatusBarHeight), bgPaint);
    
    // Top border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
    canvas->drawLine(0, y, getWidth(), y, borderPaint);
    
    SkPaint textPaint;
    textPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    textPaint.setAntiAlias(true);
    SkFont font = design::typography::getSkFont(10.0f);
    
    float textY = y + 16;
    float x = 16;
    
    // CPU
    juce::String cpuStr = "CPU: " + juce::String(static_cast<int>(cpuLoad_ * 100)) + "%";
    canvas->drawString(cpuStr.toRawUTF8(), x, textY, font, textPaint);
    x += 80;
    
    // MIDI indicator
    SkPaint midiPaint;
    midiPaint.setColor(midiActive_ ? 
        ZenithTheme::Colors::success.getARGB() : 
        ZenithTheme::Colors::text_tertiary.getARGB());
    midiPaint.setAntiAlias(true);
    canvas->drawCircle(x + 4, textY - 4, 4, midiPaint);
    
    textPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    canvas->drawString("MIDI", x + 14, textY, font, textPaint);
    x += 60;
    
    // Latency
    juce::String latStr = juce::String(audioLatency_, 1) + "ms";
    canvas->drawString(latStr.toRawUTF8(), x, textY, font, textPaint);
    x += 60;
    
    // Right side: Zoom level
    float rightX = getWidth() - 100;
    juce::String zoomStr = juce::String(static_cast<int>(zoomLevel_ * 100)) + "%";
    canvas->drawString(zoomStr.toRawUTF8(), rightX, textY, font, textPaint);
    
    // Position indicator
    rightX = getWidth() - 40;
    canvas->drawString("1:1", rightX, textY, font, textPaint);
}

//==============================================================================
// Keyboard
//==============================================================================

bool ZenithMainLayout::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    // Global shortcuts
    
    // Space - Play/Stop
    if (key == juce::KeyPress::spaceKey) {
        if (transportBar_->getState() == TransportState::Playing) {
            transportBar_->setState(TransportState::Stopped);
        } else {
            transportBar_->setState(TransportState::Playing);
        }
        return true;
    }
    
    // R - Toggle Record
    if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R') {
        transportBar_->setRecordArmed(!transportBar_->isRecordArmed());
        return true;
    }
    
    // L - Toggle Loop
    if (key.getTextCharacter() == 'l' || key.getTextCharacter() == 'L') {
        transportBar_->setLoopEnabled(!transportBar_->isLoopEnabled());
        return true;
    }
    
    // Forward to view switcher for Tab/Shift+Tab
    return viewSwitcher_->keyPressed(key, origin);
}

} // namespace zenith::ui
