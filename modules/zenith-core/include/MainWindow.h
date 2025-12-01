/**
 * @file MainWindow.h
 * @brief Main application window for Zenith DAW
 *
 * Contains the main UI layout and hosts the audio engine.
 *
 * - Audio engine integration
 */

#pragma once

#include "ArrangerView.h"
#include "ArrangementComponent.h"
#include "ClipSynchronizer.h"
#include "Engine.h"
#include "MixerComponent.h"
#include "ProjectState.h"
#include "TrackAutomationSynchronizer.h"
#include "TrackStateSynchronizer.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "rendering/SkiaRenderer.h"
#include "ui/skia/BottomBar.h"
#include "ui/skia/BrowserPanel.h"
#include "ui/skia/RightSidePanel.h"
#include "ui/skia/SkiaButtonComponent.h"
#include "ui/skia/SkiaButtonNative.h"
#include "ui/skia/SkiaColorTestComponent.h"
#include "ui/skia/SkiaLabel.h"
#include "ui/skia/SkiaMainWindowIntegration.h"
#include "ui/skia/SkiaTextDisplay.h"
#include "ui/skia/TransportBar.h"
#include "ui/skia/views/PianoKeyboardViewSkia.h"
#include "ui/skia/views/SessionViewComponent.h"
#include <include/core/SkColor.h>
#endif

// UI panels
#include "ui/WingmanPanel.h"
#include "ui/InstrumentBrowserPanel.h"

namespace zenith {
class CommandAPI;
class AIBridgeClient;
class MainLayoutComponent;
} // namespace zenith

//==============================================================================
/**
 * @class MainComponent
 * @brief Main content component that holds the UI
 *
 * This component is the main content area and contains:
 * - Transport bar (play/stop/record)
 * - ArrangerComponent (Phase 9 - interactive clip editing)
 * - Arrangement view (Phase 14 - automation display)
 * - Mixer panel (Phase 10)
 * - Status displays (CPU, device info, track count)
 * - Wingman AI panel (Phase 7)
 * - Instrument browser panel
 *
 * Integration points:
 * - Hosts ArrangerComponent which displays ProjectState clips
 * - Supports piano roll editing for MIDI clips
 * - Automation display and editing
 */
#ifdef ZENITH_USE_SKIA
class MainComponent : public zenith::SkiaMainWindowIntegration,
                      public juce::KeyListener
#else
class MainComponent : public juce::Component,
                      private juce::Timer,
                      public juce::KeyListener
#endif
{
public:
  //==========================================================================
  MainComponent(Engine &engine, zenith::CommandAPI &api,
                zenith::AIBridgeClient &aiClient, ProjectState &state);
  ~MainComponent() override;

  //==========================================================================
  // Component interface
  //==========================================================================

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;

  //==========================================================================
  // KeyListener interface (for undo/redo shortcuts)
  //==========================================================================

  bool keyPressed(const juce::KeyPress &key,
                  Component *originatingComponent) override;

#ifdef ZENITH_USE_SKIA
  //==========================================================================
  // SkiaMainWindowIntegration interface
  //==========================================================================

  void renderSkia(SkCanvas* canvas) override;
#endif

private:
#ifndef ZENITH_USE_SKIA
  //==========================================================================
  // Timer interface (for status updates)
  //==========================================================================

  void timerCallback() override;
#endif

  //==========================================================================
  // C4: Track count monitoring (read-only, dirty-checked)
  //==========================================================================

  void refreshTrackCountLabel();

  //==========================================================================
  // Integration: Piano roll opener
  //==========================================================================

  /**
   * @brief Open piano roll editor for a MIDI clip
   * @param trackId Track ID
   * @param clipId Clip ID
   */
  void openPianoRoll(const juce::String &trackId, const juce::String &clipId);

  //==========================================================================
  // Member variables
  //==========================================================================

  Engine &engine;
  ProjectState &projectState;

  // ============================================================================
  // Modern DAW Layout Panels
  // ============================================================================

#ifdef ZENITH_USE_SKIA
  // Top: Transport bar with play/stop/record, tempo, CPU, etc.
  std::unique_ptr<zenith::TransportBar> transportBar;

  // The "Perfect DAW" Tri-Pane Layout Manager
  // Manages Browser, Session View, and Arranger View
  std::unique_ptr<zenith::MainLayoutComponent> mainLayout;

  // Right: Scratch Pads + Wingman Console
  std::unique_ptr<zenith::RightSidePanel> rightSidePanel;

  // Bottom: Piano keyboard + mixer strip
  std::unique_ptr<zenith::BottomBar> bottomBar;
#else
  // JUCE fallback UI components
  juce::Label statusLabel;
  juce::Label cpuLabel;
  juce::TextButton playButton;
  juce::TextButton stopButton;
  juce::TextButton recordButton;
  juce::TextButton importButton;
  juce::TextButton virtualKeyboardButton;
  juce::Label audioDeviceLabel;
  juce::Label trackCountLabel;

  MixerComponent mixerComponent;
  std::unique_ptr<zenith::WingmanPanel> wingmanPanel;
  std::unique_ptr<zenith::InstrumentBrowserPanel> instrumentBrowserPanel;

  std::unique_ptr<juce::MidiKeyboardComponent> midiKeyboard;
  bool virtualKeyboardVisible = false;
#endif

  int lastTrackCount_ = -1;

  // Phase 9: Arranger component with interactive clip editing (center)
  // Now managed by MainLayoutComponent in Skia builds
#ifndef ZENITH_USE_SKIA
  std::unique_ptr<ArrangerView> arrangerComponent;
#endif

  // Wingman panel (owned by MainComponent, hosted in RightSidePanel when using
  // Skia)
#ifdef ZENITH_USE_SKIA
  std::unique_ptr<zenith::WingmanPanel> wingmanPanelPtr_;
#endif

  // Virtual MIDI Keyboard state (shared between Skia and JUCE builds)
  juce::MidiKeyboardState midiKeyboardState;

  //==========================================================================
  // Phase 1: Audio import
  //==========================================================================

  void handleImportAudio();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

//==============================================================================
/**
 * @class MainWindow
 * @brief Top-level application window
 *
 * Manages:
 * - Window lifecycle
 * - Menu bar
 * - Main content component
 * - Audio engine
 * - Project state
 */
class MainWindow : public juce::DocumentWindow {
public:
  //==========================================================================
  explicit MainWindow(const juce::String &name);
  ~MainWindow() override;

  //==========================================================================
  // DocumentWindow interface
  //==========================================================================

  void closeButtonPressed() override;

private:
  //==========================================================================
  // Menu bar model
  //==========================================================================

  /**
   * @class ZenithMenuBar
   * @brief Menu bar model for the application
   */
  class ZenithMenuBar : public juce::MenuBarModel {
  public:
    explicit ZenithMenuBar(MainWindow &owner);

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex,
                                    const juce::String &menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

  private:
    MainWindow &owner;

    enum MenuItems { aboutZenith = 1, quit = 2 };
  };

  //==========================================================================
  // Menu handlers
  //==========================================================================

  void showAboutDialog();

  //==========================================================================
  // Member variables
  //==========================================================================

  // Audio engine (created first, destroyed last)
  std::unique_ptr<Engine> engine;

  // Project state
  std::unique_ptr<ProjectState> projectState;

  // Phase 11: Track state synchronizer (general track state sync)
  std::unique_ptr<TrackStateSynchronizer> trackSynchronizer;

  // Phase 13: Automation synchronizer (automation-specific sync)
  std::unique_ptr<TrackAutomationSynchronizer> automationSync;

  // Phase 5: Wingman command API
  std::unique_ptr<zenith::CommandAPI> commandAPI;

  // Phase 7: AI bridge client
  std::unique_ptr<zenith::AIBridgeClient> aiBridgeClient;

  // Integration: Clip synchronizer
  std::unique_ptr<ClipSynchronizer> clipSynchronizer;

  // Main content
  std::unique_ptr<MainComponent> mainComponent;

  // Menu bar
  std::unique_ptr<ZenithMenuBar> menuBar;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
