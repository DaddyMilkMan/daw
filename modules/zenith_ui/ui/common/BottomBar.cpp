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

    BottomBar.cpp
    Created: 2025-11-28
    Author:  David Chen + Leo Rossi

  ==============================================================================
*/


#include "BottomBar.h"
#include "../../ai/SessionDebuggerAgent.h"
#include "../controls/DebugConsoleComponent.h"
#include "../controls/DeviceChainComponent.h"
#include "../transport/AutoSaveIndicator.h"
#include "../mixer/MixerComponent.h"
#include <memory>

#define ZENITH_USE_SKIA 1 // FORCE DEFINITION FOR DEBUGGING

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkPoint.h>
#include <effects/SkGradientShader.h>

#endif

#include "../controls/DeviceChainComponent.h"
#include "Engine.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

BottomBar::BottomBar(juce::MidiKeyboardState &state, Engine &engine,
                     ProjectState &projectState)
    : midiState_(state) {
  // Create Piano Keyboard
  pianoKeyboard_ = std::make_unique<PianoKeyboardViewSkia>(
      midiState_, juce::MidiKeyboardComponent::horizontalKeyboard);
  addChildComponent(pianoKeyboard_.get());

  // Create Device Chain
  deviceChain_ = std::make_unique<DeviceChainComponent>(engine, projectState);
  addChildComponent(deviceChain_.get());

  // Create Mixer Component
  mixerComponent_ = std::make_unique<MixerComponent>(engine, projectState);
  addChildComponent(mixerComponent_.get());

  // Create Auto-Save Indicator
  autoSaveIndicator_ = std::make_unique<AutoSaveIndicator>(projectState);
  addChildComponent(autoSaveIndicator_.get());
  autoSaveIndicator_->setVisible(true);

  // Debug console is created when setDebugger is called

  // Default size
  setSize(800, 150);
}

BottomBar::~BottomBar() {
  // Destructor implementation needed because of unique_ptr to incomplete types
  pianoKeyboard_.reset();
  debugConsole_.reset();
  deviceChain_.reset();
  mixerComponent_.reset();
  autoSaveIndicator_.reset();
}

void BottomBar::setDebugger(ai::SessionDebuggerAgent *debugger) {
  if (debugger) {
    debugConsole_ = std::make_unique<DebugConsoleComponent>(*debugger);
    addAndMakeVisible(debugConsole_.get());
    debugConsole_->setVisible(debugConsoleVisible_);
    resized();
  } else {
    debugConsole_.reset();
  }
}

void BottomBar::setDebugConsoleVisible(bool visible) {
  debugConsoleVisible_ = visible;
  if (debugConsole_) {
    debugConsole_->setVisible(visible);
  }
  repaint();
}

void BottomBar::setDeviceChainVisible(bool visible) {
  deviceChainVisible_ = visible;
  if (deviceChain_)
    deviceChain_->setVisible(visible);
  if (mixerComponent_)
    mixerComponent_->setVisible(!visible && !keyboardVisible_);
  resized();
}

void BottomBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Lazy update of cached resources on the Render Thread
  if (skBounds != cachedBounds_) {
    updateCachedPaints(skBounds);
    cachedBounds_ = skBounds;
  }

  // Background
  canvas->drawRect(skBounds, bgPaint_);

  // Top border glow
  canvas->drawLine(0.0f, 0.0f, skBounds.width(), 0.0f, borderPaint_);

  // If keyboard is hidden, show mixer strip OR device chain
  // Iterate through all visible children and render them if they are
  // SkiaComponents
  for (auto *child : getChildren()) {
    if (child->isVisible()) {
      if (auto *skiaChild = dynamic_cast<SkiaComponent *>(child)) {
        canvas->save();

        // Translate to child position
        canvas->translate((float)child->getX(), (float)child->getY());

        // Clip to child bounds to prevent bleeding
        canvas->clipRect(SkRect::MakeWH((float)child->getWidth(),
                                        (float)child->getHeight()));

        skiaChild->drawSkia(canvas);

        canvas->restore();
      }
    }
  }
}

void BottomBar::updateCachedPaints(const SkRect &bounds) {
  // 1. Background Paint
  bgPaint_.setAntiAlias(true);
  bgPaint_.setColor(SkColorSetARGB(255, 20, 20, 20)); // Opaque dark grey
  bgPaint_.setStyle(SkPaint::kFill_Style);

  // 2. Border Paint (Gradient)
  borderPaint_.setAntiAlias(true);
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);

  SkPoint points[2] = {SkPoint::Make(0.0f, 0.0f),
                       SkPoint::Make(bounds.width(), 0.0f)};
  SkColor colors[3] = {0x0000AAFF, 0xFF00AAFF, 0x0000AAFF};
  borderPaint_.setShader(SkGradientShader::MakeLinear(points, colors, nullptr,
                                                      3, SkTileMode::kClamp));

  // 3. Channel Background
  channelBgPaint_.setAntiAlias(true);
  channelBgPaint_.setColor(SkColorSetARGB(30, 255, 255, 255));
  channelBgPaint_.setStyle(SkPaint::kFill_Style);

  // 4. Meter Track
  meterTrackPaint_.setAntiAlias(true);
  meterTrackPaint_.setColor(SkColorSetARGB(50, 0, 0, 0));
  meterTrackPaint_.setStyle(SkPaint::kFill_Style);

  // 5. Meter Fill (Base)
  meterFillPaint_.setAntiAlias(true);
  meterFillPaint_.setStyle(SkPaint::kFill_Style);

  // 6. Text Paint
  textPaint_.setAntiAlias(true);
  textPaint_.setColor(SkColorSetARGB(150, 255, 255, 255));
  textPaint_.setStyle(SkPaint::kFill_Style);

  // 7. Font
  font_.setSize(10.0f);
  font_.setSubpixel(true);
}

void BottomBar::resized() {
  auto area = getLocalBounds();

  if (pianoKeyboard_) {
    // Piano takes full height if visible
    if (keyboardVisible_) {
      pianoKeyboard_->setBounds(area);
      if (deviceChain_)
        deviceChain_->setVisible(false);
    } else {
      // Keyboard hidden
      pianoKeyboard_->setVisible(false);

      // Setup Device Chain area
      auto linkArea = area;

      // Position debug console in the bottom-right corner
      if (debugConsole_ && debugConsoleVisible_) {
        int consoleWidth = 320;
        int consoleHeight = debugConsole_->isExpanded() ? 120 : 32;

        debugConsole_->setBounds(area.getRight() - consoleWidth - 10,
                                 area.getCentreY() - consoleHeight / 2,
                                 consoleWidth, consoleHeight);

        // Should device chain avoid console?
        linkArea.removeFromRight(consoleWidth + 20);
      }

      // Position Auto-Save Indicator (Top Right of Bottom Bar)
      if (autoSaveIndicator_) {
        autoSaveIndicator_->setBounds(area.getWidth() - 100, 5, 80, 20);
      }

      if (deviceChain_ && deviceChainVisible_) {
        deviceChain_->setVisible(true);
        deviceChain_->setBounds(linkArea);
        if (mixerComponent_)
          mixerComponent_->setVisible(false);
      } else if (mixerComponent_) {
        mixerComponent_->setVisible(true);
        mixerComponent_->setBounds(linkArea);
        if (deviceChain_)
          deviceChain_->setVisible(false);
      }
    }
  }
}

void BottomBar::setKeyboardVisible(bool visible) {
  keyboardVisible_ = visible;
  if (pianoKeyboard_) {
    pianoKeyboard_->setVisible(visible);
  }
  // Trigger resized to update mixer visibility
  resized();
  repaint();
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
