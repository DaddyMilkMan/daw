/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "MainWindow.h"
#include "../../commands/CommandAPI.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include "../engine/Clip.h"
#include "../engine/Track.h"
#include "../engine/MixerController.h"
#include <memory>
#include <cmath>
#include <utility>
#include <vector>
//
#include "../../network/MCPServer.h"
#include "../../ai/PresetGeneticistAgent.h"
#include "../engine/ZenithLogger.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "MainLayoutComponent.h"
#include "../transport/TransportBar.h"
#include "../collaboration/CollaborationPresenceBar.h"
#include "RightSidePanel.h"
#include "BottomBar.h"
#include "../../network/CollaborationManager.h"
#include "ZenithHubComponent.h"
#include "../dialogs/SettingsComponent.h"
#include "../project/ProjectManagerUISkia.h"
#include "../piano-roll/PianoRollComponent.h"
#include "../../ai/UXDirectorAgent.h"
#include <core/SkFont.h>
#include <core/SkImage.h>
#include <core/SkImageInfo.h>
#include <core/SkPixmap.h>
#include <core/SkSamplingOptions.h>
#include <core/SkSurface.h>
#include <core/SkTextBlob.h>
#include "../../engine/ProjectFileIO.h"
#include "../controls/SkiaAlertWindow.h"

namespace zenith {

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
  setName("MainComponent");
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
  ZENITH_LOG_INFO("-> Creating TransportBar...");
  
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

  transportBar->onLoopToggled = [this]() {
      bool loop = !engine.isLooping();
      engine.setLooping(loop);
      ZENITH_LOG_DEBUG("Looping toggled: " + juce::String(loop ? "ON" : "OFF"));
  };

  transportBar->onRewind = [this]() {
      engine.stop();
      engine.setPlayheadSamples(0);
      ZENITH_LOG_DEBUG("Rewound to 0");
  };

  transportBar->onClearAllSolos = [this]() {
      engine.getMixerController().clearAllSolos();
      ZENITH_LOG_DEBUG("Cleared all solos");
  };

  addAndMakeVisible(transportBar.get());
  
  // Collaboration Presence
  presenceBar = std::make_unique<zenith::CollaborationPresenceBar>();
  addAndMakeVisible(presenceBar.get());
  
  ZENITH_LOG_INFO("[OK] TransportBar created");
  DBG("MainComponent: TransportBar created");

  // The "Perfect DAW" Tri-Pane Layout Manager
  ZENITH_LOG_INFO("-> Creating MainLayoutComponent...");
  mainLayout =
      std::make_unique<zenith::MainLayoutComponent>(engine, projectState, api);

  addAndMakeVisible(mainLayout.get());
  ZENITH_LOG_INFO("[OK] MainLayoutComponent created");

  // Right: AI Assistant Panel (Wingman) - Pure Skia
  ZENITH_LOG_INFO("-> Creating RightSidePanel...");
  rightSidePanel = std::make_unique<zenith::RightSidePanel>(api, engine, state);
  addAndMakeVisible(rightSidePanel.get());
  ZENITH_LOG_INFO("[OK] RightSidePanel created");

  // Bottom: Piano Keyboard + Mixer Strip
  ZENITH_LOG_INFO("-> Creating BottomBar...");
  bottomBar = std::make_unique<zenith::BottomBar>(midiKeyboardState, engine,
                                                  projectState);
  bottomBar->setKeyboardVisible(false); // Hidden by default

  // Connect Session Debugger
  if (auto *debugger = engine.getSessionDebugger()) {
    bottomBar->setDebugger(debugger);
    ZENITH_LOG_INFO("[OK] Session Debugger connected to BottomBar");
  }

  addAndMakeVisible(bottomBar.get());
  ZENITH_LOG_INFO("[OK] BottomBar created");

  // Connect view toggle callback
  transportBar->onViewToggleClicked = [this]() {
    if (mainLayout) {
      mainLayout->toggleView();
      DBG("View toggled via MainLayout");
    }
  };

  // Connect settings callback
  transportBar->onSettingsClicked = [this]() { showWorkspaceOverlay(false); };

  // Create Zenith Hub with real project manager
  hubComponent = std::make_unique<zenith::ZenithHubComponent>(
      recentProjectManager_,
      [this](const juce::File &projectPath) {
        if (onLoadProject_) {
          onLoadProject_(projectPath);
        }
        setMainUiVisible(true);
      },
      [this]() {
        if (onNewProject_) {
          onNewProject_();
        }
        setMainUiVisible(true);
      },
      [this]() {
        if (hubComponent) {
          hubComponent->setVisible(false);
          setMainUiVisible(true);
        }
      });
  addAndMakeVisible(hubComponent.get());
  hubComponent->show();
  hubComponent->toFront(true);  // CRITICAL: Ensure hub is on top of all other components for z-order

  // Hide Main UI initially so Hub is exclusive
  setMainUiVisible(false);

  ZENITH_LOG_INFO("MainComponent Constructor COMPLETE");
  
  // Start timer for animations/updates
  animationTimer_ = std::make_unique<AnimationTimer>(*this);
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) animationTimer_->startTimerHz(60);
}

MainComponent::~MainComponent() {
  animationTimer_->stopTimer();
}

bool MainComponent::keyPressed(const juce::KeyPress &key, Component *originatingComponent) {
  // Handle global shortcuts like Undo/Redo
  if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
    projectState.undo();
    return true;
  }
  if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)) {
    projectState.redo();
    return true;
  }
  return false;
}

void MainComponent::handleAnimationTimer() {
  // Update animation time
  animationTime_ += 0.016f; // approx 60fps
  if (animationTime_ > 1000.0f) animationTime_ = 0.0f;
  
  // Trigger repaint via Skia
  triggerRepaint();
}

void MainComponent::paint(juce::Graphics &g) {
  SkiaMainWindowIntegration::paint(g);
}

void MainComponent::drawSkiaContent(SkCanvas *canvas) {
  // Amazing Wow Factor: Animated Aurora Background
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  aurora_.draw(canvas, skBounds, animationTime_);

  if ((settingsOverlay_ && settingsOverlay_->isVisible()) ||
      (projectManagerOverlay_ && projectManagerOverlay_->isVisible())) {
    SkPaint scrim;
    scrim.setColor(design::withAlpha(design::colors::BG_00, 0.72f));
    canvas->drawRect(skBounds, scrim);
  }

  // Get pointer to hubComponent once for comparison
  auto* hub = hubComponent.get();

  // Draw all children that are SkiaComponents, but SKIP hubComponent
  // We'll draw hubComponent last to ensure proper z-order (hub on top of everything)
  for (auto* child : getChildren()) {
    if (child == nullptr) continue;
    if (!child->isVisible()) continue;
    if (child == hub) continue;  // Skip hub, we draw it last
    
    if (auto* skiaChild = dynamic_cast<zenith::SkiaComponent*>(child)) {
      canvas->save();
      auto childBounds = child->getBounds();
      canvas->translate((float)childBounds.getX(), (float)childBounds.getY());
      canvas->clipRect(
          SkRect::MakeWH((float)childBounds.getWidth(), (float)childBounds.getHeight()));
      skiaChild->drawSkia(canvas);
      canvas->restore();
    }
  }

  // CRITICAL: Always draw hubComponent LAST to ensure it's on top of everything
  if (hub != nullptr && hub->isVisible()) {
    canvas->save();
    auto childBounds = hub->getBounds();
    canvas->translate((float)childBounds.getX(), (float)childBounds.getY());
    canvas->clipRect(
        SkRect::MakeWH((float)childBounds.getWidth(), (float)childBounds.getHeight()));
    hub->drawSkia(canvas);
    canvas->restore();
  }
}

void MainComponent::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isPopupMenu()) {
    juce::PopupMenu m;
    m.addItem("Show Debug Logs", [] { DBG("Debug logs requested"); });
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(nullptr),
                    nullptr);
  }
}

void MainComponent::mouseDrag(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void MainComponent::mouseUp(const juce::MouseEvent &e) {
  activeDragComponent = nullptr;
}

void MainComponent::parentHierarchyChanged() {
  DBG("MainComponent::parentHierarchyChanged called, peer=" << (getPeer() != nullptr ? "valid" : "null"));
  ZENITH_LOG_INFO("MainComponent::parentHierarchyChanged called");
  SkiaMainWindowIntegration::parentHierarchyChanged();
}

void MainComponent::setMainUiVisible(bool shouldBeVisible) {
  if (transportBar) transportBar->setVisible(shouldBeVisible);
  if (mainLayout) mainLayout->setVisible(shouldBeVisible);
  if (rightSidePanel) rightSidePanel->setVisible(shouldBeVisible);
  if (bottomBar) bottomBar->setVisible(shouldBeVisible);
  
  repaint();
}

void MainComponent::showWorkspaceOverlay(bool showProjectsTab) {
  if (!settingsOverlay_) {
    settingsOverlay_ = std::make_unique<zenith::SettingsComponent>(engine);
    addAndMakeVisible(settingsOverlay_.get());
  }

  if (!projectManagerOverlay_) {
    projectManagerOverlay_ = std::make_unique<zenith::ui::ProjectManagerUISkia>();
    projectManagerOverlay_->onOpenProject = [this](const juce::File &path) {
      if (onLoadProject_) {
        onLoadProject_(path);
      }
      dismissWorkspaceOverlay();
      setMainUiVisible(true);
    };
    projectManagerOverlay_->onNewProject = [this]() {
      if (onNewProject_) {
        onNewProject_();
      }
      dismissWorkspaceOverlay();
      setMainUiVisible(true);
    };
    projectManagerOverlay_->onDeleteProject = [this](const juce::File &path) {
      recentProjectManager_.removeProject(path);
      recentProjectManager_.save();
      refreshProjectManagerOverlay();
    };
    projectManagerOverlay_->onSaveRequested = [this]() {
      recentProjectManager_.save();
    };
    addAndMakeVisible(projectManagerOverlay_.get());
  }

  if (!overlayCloseButton_) {
    overlayCloseButton_ = std::make_unique<zenith::SkiaButton>("Close");
    overlayCloseButton_->setStyle(zenith::SkiaButton::Style::Secondary);
    overlayCloseButton_->onClick = [this]() { dismissWorkspaceOverlay(); };
    addAndMakeVisible(overlayCloseButton_.get());
  }

  if (!overlaySettingsTabButton_) {
    overlaySettingsTabButton_ = std::make_unique<zenith::SkiaButton>("Settings");
    overlaySettingsTabButton_->onClick = [this]() { showWorkspaceOverlay(false); };
    addAndMakeVisible(overlaySettingsTabButton_.get());
  }

  if (!overlayProjectsTabButton_) {
    overlayProjectsTabButton_ = std::make_unique<zenith::SkiaButton>("Projects");
    overlayProjectsTabButton_->onClick = [this]() { showWorkspaceOverlay(true); };
    addAndMakeVisible(overlayProjectsTabButton_.get());
  }

  refreshProjectManagerOverlay();
  settingsOverlay_->setVisible(!showProjectsTab);
  projectManagerOverlay_->setVisible(showProjectsTab);
  overlayCloseButton_->setVisible(true);
  overlaySettingsTabButton_->setVisible(true);
  overlayProjectsTabButton_->setVisible(true);
  overlaySettingsTabButton_->setStyle(showProjectsTab ? zenith::SkiaButton::Style::Ghost
                                                      : zenith::SkiaButton::Style::Primary);
  overlayProjectsTabButton_->setStyle(showProjectsTab ? zenith::SkiaButton::Style::Primary
                                                      : zenith::SkiaButton::Style::Ghost);

  settingsOverlay_->toFront(false);
  projectManagerOverlay_->toFront(false);
  overlaySettingsTabButton_->toFront(false);
  overlayProjectsTabButton_->toFront(false);
  overlayCloseButton_->toFront(false);
  resized();
}

void MainComponent::dismissWorkspaceOverlay() {
  if (settingsOverlay_) {
    settingsOverlay_->setVisible(false);
  }
  if (projectManagerOverlay_) {
    projectManagerOverlay_->setVisible(false);
  }
  if (overlayCloseButton_) {
    overlayCloseButton_->setVisible(false);
  }
  if (overlaySettingsTabButton_) {
    overlaySettingsTabButton_->setVisible(false);
  }
  if (overlayProjectsTabButton_) {
    overlayProjectsTabButton_->setVisible(false);
  }
  repaint();
}

void MainComponent::refreshProjectManagerOverlay() {
  if (!projectManagerOverlay_) {
    return;
  }

  const auto recent = recentProjectManager_.getRecentProjects(true);
  std::vector<zenith::ui::ProjectManagerUISkia::ProjectRow> rows;
  rows.reserve(recent.size());
  for (const auto &entry : recent) {
    rows.push_back({entry.path.getFullPathName(), entry.name,
                    entry.path.getFullPathName(),
                    entry.getRelativeTimeString()});
  }
  projectManagerOverlay_->setProjects(rows);
}

void MainComponent::visibilityChanged() {
  DBG("MainComponent::visibilityChanged called, visible=" << (isVisible() ? "yes" : "no"));
  ZENITH_LOG_INFO("MainComponent::visibilityChanged called");
  SkiaMainWindowIntegration::visibilityChanged();
}

void MainComponent::resized() {
  auto bounds = getLocalBounds();

  if (transportBar) {
    auto tBounds = bounds.removeFromTop(60);
    transportBar->setBounds(tBounds);
    if (presenceBar) {
        presenceBar->setBounds(tBounds.removeFromRight(200).withTrimmedTop(14).withTrimmedBottom(14));
    }
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

  const bool overlayVisible =
      (settingsOverlay_ && settingsOverlay_->isVisible()) ||
      (projectManagerOverlay_ && projectManagerOverlay_->isVisible());
  if (overlayVisible) {
    const int overlayW =
        juce::jlimit(760, 1300, (int)std::round((double)getWidth() * 0.84));
    const int overlayH =
        juce::jlimit(520, 840, (int)std::round((double)getHeight() * 0.82));
    juce::Rectangle<int> overlayBounds((getWidth() - overlayW) / 2,
                                       (getHeight() - overlayH) / 2, overlayW,
                                       overlayH);

    if (settingsOverlay_) {
      settingsOverlay_->setBounds(overlayBounds);
    }
    if (projectManagerOverlay_) {
      projectManagerOverlay_->setBounds(overlayBounds);
    }

    const int tabH = 34;
    const int tabW = 112;
    const int y = overlayBounds.getY() - tabH - 8;
    if (overlaySettingsTabButton_) {
      overlaySettingsTabButton_->setBounds(overlayBounds.getX(), y, tabW, tabH);
    }
    if (overlayProjectsTabButton_) {
      overlayProjectsTabButton_->setBounds(overlayBounds.getX() + tabW + 8, y,
                                           tabW, tabH);
    }
    if (overlayCloseButton_) {
      overlayCloseButton_->setBounds(overlayBounds.getRight() - 92, y, 92, tabH);
    }
  }

  if (importFileChooser_) {
    const int w =
        juce::jlimit(560, 1060, (int)std::round((double)getWidth() * 0.78));
    const int h =
        juce::jlimit(380, 760, (int)std::round((double)getHeight() * 0.76));
    importFileChooser_->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2,
                                  w, h);
  }
}

void MainComponent::openPianoRoll(const juce::String &trackId,
                                  const juce::String &clipId) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  new PianoRollWindow(projectState, engine, trackId, clipId);
}

void MainComponent::handleImportAudio() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  dismissImportChooser();
  importFileChooser_ = std::make_unique<zenith::SkiaFileChooser>(
      "Import Audio File",
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
      "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg",
      zenith::SkiaFileChooser::Mode::OpenFile);

  const int w =
      juce::jlimit(560, 1060, (int)std::round((double)getWidth() * 0.78));
  const int h =
      juce::jlimit(380, 760, (int)std::round((double)getHeight() * 0.76));
  importFileChooser_->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w,
                                h);
  addAndMakeVisible(importFileChooser_.get());
  importFileChooser_->toFront(true);

  importFileChooser_->showAsync(
      [this](zenith::SkiaFileChooser::Result result, const juce::File &file) {
        if (result == zenith::SkiaFileChooser::Result::Approved &&
            file.existsAsFile()) {
          if (engine.getNumTracks() == 0) {
            engine.addTestTracks(1);
          }

          const auto &tracks = engine.tracks();
          if (!tracks.empty()) {
            auto *track = tracks[0].get();
            if (track != nullptr) {
              auto clip = std::make_unique<zenith::Clip>();
              clip->setType(zenith::Clip::Type::Audio);
              clip->setName(file.getFileNameWithoutExtension());

              auto &pool = engine.getAudioFilePool();
              clip->setAudioFileFromPool(file, pool);
              clip->setStartPosition(0);
              clip->setPlaying(true);
              track->addClip(std::move(clip));
            }
          }
        }
        dismissImportChooser();
      });
}

void MainComponent::dismissImportChooser() {
  if (importFileChooser_) {
    removeChildComponent(importFileChooser_.get());
    importFileChooser_.reset();
  }
}

//==============================================================================
// MainWindow Implementation
//==============================================================================

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
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(30000);

  automationSync = std::make_unique<zenith::TrackAutomationSynchronizer>(
      *projectState, *engine);
  commandAPI = std::make_unique<zenith::CommandAPI>(*projectState, *engine);
  engine->setProjectState(projectState.get());
  clipSynchronizer =
      std::make_unique<zenith::ClipSynchronizer>(*projectState, *engine);
  recentProjectManager_ = std::make_unique<zenith::RecentProjectManager>();

  // Apply Global LookAndFeel
  lookAndFeel = std::make_unique<zenith::ZenithLookAndFeel>();
  juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());

  // Initialize Native CRDT Collaboration
  CollaborationManager::getInstance().initializeCRDT(projectState->getState());

  mainComponent = std::make_unique<MainComponent>(
      *engine, *commandAPI, *projectState, *recentProjectManager_,
      [this](const juce::File &file) { loadProject(file); },
      [this]() { newProject(); });
  
  // TEMPORARILY DISABLED for debugging constructor completion:
  // uxDirector = std::make_unique<ai::UXDirectorAgent>(*engine, *projectState,
  //                                                    *mainComponent);
  // commandAPI->setUXDirector(uxDirector.get());
  // uxDirector->startMonitoring(500);

  // presetGeneticist = std::make_unique<ai::PresetGeneticistAgent>();
  // commandAPI->setPresetGeneticist(presetGeneticist.get());

  // mcpServer = std::make_unique<zenith::mcp::MCPServer>(
  //     *commandAPI, *projectState, *engine, this);
  // mcpServer->start();

  setUsingNativeTitleBar(true);
  
  // CRITICAL: Set window size BEFORE adding content
  // This ensures the content component gets proper bounds
  constexpr int defaultWidth = 1400;
  constexpr int defaultHeight = 800;
  
#if JUCE_IOS || JUCE_ANDROID
  setFullScreen(true);
#else
  setResizable(true, true);
  setResizeLimits(800, 600, 4096, 2160); // Min and max sizes
  
  // Set the DocumentWindow size FIRST
  setBounds(100, 100, defaultWidth, defaultHeight);
#endif

  // Set the content component size before adding it
  mainComponent->setSize(defaultWidth - 2, defaultHeight - getTitleBarHeight() - 2);
  
  // Now add the content component  
  setContentOwned(mainComponent.get(), false); // false = don't resize to content
  
  // Ensure window is centered
  centreWithSize(getWidth(), getHeight());

  ZENITH_LOG_INFO("MainWindow: Window sized to " + std::to_string(getWidth()) + "x" + std::to_string(getHeight()));
  ZENITH_LOG_INFO("MainWindow: MainComponent size: " + std::to_string(mainComponent->getWidth()) + "x" + std::to_string(mainComponent->getHeight()));

  // Make visible - this will trigger peer creation and OpenGL context attachment
  juce::Component::setVisible(true);
  ZENITH_LOG_INFO("MainWindow: setVisible(true) called");
  
  // Force OpenGL context attachment now that the window is visible
  // The MainComponent inherits from SkiaMainWindowIntegration which has OpenGL
  if (mainComponent && mainComponent->getPeer()) {
    ZENITH_LOG_INFO("MainWindow: Manually calling attachContextNow on MainComponent");
    mainComponent->attachContextNow();
  } else {
    ZENITH_LOG_INFO("MainWindow: WARNING: MainComponent has no peer after setVisible!");
  }
  
  engine->initialize();
  automationSync->start(60);

  checkForRecovery();
  updateWindowTitle();
}

MainWindow::~MainWindow() {
  ZENITH_LOG_INFO("MainWindow::Destructor STARTED");
  stopTimer();
  dismissWindowOverlays();
  
  if (automationSync) {
      ZENITH_LOG_INFO("MainWindow: Stopping redundant automationSync...");
      automationSync->stop();
      automationSync.reset();
  }

  if (engine) {
    ZENITH_LOG_INFO("MainWindow: Shutting down engine...");
    engine->shutdown();
  }
  
  ZENITH_LOG_INFO("MainWindow: Resetting mainComponent...");
  setContentOwned(nullptr, true);
  
  ZENITH_LOG_INFO("MainWindow::Destructor COMPLETE");
}

void MainWindow::dismissWindowOverlays() {
  if (mainComponent) {
    if (activeWindowAlert_) {
      mainComponent->removeChildComponent(activeWindowAlert_.get());
    }
    if (activeWindowFileChooser_) {
      mainComponent->removeChildComponent(activeWindowFileChooser_.get());
    }
  }
  activeWindowAlert_.reset();
  activeWindowFileChooser_.reset();
}

void MainWindow::showWindowAlert(
    const juce::String &title, const juce::String &message,
    zenith::SkiaAlertWindow::IconType iconType, const juce::String &button1,
    const juce::String &button2, const juce::String &button3,
    std::function<void(zenith::SkiaAlertWindow::Result)> callback) {
  if (!mainComponent) {
    return;
  }

  dismissWindowOverlays();
  activeWindowAlert_ =
      std::make_unique<zenith::SkiaAlertWindow>(title, message, iconType);
  activeWindowAlert_->addButton(button1, zenith::SkiaAlertWindow::Result::Button1,
                                zenith::SkiaButton::Style::Primary);
  if (button2.isNotEmpty()) {
    activeWindowAlert_->addButton(button2,
                                  zenith::SkiaAlertWindow::Result::Button2,
                                  zenith::SkiaButton::Style::Secondary);
  }
  if (button3.isNotEmpty()) {
    activeWindowAlert_->addButton(button3,
                                  zenith::SkiaAlertWindow::Result::Button3,
                                  zenith::SkiaButton::Style::Ghost);
  }

  const int w =
      juce::jlimit(420, 760, (int)std::round((double)getWidth() * 0.46));
  const int h =
      juce::jlimit(220, 420, (int)std::round((double)getHeight() * 0.34));
  activeWindowAlert_->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w,
                                h);
  mainComponent->addAndMakeVisible(activeWindowAlert_.get());
  activeWindowAlert_->toFront(true);
  activeWindowAlert_->showAsync([this, callback](zenith::SkiaAlertWindow::Result result) {
    dismissWindowOverlays();
    if (callback) {
      callback(result);
    }
  });
}

void MainWindow::showProjectFileChooser(
    const juce::String &title, zenith::SkiaFileChooser::Mode mode,
    std::function<void(zenith::SkiaFileChooser::Result, const juce::File &)>
        callback) {
  if (!mainComponent) {
    return;
  }

  dismissWindowOverlays();
  activeWindowFileChooser_ = std::make_unique<zenith::SkiaFileChooser>(
      title, juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
      "*.zth", mode);

  const int w =
      juce::jlimit(560, 1100, (int)std::round((double)getWidth() * 0.80));
  const int h =
      juce::jlimit(380, 760, (int)std::round((double)getHeight() * 0.76));
  activeWindowFileChooser_->setBounds((getWidth() - w) / 2,
                                      (getHeight() - h) / 2, w, h);
  mainComponent->addAndMakeVisible(activeWindowFileChooser_.get());
  activeWindowFileChooser_->toFront(true);

  activeWindowFileChooser_->showAsync(
      [this, callback](zenith::SkiaFileChooser::Result result,
                       const juce::File &file) {
        dismissWindowOverlays();
        if (callback) {
          callback(result, file);
        }
      });
}

void MainWindow::closeButtonPressed() {
  if (projectState == nullptr) {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
    return;
  }

  if (!projectState->hasUnsavedChanges()) {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
    return;
  }

  showWindowAlert(
      "Unsaved Changes", "Save changes before closing?",
      zenith::SkiaAlertWindow::IconType::WarningIcon, "Save", "Don't Save",
      "Cancel", [this](zenith::SkiaAlertWindow::Result result) {
        if (result == zenith::SkiaAlertWindow::Result::Button1) {
      saveProject([this](bool success) {
        if (success) {
          juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
      });
        } else if (result == zenith::SkiaAlertWindow::Result::Button2) {
          juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
      });
}


void MainWindow::showAboutDialog() {
  juce::String aboutMessage;
  aboutMessage << "Zenith DAW\n\nVersion: 0.1.0\nBuilt with JUCE 8.0.9";
  showWindowAlert("About Zenith DAW", aboutMessage,
                  zenith::SkiaAlertWindow::IconType::InfoIcon, "OK");
}

void MainWindow::timerCallback() {
  if (fileIO_)
    fileIO_->autoSave();
}

void MainWindow::checkForRecovery() {
  if (!fileIO_)
    return;
  
  auto recoveries = fileIO_->getAvailableRecoveries();

  if (recoveries.empty())
    return;

  showWindowAlert(
      "Project Recovery",
      "Zenith detected unsaved work from a previous session.\n\nWould you like to recover it?",
      zenith::SkiaAlertWindow::IconType::QuestionIcon, "Recover", "Skip", {},
      [this, recoveries](zenith::SkiaAlertWindow::Result result) {
        if (result != zenith::SkiaAlertWindow::Result::Button1) {
          return;
        }

        FileIOError error =
            fileIO_->recoverFromFile(recoveries.back().recoveryFile);
        if (error == FileIOError::Success) {
          updateWindowTitle();
          repaint();
        } else {
          showWindowAlert("Recovery Failed",
                          "Failed to recover the project. The backup may be corrupted.",
                          zenith::SkiaAlertWindow::IconType::WarningIcon, "OK");
        }
      });
}

void MainWindow::createManualBackup() {
  if (!fileIO_)
    return;
  juce::File backupFile = fileIO_->createBackup();

  if (backupFile.existsAsFile()) {
    showWindowAlert("Backup Created",
                    "Project backed up to:\n" + backupFile.getFullPathName(),
                    zenith::SkiaAlertWindow::IconType::InfoIcon, "OK");
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
    showWindowAlert(
        "Unsaved Changes", "Save changes before creating a new project?",
        zenith::SkiaAlertWindow::IconType::WarningIcon, "Save", "Discard",
        "Cancel", [this](zenith::SkiaAlertWindow::Result result) {
          if (result == zenith::SkiaAlertWindow::Result::Button1) {
            saveProject([this](bool success) {
              if (success) {
                fileIO_->newProject();
                updateWindowTitle();
                repaint();
              }
            });
          } else if (result == zenith::SkiaAlertWindow::Result::Button2) {
            fileIO_->newProject();
            updateWindowTitle();
            repaint();
          }
        });
    return;
  }

  fileIO_->newProject();
  updateWindowTitle();
  repaint();
}

void MainWindow::saveProject(std::function<void(bool)> onComplete) {
  juce::File projectFile = fileIO_->getCurrentProjectFile();

  if (!projectFile.existsAsFile()) {
    saveProjectAs(onComplete);
    return;
  }

  // Use async save to keep UI responsive
  fileIO_->saveToFileAsync(projectFile, {}, [this, projectFile, onComplete](bool success, juce::String error) {
    if (!success) {
        showWindowAlert("Save Failed", "Failed to save project: " + error,
                        zenith::SkiaAlertWindow::IconType::WarningIcon, "OK");
        if (onComplete) onComplete(false);
        return;
    }

    updateWindowTitle();
    if (recentProjectManager_) {
      recentProjectManager_->addProject(projectFile,
                                        projectState->getProjectName());
      recentProjectManager_->save();
    }
    if (onComplete) onComplete(true);
  });
}

void MainWindow::saveProjectAs(std::function<void(bool)> onComplete) {
  showProjectFileChooser(
      "Save Project As...", zenith::SkiaFileChooser::Mode::SaveFile,
      [this, onComplete](zenith::SkiaFileChooser::Result result,
                         const juce::File &selectedFile) {
        if (result != zenith::SkiaFileChooser::Result::Approved ||
            selectedFile == juce::File{}) {
          if (onComplete) onComplete(false);
          return;
        }

        auto file = selectedFile;
        if (!file.hasFileExtension(".zth")) {
          file = file.withFileExtension(".zth");
        }

        fileIO_->saveToFileAsync(
            file, {},
            [this, file, onComplete](bool success, juce::String error) {
              if (success) {
                updateWindowTitle();
                if (recentProjectManager_) {
                  recentProjectManager_->addProject(file,
                                                    projectState->getProjectName());
                  recentProjectManager_->save();
                }
                if (onComplete) onComplete(true);
              } else {
                showWindowAlert("Save Failed", error,
                                zenith::SkiaAlertWindow::IconType::WarningIcon,
                                "OK");
                if (onComplete) onComplete(false);
              }
            });
      });
}
bool MainWindow::loadProject(const juce::File &file) {
  if (!file.existsAsFile())
    return false;

  engine->stop();

  fileIO_->loadFromFileAsync(file, [this, file](bool success, juce::String error) {
      if (!success) {
        showWindowAlert("Load Failed", "Failed to load project: " + error,
                        zenith::SkiaAlertWindow::IconType::WarningIcon, "OK");
        return;
      }

      if (recentProjectManager_) {
        recentProjectManager_->addProject(file, projectState->getProjectName());
        recentProjectManager_->save();
      }
      updateWindowTitle();
      repaint();
  });

  return true;
}

void MainWindow::openProject() {
  showProjectFileChooser(
      "Open Project", zenith::SkiaFileChooser::Mode::OpenFile,
      [this](zenith::SkiaFileChooser::Result result, const juce::File &file) {
        if (result == zenith::SkiaFileChooser::Result::Approved &&
            file != juce::File{}) {
          loadProject(file);
        }
      });
}

} // namespace zenith
