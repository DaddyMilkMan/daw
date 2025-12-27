/**
 * @file MainWindow.h
 * @brief Main application window for Zenith DAW
 *
 * Contains the main UI layout and hosts the audio engine.
 */

#pragma once

#include "../Source/engine/RecentProjectManager.h"
#include "../arranger/ArrangerComponent.h"
#include "../browser/BrowserPanel.h"
#include "../controls/SkiaButton.h"
#include "../framework/SkiaMainWindowIntegration.h"
#include "../mixer/MixerComponent.h"
#include "../session/SessionViewComponent.h"
#include "../transport/TransportBar.h"
#include "ArrangementComponent.h"
#include "BottomBar.h"
#include "ClipSynchronizer.h"
#include "Engine.h"
#include "PianoKeyboardViewSkia.h"
#include "ProjectState.h"
#include "RightSidePanel.h"
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
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace zenith {
class InstrumentBrowserPanel;
class CommandAPI;
class MainLayoutComponent;
class WingmanPanel;
class ZenithMenuBar;
class ZenithHubComponent;
class ZenithKnob;
class ProjectFileIO;
namespace ai {
class UXDirectorAgent;
class PresetGeneticistAgent;
} // namespace ai
namespace mcp {
class MCPServer;
} // namespace mcp
} // namespace zenith

//==============================================================================
/**
 * @class MainComponent
 * @brief Main content component that holds the UI
 */
class MainComponent : public zenith::SkiaMainWindowIntegration,
                      public juce::KeyListener {
public:
  using LoadProjectCallback = std::function<void(const juce::File &)>;
  using NewProjectCallback = std::function<void()>;

  MainComponent(zenith::Engine &engine, zenith::CommandAPI &api,
                zenith::ProjectState &state,
                zenith::RecentProjectManager &recentProjects,
                LoadProjectCallback onLoadProject,
                NewProjectCallback onNewProject);
  ~MainComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void parentHierarchyChanged() override;
  void visibilityChanged() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

protected:
  void drawSkiaContent(SkCanvas *canvas) override;

public:
  bool keyPressed(const juce::KeyPress &key,
                  Component *originatingComponent) override;

private:
  juce::Component *activeDragComponent = nullptr;
  juce::Rectangle<int> dragStartBounds;

  void openPianoRoll(const juce::String &trackId, const juce::String &clipId);

  zenith::Engine &engine;
  zenith::ProjectState &projectState;
  zenith::RecentProjectManager &recentProjectManager_;
  LoadProjectCallback onLoadProject_;
  NewProjectCallback onNewProject_;

  std::unique_ptr<zenith::TransportBar> transportBar;
  std::unique_ptr<zenith::MainLayoutComponent> mainLayout;
  std::unique_ptr<zenith::RightSidePanel> rightSidePanel;
  std::unique_ptr<zenith::BottomBar> bottomBar;
  std::unique_ptr<zenith::WingmanPanel> wingmanPanelPtr_;

  juce::MidiKeyboardState midiKeyboardState;

  void handleImportAudio();

  std::unique_ptr<zenith::ZenithHubComponent> hubComponent;
  std::unique_ptr<zenith::ZenithKnob> volumeKnob;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

//==============================================================================
/**
 * @class MainWindow
 * @brief Top-level application window
 */
class MainWindow : public juce::DocumentWindow, private juce::Timer {
public:
  explicit MainWindow(const juce::String &name);
  ~MainWindow() override;

  void closeButtonPressed() override;

  zenith::ProjectState *getProjectState() const { return projectState.get(); }

  void saveProject();
  void saveProjectAs();
  bool loadProject(const juce::File &file);
  void openProject();
  void newProject();

  zenith::RecentProjectManager &getRecentProjectManager() {
    return *recentProjectManager_;
  }

private:
  void showAboutDialog();
  void timerCallback() override;
  void checkForRecovery();
  void createManualBackup();
  void updateWindowTitle();

  juce::File currentProjectFile;
  std::unique_ptr<zenith::Engine> engine;
  std::unique_ptr<zenith::ProjectState> projectState;
  std::unique_ptr<zenith::ProjectFileIO> fileIO_;
  std::unique_ptr<zenith::TrackStateSynchronizer> trackSynchronizer;
  std::unique_ptr<zenith::TrackAutomationSynchronizer> automationSync;
  std::unique_ptr<zenith::CommandAPI> commandAPI;
  std::unique_ptr<zenith::ClipSynchronizer> clipSynchronizer;
  std::unique_ptr<zenith::RecentProjectManager> recentProjectManager_;

  std::unique_ptr<MainComponent> mainComponent;

  std::unique_ptr<zenith::ai::UXDirectorAgent> uxDirector;
  std::unique_ptr<zenith::ai::PresetGeneticistAgent> presetGeneticist;
  std::unique_ptr<zenith::mcp::MCPServer> mcpServer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};