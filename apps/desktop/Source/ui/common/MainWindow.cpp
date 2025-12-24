/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "MainWindow.h"
#include "../../commands/CommandAPI.h"
#include "../engine/Clip.h"
#include "../engine/Track.h"
#include "../network/AIBridgeClient.h"
#include "ArrangerComponent.h"
#include "ClipSynchronizer.h"
#include "InstrumentBrowserPanel.h"
#include "MainLayoutComponent.h"
#include "MenuBar.h"
#include "PianoRollComponent.h"
#include "SettingsComponent.h"
#include "TrackAutomationSynchronizer.h"
#include "WingmanPanel.h"
#include "ZenithHubComponent.h"
#include "ZenithLookAndFeel.h" // For colors

#include "../ai/PresetGeneticistAgent.h"
#include "../ai/SessionDebuggerAgent.h"
#include "../ai/UXDirectorAgent.h"
#include "../engine/ZenithLogger.h"

#include "SkiaComponent.h"
#include "SkiaMainWindowIntegration.h"
#include "ZenithDesignSystem.h"
#include <core/SkFont.h>
#include <core/SkImage.h>
#include <core/SkImageInfo.h>
#include <core/SkPixmap.h>
#include <core/SkSamplingOptions.h>
#include <core/SkSurface.h>
#include <core/SkTextBlob.h>

using namespace zenith;

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(zenith::Engine &eng, zenith::CommandAPI &api,
                             zenith::ProjectState &state,
                             zenith::RecentProjectManager &recentProjects,
                             LoadProjectCallback onLoadProject,
                             NewProjectCallback onNewProject)
    : engine(eng), projectState(state), recentProjectManager_(recentProjects),
      onLoadProject_(std::move(onLoadProject)),
      onNewProject_(std::move(onNewProject)) {
  // Thread Safety: UI component construction must happen on message thread
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Register as key listener for undo/redo shortcuts
  addKeyListener(this);
  addMouseListener(this, true); // Intercept mouse events recursively
  setWantsKeyboardFocus(true);

  setSize(1400, 800);

  ZENITH_LOG_INFO("========================================");
  ZENITH_LOG_INFO("MainComponent Constructor - Modern DAW Layout");
  ZENITH_LOG_INFO("========================================");

  // Initialize Skia rendering system
  // Skia initialization is handled by SkiaMainWindowIntegration

  // ============================================================================
  // Create Modern DAW Layout Panels
  // ============================================================================

  // Top: Transport Bar
  ZENITH_LOG_INFO("→ Creating TransportBar...");
  transportBar = std::make_unique<zenith::TransportBar>();
  transportBar->setProjectName("Zenith DAW");
  transportBar->setTempo(120.0);
  transportBar->setTimeSignature(4, 4);

  // Hook up transport callbacks
  transportBar->onPlayClicked = [this]() {
    engine.play();
    ZENITH_LOG_DEBUG("Play clicked");
  };
  transportBar->onStopClicked = [this]() {
    engine.stop();
    ZENITH_LOG_DEBUG("Stop clicked");
  };
  transportBar->onRecordClicked = [this]() {
    engine.toggleRecording();
    bool isRec = engine.isRecording();
    transportBar->setRecording(isRec);
    if (isRec) {
      ZENITH_LOG_DEBUG("Recording started");
    } else {
      ZENITH_LOG_DEBUG("Recording stopped");
    }
  };

  addAndMakeVisible(transportBar.get());
  ZENITH_LOG_INFO("✓ TransportBar created");

  // The "Perfect DAW" Tri-Pane Layout Manager
  ZENITH_LOG_INFO("→ Creating MainLayoutComponent...");
  mainLayout =
      std::make_unique<zenith::MainLayoutComponent>(engine, projectState);
  addAndMakeVisible(mainLayout.get());
  ZENITH_LOG_INFO("✓ MainLayoutComponent created");

  // Right: AI Assistant Panel (Wingman) - Pure Skia
  ZENITH_LOG_INFO("→ Creating RightSidePanel...");
  rightSidePanel = std::make_unique<zenith::RightSidePanel>(api, engine);
  addAndMakeVisible(rightSidePanel.get());
  ZENITH_LOG_INFO("✓ RightSidePanel created");

  // Bottom: Piano Keyboard + Mixer Strip
  ZENITH_LOG_INFO("→ Creating BottomBar...");
  bottomBar = std::make_unique<zenith::BottomBar>(midiKeyboardState, engine,
                                                  projectState);
  bottomBar->setKeyboardVisible(false); // Hidden by default

  // Connect Session Debugger
  if (auto *debugger = engine.getSessionDebugger()) {
    bottomBar->setDebugger(debugger);
    ZENITH_LOG_INFO("✓ Session Debugger connected to BottomBar");
  }

  addAndMakeVisible(bottomBar.get());
  ZENITH_LOG_INFO("✓ BottomBar created");

  // Connect view toggle callback
  transportBar->onViewToggleClicked = [this]() {
    if (mainLayout) {
      mainLayout->toggleView();
      DBG("View toggled via MainLayout");
    }
  };

  // Connect settings callback
  transportBar->onSettingsClicked = [this]() {
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(new zenith::SettingsComponent(engine));
    options.content->setSize(600, 500);
    options.dialogTitle = "Zenith DAW Settings";
    options.dialogBackgroundColour = juce::Colours::black;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
  };

  // Create Zenith Hub with real project manager
  hubComponent = std::make_unique<zenith::ZenithHubComponent>(
      recentProjectManager_,
      [this](const juce::File &projectPath) {
        if (onLoadProject_) {
          onLoadProject_(projectPath);
        }
      },
      [this]() {
        if (onNewProject_) {
          onNewProject_();
        }
      },
      [this]() {
        if (hubComponent) {
          hubComponent->setVisible(false);
        }
      });
  addAndMakeVisible(hubComponent.get());
  hubComponent->show();

  ZENITH_LOG_INFO("MainComponent Constructor COMPLETE");
}

MainComponent::~MainComponent() {
  removeKeyListener(this);
}

bool MainComponent::keyPressed(const juce::KeyPress &key,
                               Component *originatingComponent) {
  juce::ignoreUnused(originatingComponent);

  if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown() &&
      !key.getModifiers().isShiftDown() && projectState.canUndo()) {
    projectState.undo();
    return true;
  }

  if (key.getTextCharacter() == 'Z' && key.getModifiers().isCommandDown() &&
      key.getModifiers().isShiftDown() && projectState.canRedo()) {
    projectState.redo();
    return true;
  }

  if (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M') {
    if (bottomBar) {
      bottomBar->setKeyboardVisible(!bottomBar->isKeyboardVisible());
      resized();
    }
    return true;
  }

  if (key == juce::KeyPress::tabKey && transportBar && transportBar->onViewToggleClicked) {
    transportBar->onViewToggleClicked();
    return true;
  }

  return false;
}

void MainComponent::paint(juce::Graphics &g) {
  SkiaMainWindowIntegration::paint(g);
}

void MainComponent::drawSkiaContent(SkCanvas *canvas) {
  canvas->clear(zenith::design::colors::BG_DARKEST);

  auto drawChild = [&](juce::Component *child,
                       zenith::SkiaComponent *skiaChild) {
    if (child && child->isVisible() && skiaChild) {
      canvas->save();
      auto bounds = child->getBounds();
      canvas->translate((float)bounds.getX(), (float)bounds.getY());
      canvas->clipRect(SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()));
      skiaChild->drawSkia(canvas);
      canvas->restore();
    }
  };

  drawChild(transportBar.get(), transportBar.get());
  drawChild(mainLayout.get(), mainLayout.get());
  drawChild(rightSidePanel.get(), rightSidePanel.get());
  drawChild(bottomBar.get(), bottomBar.get());
  drawChild(hubComponent.get(), hubComponent.get());
}

void MainComponent::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isPopupMenu()) {
    juce::PopupMenu m;
    m.addItem("Show Debug Logs", [] { DBG("Debug logs requested"); });
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(nullptr), nullptr);
  }
}

void MainComponent::mouseDrag(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void MainComponent::mouseUp(const juce::MouseEvent &e) {
  activeDragComponent = nullptr;
}

void MainComponent::resized() {
  auto bounds = getLocalBounds();

  if (transportBar) {
    transportBar->setBounds(bounds.removeFromTop(60));
  }

  if (bottomBar) {
    bottomBar->setBounds(bounds.removeFromBottom(128));
  }

  if (rightSidePanel) {
    rightSidePanel->setBounds(bounds.removeFromRight(400));
  }

  if (mainLayout) {
    mainLayout->setBounds(bounds);
  }

  if (hubComponent) {
    hubComponent->setBounds(getLocalBounds());
  }
}

void MainComponent::openPianoRoll(const juce::String &trackId,
                                  const juce::String &clipId) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  new PianoRollWindow(projectState, engine, trackId, clipId);
}

void MainComponent::handleImportAudio() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto chooser = std::make_shared<juce::FileChooser>(
      "Import Audio File", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
      "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg");

  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(
      chooserFlags, [this, chooser](const juce::FileChooser &fc) {
        auto file = fc.getResult();
        if (!file.existsAsFile()) return;

        if (engine.getNumTracks() == 0) {
          engine.addTestTracks(1);
        }

        const auto &tracks = engine.tracks();
        if (tracks.empty()) return;

        auto *track = tracks[0].get();
        if (track == nullptr) return;

        auto clip = std::make_unique<zenith::Clip>();
        clip->setType(zenith::Clip::Type::Audio);
        clip->setName(file.getFileNameWithoutExtension());

        auto &pool = engine.getAudioFilePool();
        clip->setAudioFileFromPool(file, pool);
        clip->setStartPosition(0);
        clip->setPlaying(true);

        track->addClip(std::move(clip));
      });
}

//==============================================================================
// MainWindow Implementation
//==============================================================================

#include "../../engine/ProjectFileIO.h"

MainWindow::MainWindow(const juce::String &name)
    : DocumentWindow(
          name,
          juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
              juce::ResizableWindow::backgroundColourId),
          DocumentWindow::allButtons) {
  engine = std::make_unique<zenith::Engine>();
  projectState = std::make_unique<zenith::ProjectState>();
  
  fileIO_ = std::make_unique<zenith::ProjectFileIO>(*projectState);
  fileIO_->setAutoSaveInterval(300);
  fileIO_->setAutoSaveEnabled(true);
  fileIO_->setMaxBackups(10);
  startTimer(30000);

  automationSync = std::make_unique<zenith::TrackAutomationSynchronizer>(*projectState, *engine);
  commandAPI = std::make_unique<zenith::CommandAPI>(*projectState, *engine);
  engine->setProjectState(projectState.get());
  clipSynchronizer = std::make_unique<zenith::ClipSynchronizer>(*projectState, *engine);
  recentProjectManager_ = std::make_unique<zenith::RecentProjectManager>();

  mainComponent = std::make_unique<MainComponent>(
      *engine, *commandAPI, *projectState, *recentProjectManager_,
      [this](const juce::File &file) { loadProject(file); },
      [this]() { newProject(); });

  uxDirector = std::make_unique<ai::UXDirectorAgent>(*engine, *projectState, *mainComponent);
  commandAPI->setUXDirector(uxDirector.get());
  uxDirector->startMonitoring(500);

  presetGeneticist = std::make_unique<ai::PresetGeneticistAgent>();
  commandAPI->setPresetGeneticist(presetGeneticist.get());

  setUsingNativeTitleBar(true);
  setContentOwned(mainComponent.get(), true);

#if JUCE_IOS || JUCE_ANDROID
  setFullScreen(true);
#else
  setResizable(true, true);
  centreWithSize(getWidth(), getHeight());
#endif

  juce::Component::setVisible(true);
  engine->initialize();
  automationSync->start(60);
  
  checkForRecovery();
  updateWindowTitle();
}

MainWindow::~MainWindow() {
  stopTimer();
  if (engine) engine->shutdown();
  setContentOwned(nullptr, true);
}

void MainWindow::closeButtonPressed() {
  if (projectState->hasUnsavedChanges()) {
    int result = juce::NativeMessageBox::showYesNoCancelBox(
        juce::AlertWindow::WarningIcon,
        "Unsaved Changes",
        "Save changes before closing?");
    
    if (result == 1) { // Yes
      saveProject();
      // Wait for save? it's synchronous mostly except recent files
      // But if user cancels save?
    } else if (result == 0) { // Cancel
      return;
    }
    // Result 2 is No (discard)
  }

  juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::showAboutDialog() {
  juce::String aboutMessage;
  aboutMessage << "Zenith DAW\n\nVersion: 0.1.0\nBuilt with JUCE 8.0.9";
  juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "About Zenith DAW", aboutMessage, "OK");
}

void MainWindow::timerCallback() {
  if (fileIO_) fileIO_->autoSave();
}

void MainWindow::checkForRecovery() {
  if (!fileIO_) return;
  auto recoveries = fileIO_->getAvailableRecoveries();
  
  if (recoveries.empty()) return;
  
  juce::AlertWindow dialog("Project Recovery",
      "Zenith detected unsaved work from a previous session.",
      juce::AlertWindow::QuestionIcon);
  
  dialog.addButton("Recover Latest", 1, juce::KeyPress(juce::KeyPress::returnKey));
  dialog.addButton("Discard", 2);
  
  int result = dialog.showDialog();
  
  if (result == 1) {
    zenith::FileIOError error = fileIO_->recoverFromFile(recoveries.back().recoveryFile);
    if (error == zenith::FileIOError::Success) {
      updateWindowTitle();
      repaint();
    }
  }
}

void MainWindow::createManualBackup() {
  if (!fileIO_) return;
  juce::File backupFile = fileIO_->createBackup();
  
  if (backupFile.existsAsFile()) {
    juce::NativeMessageBox::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Backup Created",
        "Project backed up to:\n" + backupFile.getFullPathName());
  }
}

void MainWindow::updateWindowTitle() {
  juce::File projectFile = fileIO_->getCurrentProjectFile();
  juce::String title = "Zenith DAW";
  
  if (projectFile.existsAsFile()) {
    title += " - " + projectFile.getFileNameWithoutExtension();
  } else {
    title += " - [Untitled]";
  }
  
  if (projectState->hasUnsavedChanges()) {
    title += " *";
  }
  
  setName(title);
}

void MainWindow::newProject() {
  if (projectState->hasUnsavedChanges()) {
      int result = juce::NativeMessageBox::showYesNoCancelBox(
          juce::AlertWindow::WarningIcon,
          "Unsaved Changes",
          "Save changes before creating a new project?");
      
      if (result == 1) {
          saveProject();
      } else if (result == 0) {
          return;  // Cancel
      }
  }
  
  fileIO_->newProject();
  updateWindowTitle();
  repaint();
}

void MainWindow::saveProject() {
  juce::File projectFile = fileIO_->getCurrentProjectFile();
  
  if (!projectFile.existsAsFile()) {
    saveProjectAs();
    return;
  }
  
  zenith::FileIOError error = fileIO_->saveToFile(projectFile);
  
  if (error != zenith::FileIOError::Success) {
    juce::NativeMessageBox::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Save Failed",
        "Failed to save project: " + zenith::ProjectFileIO::getErrorMessage(error));
    return;
  }
  
  updateWindowTitle();
  if (recentProjectManager_) {
    recentProjectManager_->addProject(projectFile, projectState->getProjectName());
    recentProjectManager_->save();
  }
}

void MainWindow::saveProjectAs() {
  auto chooser = std::make_shared<juce::FileChooser>("Save Project As...", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.zth");
  auto chooserFlags = juce::FileBrowserComponent::saveMode |
                      juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file == juce::File{}) return;
    if (!file.hasFileExtension(".zth")) file = file.withFileExtension(".zth");

    zenith::FileIOError error = fileIO_->saveToFileAs(file);
    
    if (error != zenith::FileIOError::Success) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Failed",
            "Failed to save project: " + zenith::ProjectFileIO::getErrorMessage(error));
        return;
    }

    currentProjectFile = file;
    updateWindowTitle();
    if (recentProjectManager_) {
      recentProjectManager_->addProject(file, projectState->getProjectName());
      recentProjectManager_->save();
    }
  });
}

bool MainWindow::loadProject(const juce::File &file) {
  if (!file.existsAsFile()) return false;
  engine->stop();
  
  zenith::FileIOError error = fileIO_->loadFromFile(file);
        
  if (error != zenith::FileIOError::Success) {
      juce::NativeMessageBox::showMessageBoxAsync(
          juce::AlertWindow::WarningIcon,
          "Load Failed",
          "Failed to load project: " + 
          zenith::ProjectFileIO::getErrorMessage(error) +
          "\n\nDetails: " + fileIO_->getLastErrorDetails());
      return false;
  }
  
  currentProjectFile = file;
  if (recentProjectManager_) {
    recentProjectManager_->addProject(file, projectState->getProjectName());
    recentProjectManager_->save();
  }
  updateWindowTitle();
  repaint();
  return true;
}

void MainWindow::openProject() {
  auto chooser = std::make_shared<juce::FileChooser>("Open Project", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.zth");
  auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
  chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file != juce::File{}) loadProject(file);
  });
}