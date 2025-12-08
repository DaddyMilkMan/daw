/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../../include/MainWindow.h"
#include "../../include/CommandAPI.h"
#include "../../include/TrackAutomationSynchronizer.h"
#include "../../include/ClipSynchronizer.h"
#include "../engine/Clip.h"
#include "../engine/Track.h"
#include "../network/AIBridgeClient.h"
#include "../../include/ui/ArrangerComponent.h"
#include "InstrumentBrowserPanel.h"
#include "MainLayoutComponent.h"
#include "WingmanPanel.h"
#include "../../include/ui/PianoRollComponent.h"
#include "SettingsComponent.h"
#include "ZenithLookAndFeel.h" // For colors

#include "SimpleLogger.h"

#ifdef ZENITH_USE_SKIA
#include "../ui/skia/SkiaComponent.h"
#include "../ui/skia/SkiaMainWindowIntegration.h"
#include "../ui/skia/ZenithDesignSystem.h"
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkImage.h>
#include <skia/include/core/SkImageInfo.h>
#include <skia/include/core/SkPixmap.h>
#include <skia/include/core/SkSamplingOptions.h>
#include <skia/include/core/SkSurface.h>
#include <skia/include/core/SkTextBlob.h>

#endif

using namespace zenith;

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(zenith::Engine &eng, zenith::CommandAPI &api,
                             zenith::AIBridgeClient &aiClient,
                             zenith::ProjectState &state)
    : engine(eng), projectState(state)
{
  // Register as key listener for undo/redo shortcuts
  addKeyListener(this);
  addMouseListener(this, true); // Intercept mouse events recursively
  setWantsKeyboardFocus(true);

  // Add Debug Overlay
  // addChildComponent(&zenith::DebugLogOverlay::getInstance());

  // Show Console
  showDebugConsole();

  setSize(1400, 800);

  DBG("========================================");
  DBG("MainComponent Constructor - Modern DAW Layout");
  DBG("========================================");

  logToFile(">>> ZENITH_USE_SKIA IS DEFINED - MODERN SKIA DAW LAYOUT BRANCH "
            "EXECUTING <<<");

  // Initialize Skia rendering system
  // Skia initialization is handled by
  // SkiaMainWindowIntegration::newOpenGLContextCreated

  // Instantiate the SkiaRenderer
  logToFile("→ Initializing SkiaRenderer...");
  // ============================================================================
  // Create Modern DAW Layout Panels
  // ============================================================================

  // Top: Transport Bar
  DBG("→ Creating TransportBar...");
  transportBar = std::make_unique<zenith::TransportBar>();
  transportBar->setProjectName("Zenith DAW");
  transportBar->setTempo(120.0);
  transportBar->setTimeSignature(4, 4);

  // Hook up transport callbacks
  transportBar->onPlayClicked = [this]() {
    engine.play();
    DBG("Play clicked");
  };
  transportBar->onStopClicked = [this]() {
    engine.stop();
    DBG("Stop clicked");
  };
  transportBar->onRecordClicked = [this]() {
    engine.toggleRecording();
    bool isRec = engine.isRecording();
    transportBar->setRecording(isRec);
    if (isRec) {
      DBG("Recording started");
    } else {
      DBG("Recording stopped");
    }
  };

  addAndMakeVisible(transportBar.get());
  DBG("✓ TransportBar created and made visible at " +
      juce::String::toHexString(reinterpret_cast<juce::pointer_sized_int>(transportBar.get())));

  // The "Perfect DAW" Tri-Pane Layout Manager
  DBG("→ Creating MainLayoutComponent...");
  mainLayout = std::make_unique<zenith::MainLayoutComponent>(engine, projectState);
  addAndMakeVisible(mainLayout.get());
  DBG("✓ MainLayoutComponent created and made visible at " +
      juce::String::toHexString(reinterpret_cast<juce::pointer_sized_int>(mainLayout.get())));

  // Connect browser collapse callback (proxied through MainLayout if needed, or
  // handled internally) For now, MainLayout handles its own resizing when
  // browser toggles.

  // Right: AI Assistant Panel (Wingman) - Pure Skia
  DBG("→ Creating RightSidePanel...");
  rightSidePanel = std::make_unique<zenith::RightSidePanel>(api, aiClient, engine);
  addAndMakeVisible(rightSidePanel.get());
  DBG("✓ RightSidePanel created and made visible at " +
      juce::String::toHexString(reinterpret_cast<juce::pointer_sized_int>(rightSidePanel.get())));

  // Bottom: Piano Keyboard + Mixer Strip
  DBG("→ Creating BottomBar...");
  bottomBar = std::make_unique<zenith::BottomBar>(midiKeyboardState);
  bottomBar->setKeyboardVisible(false); // Hidden by default
  addAndMakeVisible(bottomBar.get());
  DBG("✓ BottomBar created and made visible at " +
      juce::String::toHexString(reinterpret_cast<juce::pointer_sized_int>(bottomBar.get())));

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
      options.dialogBackgroundColour = juce::Colours::black; // Simple fallback or lookandfeel
      options.escapeKeyTriggersCloseButton = true;
      options.useNativeTitleBar = true;
      options.resizable = true;
      options.launchAsync();
  };

  // Start animation timer (SkiaMainWindowIntegration handles this)
  DBG("✓ Animation timer managed by SkiaMainWindowIntegration");

  DBG("========================================");
  DBG("MainComponent Constructor COMPLETE");
  DBG("========================================");
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
  if (key.getTextCharacter() == 'y' && key.getModifiers().isCommandDown() && projectState.canRedo()) {
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
      !key.getModifiers().isAnyModifierKeyDown() &&
      transportBar && transportBar->onViewToggleClicked) {
      transportBar->onViewToggleClicked();
      DBG("Keyboard shortcut: Toggle Session/Arranger View (Tab)");
      return true;
  }

  return false; // Key not handled
}

void MainComponent::paint(juce::Graphics &g) {
  // Delegate to base class which handles initialization status
  SkiaMainWindowIntegration::paint(g);
}

void MainComponent::drawSkiaContent(SkCanvas* canvas) {
    // Clear background
    canvas->clear(SkColorSetRGB(20, 20, 25)); // Dark background
    
    // Helper lambda to draw a child if visible
    auto drawChild = [&](juce::Component* child, zenith::SkiaComponent* skiaChild) {
        if (child && child->isVisible() && skiaChild) {
            canvas->save();
            auto bounds = child->getBounds();
            canvas->translate((float)bounds.getX(), (float)bounds.getY());
            canvas->clipRect(SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()));
            skiaChild->drawSkia(canvas);
            canvas->restore();
        }
    };

    // Draw known children in order (Zero-Allocation, No Dynamic Cast)
    
    // 1. Transport Bar
    drawChild(transportBar.get(), transportBar.get());

    // 2. Main Layout (Browser + Session/Arranger)
    drawChild(mainLayout.get(), mainLayout.get());

    // 3. Right Side Panel
    drawChild(rightSidePanel.get(), rightSidePanel.get());

    // 4. Bottom Bar
    drawChild(bottomBar.get(), bottomBar.get());
    
    // 5. Wingman Panel (if hosted directly, but currently inside RightSidePanel)
    // If it were direct: drawChild(wingmanPanelPtr_.get(), wingmanPanelPtr_.get());
}

void MainComponent::mouseDown(const juce::MouseEvent &e) {
  if (zenith::design::LayoutManager::getInstance().isEditModeEnabled()) {
      activeDragComponent = nullptr;
      if (transportBar && transportBar->getBounds().contains(e.getPosition())) activeDragComponent = transportBar.get();
      else if (rightSidePanel && rightSidePanel->getBounds().contains(e.getPosition())) activeDragComponent = rightSidePanel.get();
      else if (bottomBar && bottomBar->getBounds().contains(e.getPosition())) activeDragComponent = bottomBar.get();
      else if (mainLayout && mainLayout->getBounds().contains(e.getPosition())) activeDragComponent = mainLayout.get();
      
      if (activeDragComponent) {
          dragStartBounds = activeDragComponent->getBounds();
          return; // Consume event
      }
  }

  if (e.mods.isPopupMenu()) {
    juce::PopupMenu m;
    m.addItem("Show Debug Logs", [] {
      // Debug logs action
    });
    m.showMenuAsync(juce::PopupMenu::Options());
  }
}

void MainComponent::mouseDrag(const juce::MouseEvent& e) {
    if (activeDragComponent && zenith::design::LayoutManager::getInstance().isEditModeEnabled()) {
        auto offset = e.getOffsetFromDragStart();
        auto newBounds = dragStartBounds.translated(offset.x, offset.y);
        
        activeDragComponent->setBounds(newBounds);
        
        // Update Manager (persist as relative)
        auto localBounds = getLocalBounds().toFloat();
        if (localBounds.getWidth() > 0 && localBounds.getHeight() > 0) {
            juce::Rectangle<float> relative(
                newBounds.getX() / localBounds.getWidth(),
                newBounds.getY() / localBounds.getHeight(),
                newBounds.getWidth() / localBounds.getWidth(),
                newBounds.getHeight() / localBounds.getHeight()
            );
            
            zenith::design::LayoutManager::PanelState state;
            state.relativeBounds = relative;
            state.isVisible = true;
            
            juce::String id;
            if (activeDragComponent == transportBar.get()) id = "Transport";
            else if (activeDragComponent == rightSidePanel.get()) id = "RightPanel";
            else if (activeDragComponent == bottomBar.get()) id = "BottomBar";
            else if (activeDragComponent == mainLayout.get()) id = "MainLayout";
            
            if (id.isNotEmpty()) {
                state.id = id;
                zenith::design::LayoutManager::getInstance().setPanelState(id, state);
            }
        }
        
        repaint(); // Skia repaint
    }
}

void MainComponent::mouseUp(const juce::MouseEvent& e) {
    activeDragComponent = nullptr;
}

void MainComponent::resized() {
  auto bounds = getLocalBounds();

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
    DBG("  ✓ TransportBar positioned at: " + transportBounds.toString());
  } else {
    DBG("  ✗ TransportBar is NULL!");
  }

  // Bottom: Piano Keyboard + Mixer Strip (96px height when visible)
  if (bottomBar) {
    auto bottomBounds = bounds.removeFromBottom(96);
    bottomBar->setBounds(bottomBounds);
    DBG("  ✓ BottomBar positioned at: " + bottomBounds.toString());
  } else {
    DBG("  ✗ BottomBar is NULL!");
  }

  // Left: Browser Panel (Managed by MainLayoutComponent now)
  // MainLayoutComponent handles Browser, Session, and Arranger internally

  // Right: Scratch Pads + Wingman Console (400px width)
  if (rightSidePanel) {
    auto rightBounds = bounds.removeFromRight(400);
    rightSidePanel->setBounds(rightBounds);
    DBG("  ✓ RightSidePanel positioned at: " + rightBounds.toString());
  } else {
    DBG("  ✗ RightSidePanel is NULL!");
  }

  // Center: Main Layout (Browser + Session/Arranger)
  if (mainLayout) {
    mainLayout->setBounds(bounds);
    DBG("  ✓ MainLayoutComponent positioned at: " + bounds.toString());
  } else {
    DBG("  ✗ MainLayoutComponent is NULL!");
  }
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
  new PianoRollWindow(projectState, trackId, clipId);
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
        auto clip = std::make_unique<zenith::Track::Clip>();
        clip->setType(zenith::Track::Clip::Type::Audio);
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
  automationSync =
      std::make_unique<zenith::TrackAutomationSynchronizer>(*projectState, *engine);

  // Phase 5: Create Wingman command API
  commandAPI = std::make_unique<zenith::CommandAPI>(*projectState, *engine);

  // Phase 7: Create AI bridge client
  aiBridgeClient = std::make_unique<zenith::AIBridgeClient>();

  // Phase 13: Connect project state to engine for automation
  engine->setProjectState(projectState.get());

  // Integration: Create clip synchronizer
  clipSynchronizer = std::make_unique<zenith::ClipSynchronizer>(*projectState, *engine);

  // Add some demo tracks for testing (Phase 9 + existing features)
  projectState->addTrack("Audio 1", "audio");
  projectState->addTrack("MIDI 1", "midi");
  projectState->addTrack("Audio 2", "audio");

  // Create main content (Phase 14: Automation + Phase 10: Mixer + Phase 9:
  // Arranger + Wingman AI)
  mainComponent = std::make_unique<MainComponent>(
      *engine, *commandAPI, *aiBridgeClient, *projectState);

  // Create menu bar
  menuBar = std::make_unique<ZenithMenuBar>(*this);
  setMenuBar(menuBar.get());

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
  setMenuBar(nullptr);
  menuBar.reset();

  // Shutdown audio engine before destroying components
  if (engine)
    engine->shutdown();

  // Clear content
  clearContentComponent();

  DBG("MainWindow destroyed");
}

void MainWindow::closeButtonPressed() {
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

void MainWindow::saveProject()
{
    if (currentProjectFile.existsAsFile())
    {
        projectState->saveToFile(currentProjectFile);
    }
    else
    {
        saveProjectAs();
    }
}

void MainWindow::saveProjectAs()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Save Project As...",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.zth");

    auto chooserFlags = juce::FileBrowserComponent::saveMode |
                        juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{})
            return;

        // Ensure extension
        if (!file.hasFileExtension(".zth"))
            file = file.withFileExtension(".zth");

        if (projectState->saveToFile(file))
        {
            currentProjectFile = file;
            setName("Zenith DAW - " + file.getFileNameWithoutExtension());
        }
    });
}

//==============================================================================
// ZenithMenuBar Implementation
//==============================================================================

MainWindow::ZenithMenuBar::ZenithMenuBar(MainWindow &mainWindow)
    : owner(mainWindow) {}

juce::StringArray MainWindow::ZenithMenuBar::getMenuBarNames() {
  return {"File", "Help"};
}

juce::PopupMenu
MainWindow::ZenithMenuBar::getMenuForIndex(int topLevelMenuIndex,
                                           const juce::String &menuName) {
  juce::PopupMenu menu;

  if (topLevelMenuIndex == 0) // File menu
  {
    menu.addItem(save, "Save", true, false);
    menu.addItem(saveAs, "Save As...", true, false);
    menu.addSeparator();
#if !(JUCE_IOS || JUCE_ANDROID)
    menu.addItem(quit, "Quit", true, false);
#endif
  } else if (topLevelMenuIndex == 1) // Help menu
  {
    menu.addItem(aboutZenith, "About Zenith DAW...", true, false);
  }

  return menu;
}

void MainWindow::ZenithMenuBar::menuItemSelected(int menuItemID,
                                                 int /*topLevelMenuIndex*/) {
  switch (menuItemID) {
  case save:
    owner.saveProject();
    break;

  case saveAs:
    owner.saveProjectAs();
    break;

  case aboutZenith:
    owner.showAboutDialog();
    break;

  case quit:
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
    break;

  default:
    break;
  }
}

