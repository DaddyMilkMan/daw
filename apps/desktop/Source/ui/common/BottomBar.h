/*
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
#include "MixerComponent.h"
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
