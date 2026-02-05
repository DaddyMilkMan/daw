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

    BottomBar.h
    Created: 2025-11-28
    Author:  David Chen + Leo Rossi

    Bottom bar container with Piano Keyboard, Mixer Strip, and Debug Console.

  ==============================================================================

*/

#pragma once

#include <memory>
#include <vector>

#include "../../engine/EngineConstants.h"
#include "../mixer/MixerComponent.h"
#include "PianoKeyboardViewSkia.h"
#include "SkiaComponent.h"

#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {
class Engine;
class ProjectState;
class DeviceChainComponent;
class MixerComponent;

// Forward declarations
namespace ai {
class SessionDebuggerAgent;
}
class DebugConsoleComponent;
class AutoSaveIndicator;

#ifdef ZENITH_USE_SKIA

class BottomBar : public SkiaComponent {
public:
  BottomBar(juce::MidiKeyboardState &state, Engine &engine,
            ProjectState &projectState);
  ~BottomBar() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void setKeyboardVisible(bool visible);
  bool isKeyboardVisible() const { return keyboardVisible_; }

  void setDeviceChainVisible(bool visible);
  bool isDeviceChainVisible() const { return deviceChainVisible_; }

  // Debug Console integration
  void setDebugger(ai::SessionDebuggerAgent *debugger);
  void setDebugConsoleVisible(bool visible);
  bool isDebugConsoleVisible() const { return debugConsoleVisible_; }

private:
  juce::MidiKeyboardState &midiState_;
  std::unique_ptr<PianoKeyboardViewSkia> pianoKeyboard_;
  std::unique_ptr<DebugConsoleComponent> debugConsole_;
  std::unique_ptr<DeviceChainComponent> deviceChain_;
  std::unique_ptr<MixerComponent> mixerComponent_;
  std::unique_ptr<AutoSaveIndicator> autoSaveIndicator_;

  bool keyboardVisible_ = false;
  bool deviceChainVisible_ = true;  // Show device chain by default
  bool debugConsoleVisible_ = true; // Show by default

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomBar)

private:
  // Cached resources for 60FPS rendering
  ::SkPaint bgPaint_;
  ::SkPaint borderPaint_;
  ::SkPaint channelBgPaint_;
  ::SkPaint meterTrackPaint_;
  ::SkPaint meterFillPaint_;
  ::SkPaint textPaint_;
  ::SkFont font_;
  ::SkRect cachedBounds_;

  void updateCachedPaints(const ::SkRect &bounds);
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
