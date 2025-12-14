/**
 * @file MainWindow.h
 * @brief Main application window for Zenith DAW
 *
 * Contains the main UI layout and hosts the audio engine.
 *
 * - Audio engine integration
 */

#pragma once

#include "../Source/engine/RecentProjectManager.h"
#include "../Source/ui/SessionViewComponent.h"
#include "../Source/ui/skia/BottomBar.h"
#include "../Source/ui/skia/BrowserPanel.h"
#include "../Source/ui/skia/RightSidePanel.h"
#include "../Source/ui/skia/SkiaButton.h"
#include "../Source/ui/skia/SkiaMainWindowIntegration.h"
#include "../Source/ui/skia/TransportBar.h"
#include "../Source/ui/skia/views/PianoKeyboardViewSkia.h"
#include "ArrangementComponent.h"
#include "ClipSynchronizer.h"
#include "Engine.h"
#include "ProjectState.h"
#include "TrackAutomationSynchronizer.h"
#include "TrackStateSynchronizer.h"
#include "ui/ArrangerComponent.h"
#include "ui/MixerComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {
class InstrumentBrowserPanel;
class CommandAPI;
class AIBridgeClient;
class MainLayoutComponent;
class WingmanPanel;
class ZenithMenuBar;
class ZenithHubComponent;
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
class MainComponent : public zenith::SkiaMainWindowIntegration,
                      public juce::KeyListener {
public:
  //==========================================================================
  // Callback type for project loading
  using LoadProjectCallback = std::function<void(const juce::File &)>;
  using NewProjectCallback = std::function<void()>;

  MainComponent(zenith::Engine &engine, zenith::CommandAPI &api,
                zenith::AIBridgeClient &aiClient, zenith::ProjectState &state,
                zenith::RecentProjectManager &recentProjects,
                LoadProjectCallback onLoadProject,
                NewProjectCallback onNewProject);
  ~MainComponent() override;

  //==========================================================================
  // Component interface
  //==========================================================================

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

protected:
  void drawSkiaContent(SkCanvas *canvas) override;

public:
  //==========================================================================
  // KeyListener interface (for undo/redo shortcuts)
  //==========================================================================

  bool keyPressed(const juce::KeyPress &key,
                  Component *originatingComponent) override;

private:
  // Layout Editing State
  juce::Component *activeDragComponent = nullptr;
  juce::Rectangle<int> dragStartBounds;

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

  zenith::Engine &engine;
  zenith::ProjectState &projectState;
  zenith::RecentProjectManager &recentProjectManager_;
  LoadProjectCallback onLoadProject_;
  NewProjectCallback onNewProject_;

  // ============================================================================
  // Modern DAW Layout Panels
  // ============================================================================

  // Top: Transport bar with play/stop/record, tempo, CPU, etc.
  std::unique_ptr<zenith::TransportBar> transportBar;

  // The "Perfect DAW" Tri-Pane Layout Manager
  // Manages Browser, Session View, and Arranger View
  std::unique_ptr<zenith::MainLayoutComponent> mainLayout;

  // Right: Scratch Pads + Wingman Console
  std::unique_ptr<zenith::RightSidePanel> rightSidePanel;

  // Bottom: Piano keyboard + mixer strip
  std::unique_ptr<zenith::BottomBar> bottomBar;

  // Wingman panel (owned by MainComponent, hosted in RightSidePanel when using
  // Skia)
  std::unique_ptr<zenith::WingmanPanel> wingmanPanelPtr_;

  // Virtual MIDI Keyboard state (shared between Skia and JUCE builds)
  juce::MidiKeyboardState midiKeyboardState;

  //==========================================================================
  // Phase 1: Audio import
  //==========================================================================

  // Phase 1: Audio import
  //==========================================================================

  void handleImportAudio();

  // Zenith Hub (Start Screen)
  std::unique_ptr<zenith::ZenithHubComponent> hubComponent;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

//==============================================================================

//==============================================================================
/**
 * @class MainWindow
 * @brief Top-level application window
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

  zenith::ProjectState *getProjectState() const { return projectState.get(); }

private:
  //==========================================================================
  // Menu bar model
  //==========================================================================

  // Legacy MenuBar removed
  // Custom menu bar is now part of MainComponent

  //==========================================================================
  // Menu handlers
  //==========================================================================

  void showAboutDialog();

  /**
   * @brief Save the current project
   */
public:
  void saveProject();

  /**
   * @brief Save the current project to a new file
   */
  void saveProjectAs();

  /**
   * @brief Load a project from file
   * @param file The project file to load
   * @return true if successful
   */
  bool loadProject(const juce::File &file);

  /**
   * @brief Open a project file dialog and load selected project
   */
  void openProject();

  /**
   * @brief Get the recent project manager
   */
  zenith::RecentProjectManager &getRecentProjectManager() {
    return *recentProjectManager_;
  }

  //==========================================================================
  // Member variables
  //==========================================================================

  // Current project file (empty if new project)
  juce::File currentProjectFile;

  // Audio engine (created first, destroyed last)
  std::unique_ptr<zenith::Engine> engine;

  // Project state
  std::unique_ptr<zenith::ProjectState> projectState;

  // Phase 11: Track state synchronizer (general track state sync)
  std::unique_ptr<zenith::TrackStateSynchronizer> trackSynchronizer;

  // Phase 13: Automation synchronizer (automation-specific sync)
  std::unique_ptr<zenith::TrackAutomationSynchronizer> automationSync;

  // Phase 5: Wingman command API
  std::unique_ptr<zenith::CommandAPI> commandAPI;

  // Phase 7: AI bridge client
  std::unique_ptr<zenith::AIBridgeClient> aiBridgeClient;

  // Integration: Clip synchronizer
  std::unique_ptr<zenith::ClipSynchronizer> clipSynchronizer;

  // Recent Project Manager (Pinocchio Protocol)
  std::unique_ptr<zenith::RecentProjectManager> recentProjectManager_;

  // Main content
  std::unique_ptr<MainComponent> mainComponent;

  // Tooltips
  juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
