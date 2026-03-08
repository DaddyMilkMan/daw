/**
 * @file MainWindow.h
 * @brief Main application window for Zenith DAW
 *
 * Contains the main UI layout and hosts the audio engine.
 */

#pragma once

#include "../Source/engine/RecentProjectManager.h"
#include "../../commands/CommandAPI.h"
#include "../framework/SkiaMainWindowIntegration.h"
#include "../framework/AuroraBackground.h"
#include "../design-system/ZenithLookAndFeel.h"
#include "Engine.h"
#include "ProjectState.h"
#include "../transport/TransportBar.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "TitleBarComponent.h"
#include <memory>
#include "../dialogs/ExportDialog.h"
#include "../settings/ModernSettingsPanel.h"
#include "../dialogs/ProjectRecoveryModal.h"
#include "../dialogs/UnsavedChangesModal.h"
#include "../controls/SkiaAlertWindow.h"
#include "../controls/SkiaFileChooser.h"

namespace zenith {

// namespace network { class EmbeddedMCPHttpServer; }

class ProjectRecoveryModal;
class MainLayoutComponent;
class RightSidePanel;
class ZenithHubComponent;
class ProjectFileIO;

//==============================================================================
/**
 * @class MainComponent
 * @brief Main content component that holds the UI
 */
class MainComponent : public SkiaMainWindowIntegration,
                      public juce::KeyListener {
public:
  using LoadProjectCallback = std::function<void(const juce::File &)>;
  using NewProjectCallback = std::function<void()>;

  MainComponent(Engine &engine, CommandAPI &api,
                ProjectState &state,
                RecentProjectManager &recentProjects,
                LoadProjectCallback onLoadProject,
                NewProjectCallback onNewProject);
  ~MainComponent() override;

  // Menu Callbacks
  std::function<void()> onOpenProjectRequest;
  std::function<void()> onSaveProjectRequest;
  std::function<void()> onSaveProjectAsRequest;
  std::function<void()> onUndoRequest;
  std::function<void()> onRedoRequest;
  std::function<void()> onToggleMixerRequest;


  void paint(juce::Graphics &g) override;
  void resized() override;
  void parentHierarchyChanged() override;
  void visibilityChanged() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  
  // Diagnostic
  void logHierarchy();
  
  void handleAnimationTimer();
  void startAnimations();

protected:
  void drawSkiaContent(SkCanvas *canvas) override;

public:
  bool keyPressed(const juce::KeyPress &key,
                  Component *originatingComponent) override;

private:
  struct AnimationTimer : public juce::Timer {
      MainComponent& owner;
      AnimationTimer(MainComponent& o) : owner(o) {}
      void timerCallback() override { owner.handleAnimationTimer(); }
  };
  std::unique_ptr<AnimationTimer> animationTimer_;

  juce::Component *activeDragComponent = nullptr;
  juce::Rectangle<int> dragStartBounds;

  zenith::AuroraBackground aurora_;
  float animationTime_ = 0.0f;

  void openPianoRoll(const juce::String &trackId, const juce::String &clipId);
  void setMainUiVisible(bool shouldBeVisible);

  Engine &engine;
  ProjectState &projectState;
  RecentProjectManager &recentProjectManager_;
  LoadProjectCallback onLoadProject_;
  NewProjectCallback onNewProject_;

  std::unique_ptr<ZenithHubComponent> hubComponent;
  std::unique_ptr<TransportBar> transportBar;
  std::unique_ptr<TitleBarComponent> titleBar;
  std::unique_ptr<MainLayoutComponent> mainLayout;
  
  std::unique_ptr<ExportDialog> exportDialog;
  std::unique_ptr<ModernSettingsPanel> settingsPanel;
  // std::unique_ptr<network::EmbeddedMCPHttpServer> mcpHttpServer;
  float lastCpuPercent_ = -1.0f;
  bool lastPlayingState_ = false;
  bool lastRecordingState_ = false;
  double lastTempoState_ = -1.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

//==============================================================================
/**
 * @class MainWindow
 * @brief Top-level application window
 */
class MainWindow : public juce::Component, private juce::Timer {
public:
  explicit MainWindow(const juce::String &name);
  ~MainWindow() override;

  void requestClose();
  void resized() override;
  void onHostShown();

  zenith::ProjectState *getProjectState() const { return projectState.get(); }

  void saveProject();
  void saveProjectAs();
  bool loadProject(const juce::File &file);
  void openProject();
  void newProject();

  zenith::RecentProjectManager &getRecentProjectManager() {
    return *recentProjectManager_;
  }
  
  // Requests a quit check (checks dirty state, shows modal if needed)
  void checkUnsavedAndQuit(); 

private:
  void showAboutDialog();
  void timerCallback() override;
  void checkForRecovery();
  void createManualBackup();
  void updateWindowTitle();

  juce::File currentProjectFile;
  std::unique_ptr<Engine> engine;
  std::unique_ptr<ProjectState> projectState;
  std::unique_ptr<ProjectFileIO> fileIO_;
  std::unique_ptr<CommandAPI> commandAPI;
  std::unique_ptr<RecentProjectManager> recentProjectManager_;
  std::unique_ptr<ZenithLookAndFeel> lookAndFeel;

  std::unique_ptr<MainComponent> mainComponent;

  std::unique_ptr<ProjectRecoveryModal> recoveryModal_;
  std::unique_ptr<UnsavedChangesModal> unsavedChangesModal_;
  std::unique_ptr<SkiaAlertWindow> activeAlert_;
  std::unique_ptr<SkiaFileChooser> activeFileChooser_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace zenith
