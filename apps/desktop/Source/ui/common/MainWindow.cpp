/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "MainWindow.h"
#include "../../commands/CommandAPI.h"
#include "../../engine/TempoMap.h" // Added by instruction
#include "../engine/Clip.h"
#include "../engine/Track.h"
#include "ArrangerComponent.h"
#include "ClipSynchronizer.h"
#include "InstrumentBrowserPanel.h"
#include "MainLayoutComponent.h"
#include "MenuBar.h"
#include "PianoRollComponent.h"
#include "SettingsComponent.h"
#include "TempoMap.h"
#include "TrackAutomationSynchronizer.h"
#include "WingmanPanel.h"
#include "ZenithHubComponent.h"
#include "ZenithLookAndFeel.h" // For colors
#include "mcp/MCPServer.h"
#include <memory>

#include "../ai/PresetGeneticistAgent.h"
#include "../ai/SessionDebuggerAgent.h"
#include "../ai/UXDirectorAgent.h"
#include "../engine/ZenithLogger.h"

#include "SkiaComponent.h"
#include "SkiaMainWindowIntegration.h"
#include "ZenithDesignSystem.h"
#include "ZenithSkia.h"
#include <core/SkImageInfo.h>
#include <core/SkPixmap.h>
#include <core/SkSamplingOptions.h>
#include <core/SkSurface.h>

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
  // Register as key listener for undo/redo shortcuts
  addKeyListener(this);
  addMouseListener(this, true); // Intercept mouse events recursively
  setWantsKeyboardFocus(true);

  // Add Debug Overlay
  // addChildComponent(&zenith::DebugLogOverlay::getInstance());

  setOpaque(true);

  ZENITH_LOG_INFO("========================================");
  ZENITH_LOG_INFO("MainComponent Constructor - Modern DAW Layout");
  ZENITH_LOG_INFO("========================================");

  ZENITH_LOG_INFO(">>> ZENITH_USE_SKIA IS DEFINED - MODERN SKIA DAW LAYOUT "
                  "BRANCH EXECUTING <<<");

  // Initialize Skia rendering system
  // Skia initialization is handled by
  // SkiaMainWindowIntegration::newOpenGLContextCreated

  // Instantiate the SkiaRenderer
  ZENITH_LOG_INFO("-> Initializing SkiaRenderer...");
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

  addAndMakeVisible(transportBar.get());
  ZENITH_LOG_INFO("V TransportBar created");

  // The "Perfect DAW" Tri-Pane Layout Manager
  ZENITH_LOG_INFO("-> Creating MainLayoutComponent...");
  mainLayout =
      std::make_unique<zenith::MainLayoutComponent>(engine, projectState);
  addAndMakeVisible(mainLayout.get());
  ZENITH_LOG_INFO("V MainLayoutComponent created");

  // Connect browser collapse callback (proxied through MainLayout if needed, or
  // handled internally) For now, MainLayout handles its own resizing when
  // browser toggles.

  // Right: AI Assistant Panel (Wingman) - Pure Skia
  ZENITH_LOG_INFO("-> Creating RightSidePanel...");
  rightSidePanel = std::make_unique<zenith::RightSidePanel>(api, engine);
  addAndMakeVisible(rightSidePanel.get());
  ZENITH_LOG_INFO("V RightSidePanel created");

  // Bottom: Piano Keyboard + Mixer Strip
  ZENITH_LOG_INFO("-> Creating BottomBar...");
  bottomBar = std::make_unique<zenith::BottomBar>(midiKeyboardState, engine,
                                                  projectState);
  bottomBar->setKeyboardVisible(false); // Hidden by default

  // Connect Session Debugger
  if (auto *debugger = engine.getSessionDebugger()) {
    bottomBar->setDebugger(debugger);
    ZENITH_LOG_INFO("V Session Debugger connected to BottomBar");
  }

  addAndMakeVisible(bottomBar.get());
  ZENITH_LOG_INFO("V BottomBar created");

  // Source of Truth Demo REMOVED - Was cluttering UI
  // To re-enable, uncomment the volumeKnob creation below
  /*
  auto trackNode =
      projectState.state.getChildWithName(Zenith::IDs::TRACKS).getChild(0);
  if (trackNode.isValid()) {
    volumeKnob =
        std::make_unique<zenith::ZenithKnob>(trackNode.getPropertyAsValue(
            Zenith::IDs::volume, &projectState.undoManager));
    volumeKnob->setLabel("Track 1 Volume");
    addAndMakeVisible(volumeKnob.get());
    ZENITH_LOG_INFO("V VolumeKnob created (Source of Truth Demo)");
  }
  */

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
    options.dialogBackgroundColour =
        juce::Colours::black; // Simple fallback or lookandfeel
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
  };

  // Create Zenith Hub with real project manager
  ZENITH_LOG_INFO("-> Creating ZenithHubComponent...");
  hubComponent = std::make_unique<zenith::ZenithHubComponent>(
      recentProjectManager_,
      // Load project callback
      [this](const juce::File &projectPath) {
        DBG("MainComponent: Loading project from Hub: " +
            projectPath.getFullPathName());
        if (hubComponent) {
          hubComponent->setVisible(false);
        }
        if (onLoadProject_) {
          onLoadProject_(projectPath);
        }
        repaint(); // Trigger redraw after Hub is hidden
      },
      // New project callback
      [this]() {
        DBG("MainComponent: Creating new project from Hub");
        if (hubComponent) {
          hubComponent->setVisible(false);
        }
        if (onNewProject_) {
          onNewProject_();
        }
        repaint(); // Trigger redraw after Hub is hidden
      },
      // Dismiss callback
      [this]() {
        if (hubComponent) {
          hubComponent->setVisible(false);
          repaint(); // Trigger redraw after Hub is hidden
          ZENITH_LOG_INFO("MainComponent: Hub dismissed, triggering repaint.");
        }
      });
  addAndMakeVisible(hubComponent.get());
  hubComponent->show();
  ZENITH_LOG_INFO("V ZenithHubComponent created");

  // Start animation timer (SkiaMainWindowIntegration handles this)
  DBG("V Animation timer managed by SkiaMainWindowIntegration");
  startTimerHz(30); // 30Hz for UI sync is plenty
  setSize(1400, 800);

  DBG("========================================");
  ZENITH_LOG_INFO("MainComponent Constructor COMPLETE");
  ZENITH_LOG_INFO("========================================");
}

MainComponent::~MainComponent() {
  DBG("MainComponent Destructor called");
  removeKeyListener(this);
}

bool MainComponent::keyPressed(const juce::KeyPress &key,
                               Component *originatingComponent) {
  juce::ignoreUnused(originatingComponent);

  // Ctrl+Z or Cmd+Z for undo
  if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown() &&
      !key.getModifiers().isShiftDown() && projectState.canUndo()) {
    projectState.undo();
    DBG("Keyboard shortcut: Undo");
    return true;
  }

  // Ctrl+Shift+Z or Cmd+Shift+Z for redo
  if (key.getTextCharacter() == 'Z' && key.getModifiers().isCommandDown() &&
      key.getModifiers().isShiftDown() && projectState.canRedo()) {
    projectState.redo();
    DBG("Keyboard shortcut: Redo");
    return true;
  }

  // Ctrl+Y or Cmd+Y for redo (alternative)
  if (key.getTextCharacter() == 'y' && key.getModifiers().isCommandDown() &&
      projectState.canRedo()) {
    projectState.redo();
    DBG("Keyboard shortcut: Redo (Y)");
    return true;
  }

  // M key: Toggle virtual MIDI keyboard (like Ableton Live)
  if (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M') {
    if (bottomBar) {
      bottomBar->setKeyboardVisible(!bottomBar->isKeyboardVisible());
      resized();
    }
    DBG("Keyboard shortcut: Toggle Virtual MIDI Keyboard (M)");
    return true;
  }

  // Tab key: Toggle between Session View and Arranger View
  if (key == juce::KeyPress::tabKey &&
      !key.getModifiers().isAnyModifierKeyDown() && transportBar &&
      transportBar->onViewToggleClicked) {
    transportBar->onViewToggleClicked();
    DBG("Keyboard shortcut: Toggle Session/Arranger View (Tab)");
    return true;
  }

  return false; // Key not handled
}

void MainComponent::timerCallback() {
  if (transportBar) {
    transportBar->setPlaying(engine.isPlaying());
    transportBar->setRecording(engine.isRecording());
    transportBar->setTempo(
        engine.getTempoMap().getTempoAt(engine.getPlaybackPositionBeats()));
    transportBar->setCPU(engine.getCpuUsage());
    transportBar->setPosition(engine.getPlaybackPosition());
  }

  // Update volume knob if it exists (Demo)
  if (volumeKnob) {
    // The knob uses juce::Value so it should sync automatically if hooked to a
    // ValueTree property, but we can force it here for the demo if needed.
  }
}

void MainComponent::paint(juce::Graphics &g) {
  // Delegate to base class which handles initialization status
  static bool hasLoggedPaint = false;
  if (!hasLoggedPaint) {
    ZENITH_LOG_INFO("MainComponent::paint called (First Paint) - RASTER MODE");
    hasLoggedPaint = true;
  }
  SkiaMainWindowIntegration::paint(g);
}

void MainComponent::drawSkiaContent(SkCanvas *canvas) {
  // Clear background
  canvas->clear(zenith::design::colors::BG_DARKEST);

  // Helper lambda to draw a child if visible
  auto drawChild = [&](juce::Component *child,
                       zenith::SkiaComponent *skiaChild) {
    if (child && child->isVisible() && skiaChild) {
      canvas->save();
      auto bounds = child->getBounds();
      canvas->translate((float)bounds.getX(), (float)bounds.getY());
      canvas->clipRect(
          SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()));
      skiaChild->drawSkia(canvas);
      canvas->restore();
    }
  };

  // Draw known children in order (Zero-Allocation, No Dynamic Cast)

  // 1. Core DAW Components (Only draw if Hub is hidden or transitioning)
  bool hubIsVisible = hubComponent && hubComponent->isVisible();
  float hubAlpha =
      hubComponent
          ? hubComponent->getAlpha()
          : 0.0f; // Note: We might need a getAlpha helper if not public

  if (!hubIsVisible || hubAlpha < 0.99f) {
    drawChild(transportBar.get(), transportBar.get());
    drawChild(mainLayout.get(), mainLayout.get());
    drawChild(rightSidePanel.get(), rightSidePanel.get());
    drawChild(bottomBar.get(), bottomBar.get());

    // Demo knob removed - was cluttering UI
    // drawChild(volumeKnob.get(), volumeKnob.get());
  }

  // 6. Zenith Hub (Topmost Overlay)
  if (hubComponent && hubComponent->isVisible()) {
    drawChild(hubComponent.get(), hubComponent.get());
  } else {
    static bool hasLoggedMainLayout = false;
    if (!hasLoggedMainLayout) {
      ZENITH_LOG_INFO("MainComponent: Hub hidden, drawing main DAW interface.");
      ZENITH_LOG_INFO(
          "  - Transport: " +
          juce::String(transportBar
                           ? (transportBar->isVisible() ? "Visible" : "Hidden")
                           : "NULL"));
      ZENITH_LOG_INFO(
          "  - MainLayout: " +
          juce::String(mainLayout
                           ? (mainLayout->isVisible() ? "Visible" : "Hidden")
                           : "NULL"));
      if (mainLayout) {
        auto b = mainLayout->getBounds();
        ZENITH_LOG_INFO("    Bounds: " + b.toString());
      }
      ZENITH_LOG_INFO(
          "  - RightPanel: " +
          juce::String(rightSidePanel ? (rightSidePanel->isVisible() ? "Visible"
                                                                     : "Hidden")
                                      : "NULL"));
      ZENITH_LOG_INFO(
          "  - BottomBar: " +
          juce::String(bottomBar
                           ? (bottomBar->isVisible() ? "Visible" : "Hidden")
                           : "NULL"));
      hasLoggedMainLayout = true;
    }
  }
}

void MainComponent::mouseDown(const juce::MouseEvent &e) {
  if (zenith::design::LayoutManager::getInstance().isEditModeEnabled()) {
    activeDragComponent = nullptr;
    if (transportBar && transportBar->getBounds().contains(e.getPosition()))
      activeDragComponent = transportBar.get();
    else if (rightSidePanel &&
             rightSidePanel->getBounds().contains(e.getPosition()))
      activeDragComponent = rightSidePanel.get();
    else if (bottomBar && bottomBar->getBounds().contains(e.getPosition()))
      activeDragComponent = bottomBar.get();
    else if (mainLayout && mainLayout->getBounds().contains(e.getPosition()))
      activeDragComponent = mainLayout.get();
    else if (hubComponent && hubComponent->isVisible() &&
             hubComponent->getBounds().contains(e.getPosition()))
      activeDragComponent = hubComponent.get();

    if (activeDragComponent) {
      dragStartBounds = activeDragComponent->getBounds();
      return; // Consume event
    }
  }

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

void MainComponent::resized() {
  auto bounds = getLocalBounds();

  if (bounds.isEmpty()) {
    ZENITH_LOG_WARNING(
        "MainComponent::resized() called with empty bounds! Skipping layout.");
    return;
  }

  DBG("MainComponent::resized() called - Total bounds: " +
      juce::String(bounds.getWidth()) + "x" + juce::String(bounds.getHeight()));

  // ============================================================================
  // Modern DAW Layout with Skia Panels
  // ============================================================================
  DBG("  Using ZENITH_USE_SKIA layout branch");

  // Top: Transport Bar (60px height)
  if (transportBar) {
    auto transportBounds = bounds.removeFromTop(60);
    transportBar->setBounds(transportBounds);
    DBG("  V TransportBar positioned at: " + transportBounds.toString());
  } else {
    DBG("  X TransportBar is NULL!");
  }

  // Bottom: Piano Keyboard + Mixer Strip (128px height when visible)
  if (bottomBar) {
    auto bottomBounds = bounds.removeFromBottom(128);
    bottomBar->setBounds(bottomBounds);
    DBG("  V BottomBar positioned at: " + bottomBounds.toString());
  } else {
    DBG("  X BottomBar is NULL!");
  }

  // Left: Browser Panel (Managed by MainLayoutComponent now)
  // MainLayoutComponent handles Browser, Session, and Arranger internally

  // Right: Scratch Pads + Wingman Console (400px width)
  if (rightSidePanel) {
    auto rightBounds = bounds.removeFromRight(400);
    rightSidePanel->setBounds(rightBounds);
    DBG("  V RightSidePanel positioned at: " + rightBounds.toString());
  } else {
    DBG("  X RightSidePanel is NULL!");
  }

  // Center: Main Layout (Browser + Session/Arranger)
  if (mainLayout) {
    mainLayout->setBounds(bounds);
    DBG("  V MainLayoutComponent positioned at: " + bounds.toString());
  } else {
    DBG("  X MainLayoutComponent is NULL!");
  }

  // Overlay: Zenith Hub
  if (hubComponent) {
    hubComponent->setBounds(getLocalBounds());
    DBG("  V ZenithHubComponent positioned at: " +
        hubComponent->getBounds().toString());
  }

  // Demo knob removed
  // if (volumeKnob) {
  //   volumeKnob->setBounds(10, 10, 100, 100);
  // }
}

//==============================================================================
// Integration: Piano roll opener
//==============================================================================

void MainComponent::openPianoRoll(const juce::String &trackId,
                                  const juce::String &clipId) {
  DBG("MainComponent: Opening piano roll for " + trackId + "/" + clipId);

  // Create new piano roll editor window
  // Note: Window deletes itself when closed (see
  // PianoRollEditor::closeButtonPressed)
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  new PianoRollWindow(projectState, engine, trackId, clipId);
}

//==============================================================================
// Phase 1: Audio Import
//==============================================================================

void MainComponent::handleImportAudio() {
  // Create file chooser for audio files
  auto chooser = std::make_shared<juce::FileChooser>(
      "Import Audio File", juce::File{},
      "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg");

  // Open file chooser (async)
  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(
      chooserFlags, [this, chooser](const juce::FileChooser &fc) {
        auto file = fc.getResult();
        if (!file.existsAsFile())
          return;

        DBG("Importing audio file: " + file.getFullPathName());

        // Ensure we have at least one track
        if (engine.getNumTracks() == 0) {
          DBG("Creating first track for audio import");
          engine.addTestTracks(1);
        }

        // Get the first track
        const auto &tracks = engine.tracks();
        if (tracks.empty()) {
          DBG("ERROR: Failed to get track after creation");
          return;
        }

        auto *track = tracks[0].get();
        if (track == nullptr) {
          DBG("ERROR: Track is null");
          return;
        }

        // Create a new clip
        auto clip = std::make_unique<zenith::Clip>();
        clip->setType(zenith::Clip::Type::Audio);
        clip->setName(file.getFileNameWithoutExtension());

        // Load audio file through pool (message thread - safe to do I/O)
        auto &pool = engine.getAudioFilePool();
        clip->setAudioFileFromPool(file, pool);

        // Set clip timing: start at position 0, play immediately
        clip->setStartPosition(0);
        clip->setPlaying(true);

        DBG("Clip created: " + clip->getName() +
            ", length: " + juce::String(clip->getLength()) + " samples");

        // Add clip to track
        track->addClip(std::move(clip));

        DBG("Audio import complete! Track now has " +
            juce::String(track->getNumClips()) + " clip(s)");
      });
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
  // Create audio engine first
  engine = std::make_unique<zenith::Engine>();

  // Create project state
  projectState = std::make_unique<zenith::ProjectState>();

  // Phase 13: Create automation synchronizer
  automationSync = std::make_unique<zenith::TrackAutomationSynchronizer>(
      *projectState, *engine);

  // Phase 5: Create Wingman command API
  commandAPI = std::make_unique<zenith::CommandAPI>(*projectState, *engine);

  // Phase 11: Create track state synchronizer
  trackSynchronizer =
      std::make_unique<zenith::TrackStateSynchronizer>(*projectState, *engine);
  trackSynchronizer->initialize();

  // Phase 13: Connect project state to engine for automation
  engine->setProjectState(projectState.get());

  // Integration: Create clip synchronizer
  clipSynchronizer =
      std::make_unique<zenith::ClipSynchronizer>(*projectState, *engine);

  // Pinocchio Protocol: Create Recent Project Manager
  recentProjectManager_ = std::make_unique<zenith::RecentProjectManager>();

  // Embedded MCP Server with Vision
  DBG("Starting embedded MCP server...");
  mcpServer = std::make_unique<zenith::mcp::MCPServer>(
      *commandAPI, *projectState, *engine, this);
  mcpServer->startBackground();

  // Add some demo tracks for testing (Phase 9 + existing features)
  projectState->addTrack("Audio 1", "audio");
  projectState->addTrack("MIDI 1", "midi");
  projectState->addTrack("Audio 2", "audio");

  // Main content with project loading callbacks
  // Main content with project loading callbacks
  mainComponent = std::make_unique<MainComponent>(
      *engine, *commandAPI, *projectState, *recentProjectManager_,
      // Load project callback
      [this](const juce::File &file) { loadProject(file); },
      // New project callback
      [this]() {
        // For now, just dismiss the hub and start with default project
        DBG("MainWindow: New project requested");
        // In the future, could show a template dialog or reset project state
      });

  // Instantiate AI agents (Brain integration)
  uxDirector = std::make_unique<ai::UXDirectorAgent>(*engine, *projectState,
                                                     *mainComponent);
  commandAPI->setUXDirector(uxDirector.get());
  uxDirector->startMonitoring(500); // 500ms intervals

  presetGeneticist = std::make_unique<ai::PresetGeneticistAgent>();
  commandAPI->setPresetGeneticist(presetGeneticist.get());

  // Set up window
  setUsingNativeTitleBar(true);
  setContentOwned(mainComponent.get(), true);

#if JUCE_IOS || JUCE_ANDROID
  setFullScreen(true);
#else
  setResizable(true, true);
  centreWithSize(getWidth(), getHeight());
#endif

  juce::Component::setVisible(true);

  // Initialize audio engine after window is visible
  engine->initialize();

  // Start automation synchronizer (Phase 13)
  automationSync->start(60); // 60 Hz update rate

  DBG("MainWindow created and initialized");
}

MainWindow::~MainWindow() {
  // Clear menu bar first
  // Clear menu bar first
  // setMenuBar(nullptr);
  // menuBar.reset(); (Already removed from header)

  // Shutdown audio engine before destroying components
  if (engine)
    engine->shutdown();

  // Clear content
  setContentOwned(nullptr, true);

  DBG("MainWindow: Content cleared");
  ZENITH_LOG_INFO("MainWindow destroyed");
}

void MainWindow::closeButtonPressed() {
  ZENITH_LOG_INFO("MainWindow: closeButtonPressed called - Requesting quit");
  // Check for unsaved changes logic deferred
  juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::showAboutDialog() {
  juce::String aboutMessage;
  aboutMessage << "Zenith DAW\n\n";
  aboutMessage << "A professional digital audio workstation\n\n";
  aboutMessage << "Version: 0.1.0\n";
  aboutMessage << "Built with JUCE 8.0.9\n\n";
  aboutMessage << "For documentation and installation instructions, see:\n";
  aboutMessage << "• docs/README.md\n";
  aboutMessage << "• docs/INSTALL_WINDOWS.md";

  juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                         "About Zenith DAW", aboutMessage,
                                         "OK");
}

void MainWindow::saveProject() {
  if (currentProjectFile.existsAsFile()) {
    if (projectState->saveToFile(currentProjectFile)) {
      // Add to recent projects on successful save
      if (recentProjectManager_) {
        recentProjectManager_->addProject(currentProjectFile,
                                          projectState->getProjectName());
        recentProjectManager_->save();
      }
    }
  } else {
    saveProjectAs();
  }
}

void MainWindow::saveProjectAs() {
  auto chooser = std::make_shared<juce::FileChooser>(
      "Save Project As...",
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
      "*.zth");

  auto chooserFlags = juce::FileBrowserComponent::saveMode |
                      juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(chooserFlags, [this,
                                      chooser](const juce::FileChooser &fc) {
    auto file = fc.getResult();
    if (file == juce::File{})
      return;

    // Ensure extension
    if (!file.hasFileExtension(".zth"))
      file = file.withFileExtension(".zth");

    if (projectState->saveToFile(file)) {
      currentProjectFile = file;
      setName("Zenith DAW - " + file.getFileNameWithoutExtension());

      // Add to recent projects on successful save
      if (recentProjectManager_) {
        recentProjectManager_->addProject(file, projectState->getProjectName());
        recentProjectManager_->save();
      }
    }
  });
}

bool MainWindow::loadProject(const juce::File &file) {
  if (!file.existsAsFile()) {
    DBG("MainWindow: Project file does not exist: " + file.getFullPathName());
    return false;
  }

  DBG("MainWindow: Loading project from " + file.getFullPathName());

  // Stop the engine during load
  engine->stop();

  // Load into EXISTING project state to preserve references held by
  // MainComponent
  if (!projectState->loadFromFile(file)) {
    DBG("MainWindow: Failed to load project file");
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, "Load Failed",
        "Failed to load project from:\n" + file.getFullPathName(), "OK");
    return false;
  }

  currentProjectFile = file;

  // Add to recent projects
  if (recentProjectManager_) {
    recentProjectManager_->addProject(file, projectState->getProjectName());
    recentProjectManager_->save();
  }

  // Update window title
  setName("Zenith DAW - " + file.getFileNameWithoutExtension());

  // Restart engine if needed (engine handles state changes via listeners
  // hopefully) engine->setProjectState(projectState.get()); // Redundant if
  // pointer hasn't changed

  // Force a repaint or refresh if necessary
  if (mainComponent) {
    // mainComponent->refresh(); // Method doesn't exist, rely on ValueTree
    // listeners
  }

  DBG("MainWindow: Project loaded successfully");
  return true;
}

void MainWindow::openProject() {
  auto chooser = std::make_shared<juce::FileChooser>(
      "Open Project",
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
      "*.zth");

  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(chooserFlags,
                       [this, chooser](const juce::FileChooser &fc) {
                         auto file = fc.getResult();
                         if (file == juce::File{})
                           return;

                         loadProject(file);
                       });
}

// Legacy ZenithMenuBar Implementation removed
// All logic moved to Source/ui/MenuBar.cpp
