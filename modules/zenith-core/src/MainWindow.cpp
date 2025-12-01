/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "commands/CommandAPI.h"
#include "engine/Clip.h"
#include "engine/Track.h"
#include "network/AIBridgeClient.h"
// #include "ui/ArrangerComponent.h"  // Using ArrangerView instead
// #include "ui/InstrumentBrowserPanel.h"  // TODO: Implement this component
#ifdef ZENITH_USE_SKIA
#include "ui/MainLayoutComponent.h"
#endif
// #include "ui/WingmanPanel.h"  // TODO: Implement WingmanPanel
#include "../include/PianoRollEditor.h"

// #include "SimpleLogger.h"  // Disabled: DebugLogOverlay doesn't exist

#ifdef ZENITH_USE_SKIA
#include "ui/skia/SkiaComponent.h"
#include <include/core/SkFont.h>
#include <include/core/SkImage.h>
#include <include/core/SkImageInfo.h>
#include <include/core/SkPixmap.h>
#include <include/core/SkSamplingOptions.h>
#include <include/core/SkSurface.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkColor.h>
#include <fstream>

static void logToDisk(const std::string& msg) {
    std::ofstream outfile;
    outfile.open("C:\\zenith\\daw\\debug_log.txt", std::ios_base::app);
    outfile << msg << std::endl;
}
#else
static void logToDisk(const std::string& msg) {}
#endif

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine &eng, zenith::CommandAPI &api,
                             zenith::AIBridgeClient &aiClient,
                             ProjectState &state)
    : engine(eng), projectState(state)
#ifndef ZENITH_USE_SKIA
      ,
      mixerComponent(state)
#endif
{
  // Register as key listener for undo/redo shortcuts
  addKeyListener(this);
  addMouseListener(this, true); // Intercept mouse events recursively
  setWantsKeyboardFocus(true);

  // Add Debug Overlay
  // addChildComponent(&zenith::DebugLogOverlay::getInstance());  // Disabled: DebugLogOverlay doesn't exist

  // Show Console
  // showDebugConsole();  // Disabled: DebugLogOverlay doesn't exist

  setSize(1400, 800);

  DBG("========================================");
  DBG("MainComponent Constructor - Modern DAW Layout");
  DBG("========================================");

#ifdef ZENITH_USE_SKIA
  logToDisk(">>> ZENITH_USE_SKIA IS DEFINED - MODERN SKIA DAW LAYOUT BRANCH EXECUTING <<<");

  // Initialize Skia rendering system
  // Skia initialization is handled by
  // SkiaMainWindowIntegration::newOpenGLContextCreated

  // Instantiate the SkiaRenderer
  logToDisk("→ Initializing SkiaRenderer...");
  // ============================================================================
  // Create Modern DAW Layout Panels
  // ============================================================================

  // Top: Transport Bar
  logToDisk("→ Creating TransportBar...");
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
  // Right: AI Assistant Panel (Wingman) - Pure Skia
    if (mainLayout) {
      mainLayout->toggleView();
      DBG("View toggled via MainLayout");
    }
  };

  // Start animation timer (SkiaMainWindowIntegration handles this)
  logToDisk("✓ Animation timer managed by SkiaMainWindowIntegration");
  
  // Initialize Skia context (starts rendering thread)
  logToDisk("→ Calling initializeSkia()...");
  initializeSkia(); // Enable Skia rendering
  logToDisk("✓ initializeSkia() returned");
  logToDisk("✓ MainComponent Constructor COMPLETE");

#else
  // ============================================================================
  // JUCE Fallback Layout (Legacy)
  // ============================================================================

  DBG("ZENITH_USE_SKIA is NOT DEFINED - Using JUCE fallback layout");

  // Status label
  statusLabel.setText("Zenith DAW - JUCE Fallback Mode",
                      juce::dontSendNotification);
  statusLabel.setJustificationType(juce::Justification::centredLeft);
  statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
  addAndMakeVisible(statusLabel);

  // CPU usage label
  cpuLabel.setText("CPU: 0%", juce::dontSendNotification);
  cpuLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(cpuLabel);

  // Audio device label
  audioDeviceLabel.setText("Audio Device: Not initialized",
                           juce::dontSendNotification);
  audioDeviceLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(audioDeviceLabel);

  // Track count label
  trackCountLabel.setText("Tracks: 0", juce::dontSendNotification);
  trackCountLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(trackCountLabel);

  // Transport buttons
  playButton.setButtonText("Play");
  playButton.onClick = [this]() {
    engine.play();
    DBG("Play button clicked");
  };
  addAndMakeVisible(playButton);

  stopButton.setButtonText("Stop");
  stopButton.onClick = [this]() {
    engine.stop();
    DBG("Stop button clicked");
  };
  addAndMakeVisible(stopButton);

  recordButton.setButtonText("Record");
  recordButton.onClick = [this]() {
    engine.toggleRecording();
    bool isRecording = engine.isRecording();
    recordButton.setColour(juce::TextButton::buttonColourId,
                           isRecording ? juce::Colours::red
                                       : juce::Colours::darkgrey);
    DBG((isRecording ? "Recording started" : "Recording stopped"));
  };
  addAndMakeVisible(recordButton);

  // Import Audio button
  importButton.setButtonText("Import Audio...");
  importButton.onClick = [this]() { handleImportAudio(); };
  addAndMakeVisible(importButton);

  // Virtual MIDI Keyboard toggle button
  virtualKeyboardButton.setButtonText("🎹 Keyboard (M)");
  virtualKeyboardButton.setClickingTogglesState(true);
  virtualKeyboardButton.onClick = [this]() {
    virtualKeyboardVisible = virtualKeyboardButton.getToggleState();
    if (midiKeyboard)
      midiKeyboard->setVisible(virtualKeyboardVisible);
    resized();
  };
  addAndMakeVisible(virtualKeyboardButton);

  // Mixer component
  addAndMakeVisible(mixerComponent);

  // Arranger component
  arrangerComponent =
      std::make_unique<ArrangerView>(*engine.getProjectState());
  addAndMakeVisible(arrangerComponent.get());

  // Wingman panel - TODO: Implement WingmanPanel
  // wingmanPanel = std::make_unique<WingmanPanel>(api, aiClient);
  // addAndMakeVisible(wingmanPanel.get());

  // Instrument Browser - TODO: Implement InstrumentBrowserPanel
  // instrumentBrowserPanel =
  //     std::make_unique<zenith::InstrumentBrowserPanel>(engine, projectState);
  // addAndMakeVisible(instrumentBrowserPanel.get());

  // Virtual MIDI Keyboard
  midiKeyboard = std::make_unique<juce::MidiKeyboardComponent>(
      midiKeyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);
  midiKeyboard->setVisible(false);
  addAndMakeVisible(midiKeyboard.get());

  // Start timer for CPU monitoring
  startTimer(16);
#endif

  DBG("========================================");
  DBG("MainComponent Constructor COMPLETE");
  DBG("========================================");
}

MainComponent::~MainComponent() {
  DBG("MainComponent Destructor called");
  removeKeyListener(this);

#ifndef ZENITH_USE_SKIA
  stopTimer();
#endif
}

bool MainComponent::keyPressed(const juce::KeyPress &key,
                               Component *originatingComponent) {
  juce::ignoreUnused(originatingComponent);

  // Ctrl+Z or Cmd+Z for undo
  if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown() &&
      !key.getModifiers().isShiftDown()) {
    if (projectState.canUndo()) {
      projectState.undo();
      DBG("Keyboard shortcut: Undo");
      return true;
    }
  }

  // Ctrl+Shift+Z or Cmd+Shift+Z for redo
  if (key.getTextCharacter() == 'Z' && key.getModifiers().isCommandDown() &&
      key.getModifiers().isShiftDown()) {
    if (projectState.canRedo()) {
      projectState.redo();
      DBG("Keyboard shortcut: Redo");
      return true;
    }
  }

  // Ctrl+Y or Cmd+Y for redo (alternative)
  if (key.getTextCharacter() == 'y' && key.getModifiers().isCommandDown()) {
    if (projectState.canRedo()) {
      projectState.redo();
      DBG("Keyboard shortcut: Redo (Y)");
      return true;
    }
  }

  // M key: Toggle virtual MIDI keyboard (like Ableton Live)
  if (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M') {
#ifndef ZENITH_USE_SKIA
    virtualKeyboardButton.setToggleState(
        !virtualKeyboardButton.getToggleState(), juce::sendNotification);
#else
    if (bottomBar) {
      bottomBar->setKeyboardVisible(!bottomBar->isKeyboardVisible());
      resized();
    }
#endif
    DBG("Keyboard shortcut: Toggle Virtual MIDI Keyboard (M)");
    return true;
  }

#ifdef ZENITH_USE_SKIA
  // Tab key: Toggle between Session View and Arranger View
  if (key == juce::KeyPress::tabKey &&
      !key.getModifiers().isAnyModifierKeyDown()) {
    if (transportBar && transportBar->onViewToggleClicked) {
      transportBar->onViewToggleClicked();
      DBG("Keyboard shortcut: Toggle Session/Arranger View (Tab)");
      return true;
    }
  }
#endif

  return false; // Key not handled
}

void MainComponent::paint(juce::Graphics &g) {
#ifdef ZENITH_USE_SKIA
  logToDisk("MainComponent::paint() called");
  // Delegate to base class which handles initialization status
  SkiaMainWindowIntegration::paint(g);
#else
// ...
#endif
}

#ifdef ZENITH_USE_SKIA
void MainComponent::renderSkia(SkCanvas* canvas) {
  if (!canvas) return;

  // Lock the message manager to safely access component state and ValueTrees
  // This is necessary because renderSkia runs on the OpenGL thread
  juce::MessageManagerLock mmLock;
  if (!mmLock.lockWasGained()) return;

  // logToDisk("MainComponent::renderSkia called"); // Commented out to reduce spam
  
  // Draw background (Dark Grey for DAW look)
  canvas->clear(SkColorSetRGB(18, 18, 18)); 

  // Draw all Skia UI panels
  // Each panel translates the canvas to its local coordinate space

  // Top: Transport Bar
  if (transportBar) {
    canvas->save();
    auto bounds = transportBar->getBounds();
    canvas->translate(bounds.getX(), bounds.getY());
    transportBar->drawSkia(canvas);
    canvas->restore();
  }

  // Center: Main Layout (Browser + Session/Arranger)
  if (mainLayout) {
    canvas->save();
    // MainLayoutComponent handles its own translation if we pass the canvas, 
    // but here we are in MainComponent coordinates.
    // MainLayoutComponent::drawSkia expects to be called, but it doesn't take bounds.
    // It assumes it's drawing at (0,0) of its local bounds?
    // Let's check MainLayoutComponent::drawSkia implementation.
    // It translates for its children: canvas->translate(bounds.getX(), bounds.getY());
    // So if we are in MainComponent, we shouldn't translate for MainLayout?
    // Wait, MainLayoutComponent::drawSkia translates for ITS children relative to ITSELF.
    // So MainComponent must translate to MainLayout's position.
    
    auto bounds = mainLayout->getBounds();
    // canvas->translate(bounds.getX(), bounds.getY()); // MainLayout is usually at 0,0 or below transport?
    // Let's check resized() in MainComponent.
    // mainLayout->setBounds(bounds); where bounds is the remaining area.
    // So yes, we need to translate.
    
    // However, MainLayoutComponent::drawSkia takes void* canvas.
    // It doesn't seem to do any translation for itself.
    // So we should translate here.
    
    // Wait, MainLayoutComponent::drawSkia implementation:
    // canvas->translate((float)bounds.getX(), (float)bounds.getY());
    // This uses `arrangerComponent_->getBounds()`.
    // ArrangerComponent is a child of MainLayoutComponent.
    // So `arrangerComponent_->getBounds()` is relative to `MainLayoutComponent`.
    // So `MainLayoutComponent::drawSkia` assumes the canvas is already transformed to `MainLayoutComponent`'s origin.
    
    // So in MainComponent, we MUST translate to MainLayoutComponent's origin.
    
    // But wait, MainLayoutComponent::drawSkia implementation I just wrote:
    // canvas->translate((float)bounds.getX(), (float)bounds.getY());
    // This translates by the child's position.
    
    // So yes, here in MainComponent, we translate to MainLayout's position.
    
    canvas->translate((float)bounds.getX(), (float)bounds.getY());
    mainLayout->drawSkia(canvas);
    canvas->restore();
  }

  // Right: Scratch Pads + Wingman Console
  if (rightSidePanel) {
    canvas->save();
    auto bounds = rightSidePanel->getBounds();
    canvas->translate(bounds.getX(), bounds.getY());
    rightSidePanel->drawSkia(canvas);
    canvas->restore();
  }

  // Bottom: Piano Keyboard + Mixer Strip
  if (bottomBar) {
    canvas->save();
    auto bounds = bottomBar->getBounds();
    canvas->translate(bounds.getX(), bounds.getY());
    bottomBar->drawSkia(canvas);
    canvas->restore();
  }
}
#endif

// ...

void MainComponent::resized() {
  auto bounds = getLocalBounds();
  logToDisk("MainComponent::resized() called - Bounds: " + std::to_string(bounds.getWidth()) + "x" + std::to_string(bounds.getHeight()));
  // ...
  DBG("MainComponent::resized() called - Total bounds: " +
      juce::String(bounds.getWidth()) + "x" + juce::String(bounds.getHeight()));

#ifdef ZENITH_USE_SKIA
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

#else
  // ============================================================================
  // JUCE Fallback Layout
  // ============================================================================

  // Top bar (status)
  auto topBar = bounds.removeFromTop(40);
  statusLabel.setBounds(topBar.removeFromLeft(500).reduced(10, 8));

  auto trackCountArea = topBar.removeFromRight(120);
  trackCountLabel.setBounds(trackCountArea.reduced(10, 8));

  cpuLabel.setBounds(topBar.removeFromRight(150).reduced(10, 8));

  // Bottom bar (transport + audio device)
  auto bottomBar = bounds.removeFromBottom(50);

  auto deviceSection = bottomBar.removeFromLeft(400);
  audioDeviceLabel.setBounds(deviceSection.reduced(10, 12));

  auto importSection = bottomBar.removeFromLeft(140);
  importButton.setBounds(importSection.reduced(10, 8));

  auto keyboardButtonSection = bottomBar.removeFromLeft(160);
  virtualKeyboardButton.setBounds(keyboardButtonSection.reduced(10, 8));

  // Center transport buttons
  auto transportSection = bottomBar.reduced(10, 8);
  int buttonWidth = 100;
  int totalWidth = buttonWidth * 3 + 20;
  int startX = transportSection.getCentreX() - totalWidth / 2;

  playButton.setBounds(startX, transportSection.getY(), buttonWidth,
                       transportSection.getHeight());
  stopButton.setBounds(startX + buttonWidth + 10, transportSection.getY(),
                       buttonWidth, transportSection.getHeight());
  recordButton.setBounds(startX + (buttonWidth + 10) * 2,
                         transportSection.getY(), buttonWidth,
                         transportSection.getHeight());

  // Virtual MIDI Keyboard
  if (virtualKeyboardVisible && midiKeyboard) {
    auto keyboardArea = bounds.removeFromBottom(80);
    midiKeyboard->setBounds(keyboardArea);
  }

  // Mixer panel
  auto mixerArea = bounds.removeFromBottom(220);
  mixerComponent.setBounds(mixerArea);

  // Wingman panel (right) - TODO: Implement WingmanPanel
  // if (wingmanPanel) {
  //   auto wingmanBounds = bounds.removeFromRight(400);
  //   wingmanPanel->setBounds(wingmanBounds);
  // }

  // Instrument Browser (left) - TODO: Implement InstrumentBrowserPanel
  // if (instrumentBrowserPanel) {
  //   auto browserBounds = bounds.removeFromLeft(300);
  //   instrumentBrowserPanel->setBounds(browserBounds);
  // }

  // Arranger Component (center)
  if (arrangerComponent)
    arrangerComponent->setBounds(bounds);
#endif
}

void MainComponent::mouseDown(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    DBG("MainComponent::mouseDown");
}

#ifndef ZENITH_USE_SKIA
void MainComponent::timerCallback() {
  // JUCE fallback timer updates
  double cpuUsage = engine.getCpuUsage();
  cpuLabel.setText("CPU: " + juce::String(cpuUsage, 1) + "%",
                   juce::dontSendNotification);

  auto deviceInfo = engine.getAudioDeviceInfo();
  audioDeviceLabel.setText("Audio: " + deviceInfo, juce::dontSendNotification);

  refreshTrackCountLabel();

  if (virtualKeyboardVisible) {
    juce::MidiBuffer midiMessages;
    midiKeyboardState.processNextMidiBuffer(midiMessages, 0, 16, true);

    for (const auto metadata : midiMessages) {
      auto message = metadata.getMessage();
      engine.handleIncomingMidiMessage(nullptr, message);
    }
  }
}
#endif

//==============================================================================
// C4: Track count monitoring (read-only, dirty-checked)
//==============================================================================

void MainComponent::refreshTrackCountLabel() {
  // Message-thread read only
  const int count = engine.getNumTracks();
  if (count == lastTrackCount_)
    return;

  lastTrackCount_ = count;
  // No heavy formatting, no repaint storm
#ifndef ZENITH_USE_SKIA
  trackCountLabel.setText("Tracks: " + juce::String(count),
                          juce::dontSendNotification);
#endif
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
  new PianoRollEditor(projectState, trackId, clipId);
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
  engine = std::make_unique<Engine>();

  // Create project state
  projectState = std::make_unique<ProjectState>();

  // Phase 13: Create automation synchronizer
  automationSync =
      std::make_unique<TrackAutomationSynchronizer>(*projectState, *engine);

  // Phase 5: Create Wingman command API
  commandAPI = std::make_unique<zenith::CommandAPI>(*engine, *projectState);

  // Phase 7: Create AI bridge client
  aiBridgeClient = std::make_unique<zenith::AIBridgeClient>();

  // Phase 13: Connect project state to engine for automation
  engine->setProjectState(projectState.get());

  // Integration: Create clip synchronizer
  clipSynchronizer = std::make_unique<ClipSynchronizer>(*projectState, *engine);

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

  setVisible(true);

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
  setContentOwned(nullptr, true);

  DBG("MainWindow destroyed");
}

void MainWindow::closeButtonPressed() {
  // TODO: Check for unsaved changes
  // TODO: Show save dialog if needed

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
