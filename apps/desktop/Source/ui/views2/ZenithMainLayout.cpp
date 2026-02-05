/*
  ==============================================================================

    ZenithMainLayout.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of main layout container.

  ==============================================================================
*/

#include "ZenithMainLayout.h"
#include "../design-system/ZenithTheme.h"

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

ZenithMainLayout::ZenithMainLayout() {
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    
    // Create transport bar
    transportBar_ = std::make_unique<SkiaTransportBar>();
    addAndMakeVisible(transportBar_.get());
    
    // Create view switcher
    viewSwitcher_ = std::make_unique<ViewSwitcher>();
    viewSwitcher_->addListener(this);
    addAndMakeVisible(viewSwitcher_.get());
    
    // Wire up callbacks
    setupCallbacks();
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
            case SkiaTransportBar::ActiveView::AIJam:
                viewSwitcher_->setActiveView(ViewType::AIJam);
                break;
        }
    };
    
    // Placeholder transport actions
    transportBar_->onPlay = [this]() {
        transportBar_->setState(TransportState::Playing);
    };
    
    transportBar_->onStop = [this]() {
        transportBar_->setState(TransportState::Stopped);
        transportBar_->setPosition(0.0);
    };
    
    transportBar_->onRecord = [this]() {
        if (transportBar_->getState() == TransportState::Recording) {
            transportBar_->setState(TransportState::Playing);
        } else {
            transportBar_->setState(TransportState::Recording);
        }
    };
    
    transportBar_->onLoopToggle = [this]() {
        transportBar_->setLoopEnabled(!transportBar_->isLoopEnabled());
    };
    
    transportBar_->onMetronomeToggle = [this]() {
        transportBar_->setMetronomeEnabled(!transportBar_->isMetronomeEnabled());
    };
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

//==============================================================================
// ViewSwitcher::Listener
//==============================================================================

void ZenithMainLayout::viewDidChange(ViewType newView) {
    // Sync transport bar view toggle
    switch (newView) {
        case ViewType::Arrangement:
            transportBar_->setActiveView(SkiaTransportBar::ActiveView::Arrangement);
            break;
        case ViewType::Session:
            transportBar_->setActiveView(SkiaTransportBar::ActiveView::Session);
            break;
        case ViewType::AIJam:
            transportBar_->setActiveView(SkiaTransportBar::ActiveView::AIJam);
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
