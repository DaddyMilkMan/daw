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

 * @file MainWindow.cpp
 * @brief Main window implementation
 */


#include "engine/ZenithLogger.h"
#include "network/MCPServer.h"
#include "network/EmbeddedMCPHttpServer.h"
#include "network/UpdateService.h"
#include "ui/framework/GlassmorphicPanel.h"
#include "utils/PlatformSystemUtils.h"
#include "commands/CommandAPI.h"
#include "ui/dialogs/ExportDialog.h"
#include "ui/settings/ModernSettingsPanel.h"
#include "ui/dialogs/ProjectRecoveryModal.h"
#include "ui/dialogs/UnsavedChangesModal.h"

// AI Agents
#include "ai/UXDirectorAgent.h"
#include "ai/PresetGeneticistAgent.h"
#include "ZenithHubComponent.h"
#include "RightSidePanel.h"
#include "WingmanPanel.h"
#include "TitleBarComponent.h"
#include "../framework/PlatformWindowUtils.h"
#include <memory>

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
  setOpaque(true);
  setVisible(true);

  
  // Create Zenith Hub with real project manager
  hubComponent = std::make_unique<zenith::ZenithHubComponent>(
      recentProjectManager_,
      [this](const juce::File &projectPath) {
        if (onLoadProject_) {
          onLoadProject_(projectPath);
        }
        // Visibility is now handled by onLoadProject_ (MainWindow::loadProject)
        // after async load completes to prevent crashes
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
  // hubComponent = std::make_unique<zenith::ZenithHubComponent>(...);
  addAndMakeVisible(hubComponent.get());

  // Create Title Bar
  titleBar = std::make_unique<TitleBarComponent>();
  addAndMakeVisible(titleBar.get());
  
  // Wire up Menu Bar Callbacks
  auto& menu = titleBar->getMenuBar();
  menu.onNewProject = [this] { if (onNewProject_) onNewProject_(); };
  menu.onOpenProject = [this] { if (onOpenProjectRequest) onOpenProjectRequest(); };
  menu.onSaveProject = [this] { if (onSaveProjectRequest) onSaveProjectRequest(); };
  menu.onSaveProjectAs = [this] { if (onSaveProjectAsRequest) onSaveProjectAsRequest(); };
  menu.onToggleView = [this] {
      if (!newUILayout) return;
      auto current = newUILayout->getActiveView();
      newUILayout->setActiveView(
          current == ui::ViewType::Arrangement ? ui::ViewType::Session : ui::ViewType::Arrangement);
  };
  menu.onToggleWingman = [this] { toggleWingman(); };
  menu.onToggleSettings = [this] { toggleSettingsPanel(); };
  menu.onZoomIn = [this] { if (newUILayout) newUILayout->zoomIn(); };
  menu.onZoomOut = [this] { if (newUILayout) newUILayout->zoomOut(); };
  menu.onZoomToFit = [this] { if (newUILayout) newUILayout->zoomToFit(); };
  
  menu.onExportAudio = [&api, this] {
      // Trigger export dialog
      exportDialog = std::make_unique<ExportDialog>(api);
      addAndMakeVisible(exportDialog.get());
      exportDialog->setBounds(getLocalBounds());
      exportDialog->setVisible(true);
  };
  
  menu.onUndo = [this] { if (onUndoRequest) onUndoRequest(); };
  menu.onRedo = [this] { if (onRedoRequest) onRedoRequest(); };
  
  updateService_ = std::make_unique<zenith::network::UpdateService>();
  updateService_->checkForUpdates([this](const zenith::network::UpdateService::UpdateInfo& info) {
      if (info.available) {
          if (titleBar) {
              titleBar->getMenuBar().setUpdateAvailable(true);
          }
          if (newUILayout) {
              if (auto* transport = newUILayout->getTransportBar()) {
                  transport->setUpdateAvailable(true);
              }
          }
      }
  });

  titleBar->onClose = [this] {
      if (auto* app = juce::JUCEApplication::getInstance())
          app->systemRequestedQuit(); 
  };
  titleBar->onMinimize = [this] {
      if (auto* peer = getPeer()) peer->setMinimised(true);
  };
  titleBar->onMaximize = [this, isFullscreen = std::make_shared<bool>(false)]() mutable {
      // Use X11 true fullscreen - covers entire screen including taskbars
      *isFullscreen = !*isFullscreen;
      PlatformWindowUtils::setTrueFullscreen(this, *isFullscreen);
  };
  addAndMakeVisible(titleBar.get());
  
  // CRITICAL: Set hub bounds BEFORE showing it - it needs valid bounds for layout
  hubComponent->setBounds(getLocalBounds());
  hubComponent->show();
  hubComponent->toFront(true);

  // Create New Glassmorphism UI (views2) - WITH engine connection for real data
  newUILayout = std::make_unique<ui::ZenithMainLayout>(engine, projectState);
  addChildComponent(newUILayout.get());

  // Create RightSidePanel with Wingman (for new UI)
  rightSidePanel_ = std::make_unique<RightSidePanel>(api, engine, projectState);
  addChildComponent(rightSidePanel_.get());
  
  // Wire new UI transport bar to app-level actions
  if (auto* transport = newUILayout ? newUILayout->getTransportBar() : nullptr) {
      transport->onSettings = [this] { toggleSettingsPanel(); };
      transport->onWingman = [this] { toggleWingman(); };
      transport->setWingmanActive(wingmanVisible_);
  }

  // Ensure Top Bar is at the absolute front
  titleBar->toFront(false);
  // Create Export Dialog
  exportDialog = std::make_unique<ExportDialog>(api);
  addChildComponent(exportDialog.get());

  // Create Settings Panel
  settingsPanel = std::make_unique<ModernSettingsPanel>();
  addChildComponent(settingsPanel.get()); // Keep as child component since it's a modal
  
  // Set initial visibility
  exportDialog->setVisible(false);
  settingsPanel->setVisible(false);

  setMainUiVisible(false);

    
  // Start timer for animations/updates
  animationTimer_ = std::make_unique<AnimationTimer>(*this);
  // DISABLED FOR DEBUG: if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) animationTimer_->startTimerHz(60);
  
  // Optional embedded MCP HTTP server (start if MCP_HTTP_PORT env var set or auto-bind to available port 8090-8100)
  {
      const char* mcp_port_env = std::getenv("MCP_HTTP_PORT");
      const char* token_env = std::getenv("MCP_HTTP_TOKEN");
      if (!token_env) token_env = std::getenv("MCP_SERVER_TOKEN");
      juce::String token = token_env ? juce::String(token_env) : juce::String();

      auto tryStartPort = [&](int port)->bool {
          try {
              mcpHttpServer = std::make_unique<zenith::network::EmbeddedMCPHttpServer>(engine);
              if (!mcpHttpServer->start(port, "127.0.0.1", token)) {
                  mcpHttpServer.reset();
                  return false;
              }
              // Wait briefly for server to bind
              int waited = 0;
              while (waited < 500 && !mcpHttpServer->isRunning()) {
                  juce::Thread::sleep(50);
                  waited += 50;
              }
              if (mcpHttpServer->isRunning()) {
                                    return true;
              }
              mcpHttpServer->stop();
              mcpHttpServer.reset();
          } catch (...) {
                            mcpHttpServer.reset();
          }
          return false;
      };

      if (mcp_port_env != nullptr) {
          int port = atoi(mcp_port_env);
          (void)tryStartPort(port);
      } else {
          // auto-select from 49152..49162 (ephemeral/dynamic range)
          for (int p = 49152; p <= 49162; ++p) {
              if (tryStartPort(p)) break;
          }
      }
  }
}

MainComponent::~MainComponent() {
  animationTimer_->stopTimer();
  if (mcpHttpServer) {
      mcpHttpServer->stop();
      mcpHttpServer.reset();
  }
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
  
  // Wingman Toggle (Cmd+W)
  if (key == juce::KeyPress('w', juce::ModifierKeys::commandModifier, 0)) {
    toggleWingman();
    return true;
  }

  // Zoom shortcuts (Cmd/Ctrl +, -, 0)
  if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown()) {
    auto keyChar = key.getTextCharacter();
    if (keyChar == '+' || keyChar == '=') {
      if (newUILayout) newUILayout->zoomIn();
      return true;
    }
    if (keyChar == '-' || keyChar == '_') {
      if (newUILayout) newUILayout->zoomOut();
      return true;
    }
    if (keyChar == '0') {
      if (newUILayout) newUILayout->zoomToFit();
      return true;
    }
  }
  
  // View switching shortcuts (Cmd+1/2/3)
  if (key.getModifiers().isCommandDown()) {
    if (newUILayout) {
      if (key.getTextCharacter() == '1') {
        newUILayout->setActiveView(ui::ViewType::Arrangement);
        return true;
      }
      if (key.getTextCharacter() == '2') {
        newUILayout->setActiveView(ui::ViewType::Session);
        return true;
      }
      // Cmd+3 removed - AI Jam view will be integrated into Session view later
    }
  }
  
  // Forward to new UI layout for Tab/Shift+Tab view switching
  if (newUILayout && newUILayout->isVisible()) {
    return newUILayout->keyPressed(key, originatingComponent);
  }

  return false;
}

void MainComponent::handleAnimationTimer() {
  static int tickCount = 0;
    // Update animation time
  animationTime_ += 0.016f; // approx 60fps
  if (animationTime_ > 1000.0f) animationTime_ = 0.0f;
  
  // Trigger repaint via Skia
  triggerRepaint();
  
  // Update new UI layout (with ViewSwitcher)
  if (newUILayout && newUILayout->isVisible()) {
    newUILayout->setCPULoad(engine.getCpuUsage());
    newUILayout->setTempo(projectState.getTempo());
    newUILayout->setLoopEnabled(engine.isLooping());
    if (auto* transport = newUILayout->getTransportBar()) {
      transport->setTimeSignature(
          projectState.getTimeSignatureNumerator(),
          projectState.getTimeSignatureDenominator());
    }

    ui::TransportState transportState = ui::TransportState::Stopped;
    if (engine.isRecording()) {
      transportState = ui::TransportState::Recording;
    } else if (engine.isPlaying()) {
      transportState = ui::TransportState::Playing;
    }
    newUILayout->setPlayState(transportState);
    
    // Update arrangement view playhead position
    newUILayout->setPosition(engine.getPlaybackPositionBeats());
  }
  
  if (titleBar && hubComponent) {
      titleBar->setTransparentBackground(hubComponent->isVisible());
  }
}

void MainComponent::startAnimations() {
  if (animationTimer_ && !animationTimer_->isTimerRunning()) {
      ZENITH_LOG_INFO("MainComponent: Starting animation timer...");
      animationTimer_->startTimerHz(60);
  }
}

void MainComponent::paint(juce::Graphics &g) {
  SkiaMainWindowIntegration::paint(g);
}

void MainComponent::drawSkiaContent(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  auto* hub = hubComponent.get();
  
  if (hub != nullptr && hub->isVisible()) {
      // --- HUB MODE ---
      // 1. Animated Aurora Background (Fills the whole window)
      aurora_.draw(canvas, skBounds, animationTime_);
      
      // 2. Draw Hub Content
      // Since Hub is full-screen (0,0), we don't need translation
      hub->drawSkia(canvas);
      
      // 3. Draw Title Bar (Transparent) on top if visible
      if (titleBar && titleBar->isVisible()) {
          canvas->save();
          canvas->translate(titleBar->getX(), titleBar->getY());
          titleBar->drawSkia(canvas);
          canvas->restore();
      }
      
  } else {
      // --- MAIN DAW MODE ---
      // 1. Static Dark Background
      SkPaint bgPaint;
      bgPaint.setColor(SkColorSetARGB(255, 18, 18, 18));
      canvas->drawRect(skBounds, bgPaint);
      
      // 2. Draw New UI Layout (with ViewSwitcher containing Arrangement/Session)
      if (newUILayout && newUILayout->isVisible()) {
          canvas->save();
          canvas->translate(newUILayout->getX(), newUILayout->getY());
          canvas->clipRect(SkRect::MakeWH(newUILayout->getWidth(), newUILayout->getHeight()));
          newUILayout->drawSkia(canvas);
          canvas->restore();
      }
      
      // 4. Draw Wingman Panel (RightSidePanel)
      if (rightSidePanel_ && rightSidePanel_->isVisible()) {
          canvas->save();
          canvas->translate(rightSidePanel_->getX(), rightSidePanel_->getY());
          canvas->clipRect(SkRect::MakeWH(rightSidePanel_->getWidth(), rightSidePanel_->getHeight()));
          rightSidePanel_->drawSkia(canvas);
          canvas->restore();
      }
      
      // 5. Draw Top Bar Elements (drawn LAST so they're on top of everything)
      if (titleBar && titleBar->isVisible()) {
          canvas->save();
          canvas->translate(titleBar->getX(), titleBar->getY());
          canvas->clipRect(SkRect::MakeWH(titleBar->getWidth(), titleBar->getHeight()));
          titleBar->drawSkia(canvas);
          canvas->restore();
      }
      
  }
  
  // 4. Draw Modal Dialogs LAST (on top of everything)
  if (exportDialog && exportDialog->isVisible()) {
      canvas->save();
      canvas->translate(exportDialog->getX(), exportDialog->getY());
      exportDialog->drawSkia(canvas);
      canvas->restore();
  }
  
  if (settingsPanel && settingsPanel->isVisible()) {
      DBG("Drawing settings panel at: " + settingsPanel->getBounds().toString());
      canvas->save();
      canvas->translate(settingsPanel->getX(), settingsPanel->getY());
      settingsPanel->drawSkia(canvas);
      canvas->restore();
  }
}

void MainComponent::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
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
  if (newUILayout) {
      newUILayout->setVisible(shouldBeVisible);
      if (auto* transport = newUILayout->getTransportBar()) {
          transport->setWingmanActive(shouldBeVisible && wingmanVisible_);
      }
  }
  if (rightSidePanel_) {
      rightSidePanel_->setVisible(shouldBeVisible && wingmanVisible_);
  }
  
  // Hub should be visible when DAW is NOT visible
  if (hubComponent) {
      ZENITH_LOG_INFO("MainComponent::setMainUiVisible - Setting Hub visible=" + juce::String(!shouldBeVisible ? "true" : "false"));
      if (!shouldBeVisible) {
          hubComponent->show();
          hubComponent->toFront(true);
      } else {
          hubComponent->setVisible(false);
      }
  }
  
  if (titleBar) {
      titleBar->setVisible(shouldBeVisible);
      titleBar->setTransparentBackground(!shouldBeVisible);
      titleBar->setShowTitle(shouldBeVisible);
  }
  
  resized();
  repaint();
}

void MainComponent::toggleWingman() {
  wingmanVisible_ = !wingmanVisible_;
  if (rightSidePanel_) {
    rightSidePanel_->setVisible(wingmanVisible_);
  }
  if (newUILayout) {
    if (auto* transport = newUILayout->getTransportBar()) {
      transport->setWingmanActive(wingmanVisible_);
    }
  }
  // Trigger layout update
  resized();
  repaint();
}

void MainComponent::toggleSettingsPanel() {
  if (!settingsPanel) {
    DBG("Settings panel is null!");
    return;
  }

  bool isCurrentlyVisible = settingsPanel->isVisible();
  if (isCurrentlyVisible) {
    settingsPanel->setVisible(false);
  } else {
    settingsPanel->setVisible(true);
    settingsPanel->toFront(true);
    settingsPanel->repaint();
    resized();
    repaint();
  }
}

void MainComponent::visibilityChanged() {
  DBG("MainComponent::visibilityChanged called, visible=" << (isVisible() ? "yes" : "no"));
  ZENITH_LOG_INFO("MainComponent::visibilityChanged called");
  SkiaMainWindowIntegration::visibilityChanged();
}

void MainComponent::resized() {
  auto bounds = getLocalBounds();
  
  // Early exit if no size yet (can happen during initialization)
  if (bounds.isEmpty()) return;
  
  // Hub Mode check
  bool isHubVisible = hubComponent && hubComponent->isVisible();
  ZENITH_LOG_INFO(juce::String::formatted("MainComponent::resized() - bounds: %d x %d, isHubVisible: %s", 
                  bounds.getWidth(), bounds.getHeight(), isHubVisible ? "YES" : "NO"));
  
  // Layout Title Bar (EXACTLY 40px, no gap) - directly from bounds
  if (titleBar && titleBar->isVisible()) {
      titleBar->setBounds(bounds.removeFromTop(40));
  }
  
  // ALWAYS size components, even if hidden, to ensure layout transition is smooth
  if (hubComponent) {
      // Hub always wants full window bounds
      hubComponent->setBounds(getLocalBounds());
  }
  ZENITH_LOG_INFO("MainComponent::resized() - Hub bounds set");
  
  // New UI layout with Wingman panel on LEFT side (copilot position)
  if (newUILayout) {
    // Reserve space for Wingman panel on the left if visible
    if (rightSidePanel_ && wingmanVisible_) {
      const int wingmanWidth = juce::jmin(320, bounds.getWidth() / 3);  // Max 1/3 width
      auto mainBounds = bounds;
      auto wingmanBounds = mainBounds.removeFromLeft(wingmanWidth);

      newUILayout->setBounds(mainBounds);
      rightSidePanel_->setBounds(wingmanBounds);
    } else {
      newUILayout->setBounds(bounds);
      if (rightSidePanel_) rightSidePanel_->setBounds(bounds.withWidth(0));
    }
  }
  ZENITH_LOG_INFO("MainComponent::resized() - MainLayout bounds set");

  // Center Dialogs
  if (exportDialog) {
      exportDialog->centreWithSize(550, 520);
  }
  if (settingsPanel) {
      settingsPanel->centreWithSize(800, 600);
  }
  ZENITH_LOG_INFO("MainComponent::resized() - COMPLETE");
  
  // CRITICAL: Call base class to update OpenGL dimensions!
  SkiaMainWindowIntegration::resized();
}

void MainComponent::openPianoRoll(const juce::String &trackId,
                                  const juce::String &clipId) {
  // Piano roll disabled in simplified UI
  juce::ignoreUnused(trackId);
  juce::ignoreUnused(clipId);
}

//==============================================================================
// MainWindow Implementation
//==============================================================================

MainWindow::MainWindow(const juce::String &name)
    : DocumentWindow(
          name,
          juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
              juce::ResizableWindow::backgroundColourId),
          0) {  // 0 = no buttons from JUCE, prevents native WM decorations on Linux
  // DISABLE native title bar from the start - use Zenith custom title bar only
  setUsingNativeTitleBar(false);
  setResizable(true, true);
  engine = std::make_unique<zenith::Engine>();
  projectState = std::make_unique<zenith::ProjectState>();

  fileIO_ = std::make_unique<zenith::ProjectFileIO>(*projectState);
  fileIO_->setAutoSaveInterval(300);
  fileIO_->setAutoSaveEnabled(true);
  fileIO_->setMaxBackups(10);
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(30000);

  commandAPI = std::make_unique<zenith::CommandAPI>(*projectState, *engine);
  engine->setProjectState(projectState.get());
  recentProjectManager_ = std::make_unique<zenith::RecentProjectManager>();

  // Apply Global LookAndFeel only when Skia is disabled (legacy JUCE widgets)
#if !defined(ZENITH_USE_SKIA) || !ZENITH_USE_SKIA
  lookAndFeel = std::make_unique<zenith::ZenithLookAndFeel>();
  juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());
#endif

  mainComponent = std::make_unique<MainComponent>(
      *engine, *commandAPI, *projectState, *recentProjectManager_,
      [this](const juce::File &file) { loadProject(file); },
      [this]() { newProject(); });

  // Wire up callbacks from MainComponent's menu bar
  mainComponent->onOpenProjectRequest = [this] { openProject(); };
  mainComponent->onSaveProjectRequest = [this] { saveProject(); };
  mainComponent->onSaveProjectAsRequest = [this] { saveProjectAs(); };
  mainComponent->onUndoRequest = [this] { if (commandAPI) commandAPI->undo(); };
  mainComponent->onRedoRequest = [this] { if (commandAPI) commandAPI->redo(); };
  
  // Initialize Modal (Hidden)
  unsavedChangesModal_ = std::make_unique<UnsavedChangesModal>();
  unsavedChangesModal_->setVisible(false);
  addChildComponent(unsavedChangesModal_.get());
  
  // Re-enable AI agents
  uxDirector = std::make_unique<ai::UXDirectorAgent>(*engine, *projectState,
                                                     *mainComponent);
  commandAPI->setUXDirector(uxDirector.get());
  uxDirector->startMonitoring(500);

  presetGeneticist = std::make_unique<ai::PresetGeneticistAgent>();
  commandAPI->setPresetGeneticist(presetGeneticist.get());

  auto shouldStartMcpStdio = [](const char* value) -> bool {
    if (!value) return false;
    juce::String v(value);
    v = v.trim().toLowerCase();
    return v == "1" || v == "true" || v == "yes" || v == "on";
  };

  if (shouldStartMcpStdio(std::getenv("MCP_STDIO"))) {
    mcpServer = std::make_unique<zenith::mcp::MCPServer>(
        *commandAPI, *projectState, *engine, this);
    mcpServer->start();
    ZENITH_LOG_INFO("[MCP STDIO] Started stdio MCP server (GUI mode)");
  }

  // Disable Native Title Bar (Use custom TitleBarComponent)
  setUsingNativeTitleBar(false);
  setTitleBarHeight(0); // Frameless content area
  
  // CRITICAL: Set window size BEFORE adding content
  // This ensures the content component gets proper bounds
  constexpr int defaultWidth = 1400;
  constexpr int defaultHeight = 800;
  
#if JUCE_IOS || JUCE_ANDROID
  setFullScreen(true);
#else
  setResizable(true, false); // Resizable, NO Native Title Bar
  setResizeLimits(800, 600, 4096, 2160); // Min and max sizes
  
  // Set the DocumentWindow size FIRST
  setBounds(100, 100, defaultWidth, defaultHeight);
#endif

  // Set the content component size before adding it
  mainComponent->setSize(defaultWidth - 2, defaultHeight - getTitleBarHeight() - 2);
  
  // Now add the content component  
  // Use setContentNonOwned (setContentComponent) because MainWindow holds unique_ptr
  setContentNonOwned(mainComponent.get(), false); // false = don't resize to content
  
  // Ensure window is centered
  // centreWithSize(getWidth(), getHeight());
  setDropShadowEnabled(false);

  ZENITH_LOG_INFO("MainWindow: Window sized to " + std::to_string(getWidth()) + "x" + std::to_string(getHeight()));
  ZENITH_LOG_INFO("MainWindow: MainComponent size: " + std::to_string(mainComponent->getWidth()) + "x" + std::to_string(mainComponent->getHeight()));

  // Make visible - this will trigger peer creation and OpenGL context attachment
  juce::Component::setVisible(true);
  toFront(true);
  ZENITH_LOG_INFO("MainWindow: setVisible(true) called");
  
  // centreWithSize(getWidth(), getHeight());
  
  // CRITICAL: Remove native window decorations on Linux using X11 hints
  // This must be called AFTER setVisible() so the peer exists
  PlatformWindowUtils::removeWindowDecorations(this);
  
  // Force OpenGL context attachment now that the window is visible
  // The MainComponent inherits from SkiaMainWindowIntegration which has OpenGL
  if (mainComponent && mainComponent->getPeer()) {
    ZENITH_LOG_INFO("MainWindow: Manually scheduling deferred attachment on MainComponent");
    mainComponent->scheduleAttachmentCheck();
  } else {
    ZENITH_LOG_INFO("MainWindow: WARNING: MainComponent has no peer after setVisible!");
  }
  
  ZENITH_LOG_INFO("MainWindow: Initializing Engine...");
  engine->initialize();
  ZENITH_LOG_INFO("MainWindow: Engine initialized successfully.");

  // Start UI animations now that Engine is ready
  if (mainComponent) {
      mainComponent->startAnimations();
  }

  checkForRecovery();
  updateWindowTitle();
}

MainWindow::~MainWindow() {
  ZENITH_LOG_INFO("MainWindow::Destructor STARTED");
  stopTimer();

  if (mcpServer) {
    ZENITH_LOG_INFO("[MCP STDIO] Stopping stdio MCP server");
    mcpServer->stop();
    mcpServer.reset();
  }
  
  ZENITH_LOG_INFO("MainWindow: Shutting down engine...");
  engine->shutdown();
  
  ZENITH_LOG_INFO("MainWindow: Resetting mainComponent...");
  setContentOwned(nullptr, true);
  
  ZENITH_LOG_INFO("MainWindow::Destructor COMPLETE");
}

void MainWindow::closeButtonPressed() {
  ZENITH_LOG_INFO("MainWindow::closeButtonPressed() CALLED");
  
  if (projectState->hasUnsavedChanges()) {
    int result = juce::NativeMessageBox::showYesNoCancelBox(
        juce::AlertWindow::WarningIcon, "Unsaved Changes",
        "Save changes before closing?", this, nullptr);

    if (result == 1) { // Yes
      saveProject();
      // Wait for save? it's synchronous mostly except recent files
      // But if user cancels save?
    } else if (result == 0) { // Cancel
      return;
    }
    // Result 2 is No (discard)
  }

  ZENITH_LOG_INFO("MainWindow: Calling systemRequestedQuit()");
  juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::showAboutDialog() {
  juce::String aboutMessage;
  aboutMessage << "Zenith DAW\n\nVersion: 0.1.0\nBuilt with JUCE 8.0.9";
  juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                         "About Zenith DAW", aboutMessage,
                                         "OK");
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

  // Show premium Skia-based recovery modal
  recoveryModal_ = std::make_unique<ProjectRecoveryModal>(
      recoveries,
      // On Recover callback
      [this](const RecoveryInfo& selected) {
        FileIOError error = fileIO_->recoverFromFile(selected.recoveryFile);
        if (error == FileIOError::Success) {
          updateWindowTitle();
          if (mainComponent) mainComponent->repaint();
        } else {
          juce::NativeMessageBox::showMessageBoxAsync(
              juce::AlertWindow::WarningIcon,
              "Recovery Failed",
              "Failed to recover the project. The backup may be corrupted.");
        }
        recoveryModal_.reset();
      },
      // On Discard callback
      [this, recoveries]() {
        // Delete all recovery files
        for (const auto& info : recoveries) {
          fileIO_->deleteRecoveryFile(info.recoveryFile);
        }
        recoveryModal_.reset();
        recoveryModal_.reset();
      });

  if (mainComponent) {
    mainComponent->addAndMakeVisible(recoveryModal_.get());
    recoveryModal_->setBounds(mainComponent->getLocalBounds());
    recoveryModal_->show();
  }
}

void MainWindow::resized() {
    DocumentWindow::resized(); // Call base
    if (unsavedChangesModal_) {
        unsavedChangesModal_->setBounds(getLocalBounds());
    }
}





void MainWindow::createManualBackup() {
  if (!fileIO_)
    return;
  juce::File backupFile = fileIO_->createBackup();

  if (backupFile.existsAsFile()) {
    juce::NativeMessageBox::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon, "Backup Created",
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

void MainWindow::checkUnsavedAndQuit() {
    if (!unsavedChangesModal_) return; // Safety

    // Configure callbacks
    unsavedChangesModal_->onSaveAndQuit = [this] {
        saveProject(); // This is async usually, but we need to ensure it finishes or triggers quit after.
        // Actually saveProject is async. We might need a blocking save here OR modify saveProject to take a callback.
        // For now, let's assume save is fast enough or use internal fileIO logic.
        // BETTER: saveProject calls fileIO_->saveToFileAsync.
        // We should chain the quit.
        
        // Quick dirty fix: Trigger save logic manually here to chain quit.
         if (fileIO_) {
             juce::File file = fileIO_->getCurrentProjectFile();
             if (file.existsAsFile()) {
                  fileIO_->saveToFileAsync(file, {}, [this](bool success, juce::String) {
                      if (success) {
                          projectState->markSaved(); // Ensure dirty flag is cleared
                          juce::JUCEApplication::getInstance()->systemRequestedQuit(); 
                      }
                  });
             } else {
                 saveProjectAs(); // This is complex to chain. User likely has a file if "Unsaved Changes" is confusing.
                 // If never saved, saveProjectAs opens dialog.
             }
         }
         
         unsavedChangesModal_->setVisible(false);
     };
 
     unsavedChangesModal_->onDiscardAndQuit = [this] {
         // Clear dirty flag so next systemRequestedQuit passes
         if (projectState) projectState->markSaved(); 
         unsavedChangesModal_->setVisible(false);
         juce::JUCEApplication::getInstance()->systemRequestedQuit();
     };

    unsavedChangesModal_->onCancel = [this] {
        unsavedChangesModal_->setVisible(false);
    };

    unsavedChangesModal_->setVisible(true);
    unsavedChangesModal_->toFront(true);
    resized(); // Ensure bounds
}

void MainWindow::newProject() {
  if (projectState->hasUnsavedChanges()) {
    int result = juce::NativeMessageBox::showYesNoCancelBox(
        juce::AlertWindow::WarningIcon, "Unsaved Changes",
        "Save changes before creating a new project?", this, nullptr);

    if (result == 1) {
      saveProject();
    } else if (result == 0) {
      return; // Cancel
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

  // Use async save to keep UI responsive
  fileIO_->saveToFileAsync(projectFile, {}, [this, projectFile](bool success, juce::String error) {
    if (!success) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "Save Failed",
            "Failed to save project: " + error);
        return;
    }

    updateWindowTitle();
    if (recentProjectManager_) {
      recentProjectManager_->addProject(projectFile,
                                        projectState->getProjectName());
      recentProjectManager_->save();
    }
  });
}

void MainWindow::saveProjectAs() {
  auto chooser = std::make_shared<::juce::FileChooser>(
      "Save Project As...",
      ::juce::File::getSpecialLocation(::juce::File::userDocumentsDirectory),
      "*.zth");
  auto chooserFlags = ::juce::FileBrowserComponent::saveMode |
                      ::juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(chooserFlags, [this,
                                       chooser](const ::juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file == juce::File{})
      return;
    if (!file.hasFileExtension(".zth"))
      file = file.withFileExtension(".zth");

    fileIO_->saveToFileAsync(file, {}, [this, file](bool success, juce::String error) {
        if (!success) {
          juce::NativeMessageBox::showMessageBoxAsync(
              juce::AlertWindow::WarningIcon, "Save Failed",
              "Failed to save project: " + error);
          return;
        }

        updateWindowTitle();
        if (recentProjectManager_) {
          recentProjectManager_->addProject(file, projectState->getProjectName());
          recentProjectManager_->save();
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
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "Load Failed",
            "Failed to load project: " + error);
        return;
      }

      if (recentProjectManager_) {
        recentProjectManager_->addProject(file, projectState->getProjectName());
        recentProjectManager_->save();
      }
      updateWindowTitle();
      
      // CRITICAL: Only show the main UI once data is fully loaded to prevent Skia crashes
      if (mainComponent) {
          mainComponent->setMainUiVisible(true);
      }
      repaint();
  });

  return true;
}

void MainWindow::openProject() {
  auto chooser = std::make_shared<::juce::FileChooser>(
      "Open Project",
      ::juce::File::getSpecialLocation(::juce::File::userDocumentsDirectory),
      "*.zth");
  auto chooserFlags = ::juce::FileBrowserComponent::openMode |
                      ::juce::FileBrowserComponent::canSelectFiles;
  chooser->launchAsync(chooserFlags,
                       [this, chooser](const ::juce::FileChooser &fc) {
                         auto file = fc.getResult();
                         if (file != juce::File{})
                           loadProject(file);
                       });
}

} // namespace zenith
