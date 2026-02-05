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

    ZenithMainLayout.cpp - UPDATED VERSION
    Created: 2026-02-04
    Author:  Zenith DAW

    PRODUCTION UPDATE - Complete callback wiring for transport integration

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

ZenithMainLayout::ZenithMainLayout(zenith::Engine& engine, 
                                   zenith::ProjectState& projectState,
                                   zenith::CommandAPI* commandAPI)
    : engine_(&engine), projectState_(&projectState), commandAPI_(commandAPI) {
    
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Create transport bar
    transportBar_ = std::make_unique<SkiaTransportBar>();
    addAndMakeVisible(transportBar_.get());

    // Create view switcher
    viewSwitcher_ = std::make_unique<ViewSwitcher>(engine, projectState);
    viewSwitcher_->addListener(this);
    addAndMakeVisible(viewSwitcher_.get());

    // Wire up all callbacks
    setupCallbacks();
    setupEngineCallbacks();
}

ZenithMainLayout::ZenithMainLayout()
    : engine_(nullptr), projectState_(nullptr), commandAPI_(nullptr) {
    
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Create transport bar (standalone mode)
    transportBar_ = std::make_unique<SkiaTransportBar>();
    addAndMakeVisible(transportBar_.get());

    // Create view switcher (standalone mode)
    viewSwitcher_ = std::make_unique<ViewSwitcher>();
    viewSwitcher_->addListener(this);
    addAndMakeVisible(viewSwitcher_.get());

    // Setup basic callbacks
    setupCallbacks();
}

ZenithMainLayout::~ZenithMainLayout() {
    if (viewSwitcher_) {
        viewSwitcher_->removeListener(this);
    }
    removeKeyListener(this);
}

//==============================================================================
// Callback Setup - PRODUCTION IMPLEMENTATION
//==============================================================================

void ZenithMainLayout::setupCallbacks() {
    if (!transportBar_) return;

    // Transport control callbacks
    transportBar_->onPlay = [this]() {
        if (onPlayRequest) {
            onPlayRequest();
        } else if (commandAPI_) {
            commandAPI_->executeCommand(CommandAPI::CommandID::Play, juce::var());
        } else if (engine_) {
            engine_->play();
        }
    };

    transportBar_->onStop = [this]() {
        if (onStopRequest) {
            onStopRequest();
        } else if (commandAPI_) {
            commandAPI_->executeCommand(CommandAPI::CommandID::Stop, juce::var());
        } else if (engine_) {
            engine_->stop();
        }
    };

    transportBar_->onRecord = [this]() {
        if (onRecordRequest) {
            onRecordRequest();
        } else if (commandAPI_) {
            commandAPI_->executeCommand(CommandAPI::CommandID::Record, juce::var());
        } else if (engine_) {
            if (engine_->isRecording()) {
                engine_->stopRecording();
            } else {
                engine_->record();
            }
        }
    };

    transportBar_->onRewind = [this]() {
        if (onRewindRequest) {
            onRewindRequest();
        } else if (commandAPI_) {
            commandAPI_->executeCommand(CommandAPI::CommandID::Rewind, juce::var());
        }
    };

    transportBar_->onLoopToggle = [this]() {
        if (onLoopToggleRequest) {
            onLoopToggleRequest();
        } else if (engine_) {
            engine_->setLooping(!engine_->isLooping());
        }
        // Update visual state
        if (transportBar_ && engine_) {
            transportBar_->setLoopEnabled(engine_->isLooping());
        }
    };

    transportBar_->onMetronomeToggle = [this]() {
        if (onMetronomeToggleRequest) {
            onMetronomeToggleRequest();
        } else if (engine_) {
            engine_->toggleMetronome();
        }
    };

    // Tempo change callback
    transportBar_->onTempoChange = [this](float bpm) {
        if (onTempoChangeRequest) {
            onTempoChangeRequest(bpm);
        } else if (projectState_) {
            projectState_->setTempo(bpm);
        }
    };

    // Time signature change callback
    transportBar_->onTimeSignatureChange = [this](int num, int den) {
        if (onTimeSignatureChangeRequest) {
            onTimeSignatureChangeRequest(num, den);
        } else if (projectState_) {
            projectState_->setTimeSignature(num, den);
        }
    };

    // View change callback
    transportBar_->onViewChange = [this](SkiaTransportBar::ActiveView view) {
        switch (view) {
            case SkiaTransportBar::ActiveView::Arrangement:
                viewSwitcher_->setActiveView(ViewType::Arrangement);
                break;
            case SkiaTransportBar::ActiveView::Session:
                viewSwitcher_->setActiveView(ViewType::Session);
                break;
            case SkiaTransportBar::ActiveView::AIJam:
                // AI Jam is an overlay, not a main view
                // Toggle AI Jam visibility if implemented
                break;
        }
    };

    // Settings callback
    transportBar_->onSettings = [this]() {
        if (onSettingsRequest) {
            onSettingsRequest();
        }
    };

    // Wingman toggle callback
    transportBar_->onWingman = [this]() {
        if (onWingmanToggleRequest) {
            onWingmanToggleRequest();
        }
    };
}

void ZenithMainLayout::setupEngineCallbacks() {
    if (!transportBar_ || !projectState_) return;

    // Set initial values from project state
    transportBar_->setTempo(projectState_->getTempo());
    transportBar_->setTimeSignature(
        projectState_->getTimeSignatureNumerator(),
        projectState_->getTimeSignatureDenominator()
    );

    // Sync with engine state if available
    if (engine_) {
        transportBar_->setLoopEnabled(engine_->isLooping());
        
        if (engine_->isPlaying()) {
            transportBar_->setState(TransportState::Playing);
        } else if (engine_->isRecording()) {
            transportBar_->setState(TransportState::Recording);
        } else {
            transportBar_->setState(TransportState::Stopped);
        }
    }
}

//==============================================================================
// Transport Integration
//==============================================================================

void ZenithMainLayout::setPlayState(TransportState state) {
    if (transportBar_) {
        transportBar_->setState(state);
    }
}

void ZenithMainLayout::setPosition(double beats) {
    if (transportBar_) {
        transportBar_->setPosition(beats);
    }
    
    // Update arrangement view playhead
    if (auto* arr = viewSwitcher_->getArrangementView()) {
        arr->setPlayheadPosition(beats);
    }
}

void ZenithMainLayout::setTempo(float bpm) {
    if (transportBar_) {
        transportBar_->setTempo(bpm);
    }
}

void ZenithMainLayout::setLoopEnabled(bool enabled) {
    if (transportBar_) {
        transportBar_->setLoopEnabled(enabled);
    }
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

void ZenithMainLayout::viewDidChange(ViewType newView) {
    // Sync transport bar view toggle
    if (!transportBar_) return;
    
    switch (newView) {
        case ViewType::Arrangement:
            transportBar_->setActiveView(SkiaTransportBar::ActiveView::Arrangement);
            break;
        case ViewType::Session:
            transportBar_->setActiveView(SkiaTransportBar::ActiveView::Session);
            break;
        default:
            break;
    }
}

//==============================================================================
// Status Bar
//==============================================================================

void ZenithMainLayout::setCPULoad(float load) {
    cpuLoad_ = load;
    if (transportBar_) {
        transportBar_->setCPULoad(load);
    }
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
    zoomLevel_ *= 1.25f;
    markDirty();
}

void ZenithMainLayout::zoomOut() {
    zoomLevel_ /= 1.25f;
    markDirty();
}

void ZenithMainLayout::zoomToFit() {
    zoomLevel_ = 1.0f;
    markDirty();
}

//==============================================================================
// Layout
//==============================================================================

void ZenithMainLayout::resized() {
    auto bounds = getLocalBounds();
    
    // Transport bar at top
    if (transportBar_) {
        transportBar_->setBounds(0, 0, bounds.getWidth(), static_cast<int>(kTransportHeight));
    }
    
    // View switcher fills middle
    int viewTop = static_cast<int>(kTransportHeight);
    int viewHeight = bounds.getHeight() - static_cast<int>(kTransportHeight + kStatusBarHeight);
    if (viewSwitcher_) {
        viewSwitcher_->setBounds(0, viewTop, bounds.getWidth(), viewHeight);
    }
    
    // Status bar is drawn, not a separate component
}

//==============================================================================
// Drawing
//==============================================================================

void ZenithMainLayout::drawSkia(SkCanvas* canvas) {
    SkAutoCanvasRestore acr(canvas, true);
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
    
    // Transport bar draws itself
    if (transportBar_) {
        canvas->save();
        transportBar_->drawSkia(canvas);
        canvas->restore();
    }
    
    // View switcher draws itself
    if (viewSwitcher_) {
        canvas->save();
        canvas->translate(0, kTransportHeight);
        viewSwitcher_->drawSkia(canvas);
        canvas->restore();
    }
    
    // Status bar at bottom
    drawStatusBar(canvas);
}

void ZenithMainLayout::drawStatusBar(SkCanvas* canvas) {
    float y = getHeight() - kStatusBarHeight;
    
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
        if (transportBar_) {
            if (transportBar_->getState() == TransportState::Playing) {
                transportBar_->onStop();
            } else {
                transportBar_->onPlay();
            }
        }
        return true;
    }
    
    // R - Toggle Record
    if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R') {
        if (transportBar_) {
            transportBar_->onRecord();
        }
        return true;
    }
    
    // L - Toggle Loop
    if (key.getTextCharacter() == 'l' || key.getTextCharacter() == 'L') {
        if (transportBar_) {
            transportBar_->onLoopToggle();
        }
        return true;
    }
    
    // Forward to view switcher for Tab/Shift+Tab
    if (viewSwitcher_) {
        return viewSwitcher_->keyPressed(key, origin);
    }
    
    return false;
}

} // namespace zenith::ui
