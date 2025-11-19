/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "ArrangerComponent.h"
#include "WingmanPanel.h"
#include "InstrumentBrowserPanel.h"
#include "CommandAPI.h"
#include "AIBridgeClient.h"
#include "../include/PianoRollEditor.h"
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng, zenith::CommandAPI& api, zenith::AIBridgeClient& aiClient, ProjectState& state)
    : engine(eng), projectState(state), mixerComponent(state)
{
    // Set size
    setSize(1400, 800);

    // Add mixer component
    addAndMakeVisible(mixerComponent);

    // Register as key listener for undo/redo shortcuts
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Status label
    statusLabel.setText("Zenith DAW - Phase 10: Mixer + Phase 9: Arranger MVP", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(statusLabel);

    // CPU usage label
    cpuLabel.setText("CPU: 0%", juce::dontSendNotification);
    cpuLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(cpuLabel);

    // Audio device label
    audioDeviceLabel.setText("Audio Device: Not initialized", juce::dontSendNotification);
    audioDeviceLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(audioDeviceLabel);

    // C4: Track count label (read-only)
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
    recordButton.setEnabled(false);  // Future: recording UI
    addAndMakeVisible(recordButton);

    // Phase 1: Import Audio button
    importButton.setButtonText("Import Audio...");
    importButton.onClick = [this]() {
        handleImportAudio();
    };
    addAndMakeVisible(importButton);

    // Phase 9: Create ArrangerComponent with interactive clip editing
    arrangerComponent = std::make_unique<ArrangerComponent>(projectState);
    addAndMakeVisible(arrangerComponent.get());

    // Phase 7: Create Wingman AI console panel
    wingmanPanel = std::make_unique<WingmanPanel>(api, aiClient);
    addAndMakeVisible(wingmanPanel.get());

    // Create Instrument Browser Panel
    instrumentBrowserPanel = std::make_unique<zenith::InstrumentBrowserPanel>(engine, projectState);
    addAndMakeVisible(instrumentBrowserPanel.get());

    // Start timer for CPU monitoring (60 Hz)
    startTimer(16);
}

MainComponent::~MainComponent()
{
    removeKeyListener(this);
    stopTimer();
}

bool MainComponent::keyPressed(const juce::KeyPress& key, Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);

    // Ctrl+Z or Cmd+Z for undo
    if (key.isKeyCode(juce::KeyPress::zKey) && key.getModifiers().isCommandDown() && !key.getModifiers().isShiftDown())
    {
        if (projectState.canUndo())
        {
            projectState.undo();
            DBG("Keyboard shortcut: Undo");
            return true;
        }
    }

    // Ctrl+Shift+Z or Cmd+Shift+Z for redo
    if (key.isKeyCode(juce::KeyPress::zKey) && key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown())
    {
        if (projectState.canRedo())
        {
            projectState.redo();
            DBG("Keyboard shortcut: Redo");
            return true;
        }
    }

    // Ctrl+Y or Cmd+Y for redo (alternative)
    if (key.isKeyCode(juce::KeyPress::yKey) && key.getModifiers().isCommandDown())
    {
        if (projectState.canRedo())
        {
            projectState.redo();
            DBG("Keyboard shortcut: Redo (Y)");
            return true;
        }
    }

    return false;  // Key not handled
}

void MainComponent::paint(juce::Graphics& g)
{
    // Background (ArrangerComponent handles its own painting)
    g.fillAll(juce::Colour(0xff1e1e1e));  // Dark grey (LUNA-inspired)
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Top bar (status)
    auto topBar = bounds.removeFromTop(40);
    statusLabel.setBounds(topBar.removeFromLeft(500).reduced(10, 8));

    // C4: Track count label sits on the right side of the top bar (after CPU)
    auto trackCountArea = topBar.removeFromRight(120);
    trackCountLabel.setBounds(trackCountArea.reduced(10, 8));

    cpuLabel.setBounds(topBar.removeFromRight(150).reduced(10, 8));

    // Bottom bar (transport + audio device)
    auto bottomBar = bounds.removeFromBottom(50);

    auto deviceSection = bottomBar.removeFromLeft(400);
    audioDeviceLabel.setBounds(deviceSection.reduced(10, 12));

    // Phase 1: Import button on the left
    auto importSection = bottomBar.removeFromLeft(140);
    importButton.setBounds(importSection.reduced(10, 8));

    // Center transport buttons
    auto transportSection = bottomBar.reduced(10, 8);
    int buttonWidth = 100;
    int totalWidth = buttonWidth * 3 + 20;  // 3 buttons + spacing
    int startX = transportSection.getCentreX() - totalWidth / 2;

    playButton.setBounds(startX, transportSection.getY(), buttonWidth, transportSection.getHeight());
    stopButton.setBounds(startX + buttonWidth + 10, transportSection.getY(), buttonWidth, transportSection.getHeight());
    recordButton.setBounds(startX + (buttonWidth + 10) * 2, transportSection.getY(), buttonWidth, transportSection.getHeight());

    // Phase 10: Mixer panel at bottom (above transport bar)
    auto mixerHeight = 220;
    auto mixerArea = bounds.removeFromBottom(mixerHeight);
    mixerComponent.setBounds(mixerArea);

    // Phase 7: Layout Wingman panel on the right (400px width)
    if (wingmanPanel != nullptr)
    {
        auto wingmanBounds = bounds.removeFromRight(400);
        wingmanPanel->setBounds(wingmanBounds);
    }

    // Layout Instrument Browser panel on the left (300px width)
    if (instrumentBrowserPanel != nullptr)
    {
        auto browserBounds = bounds.removeFromLeft(300);
        instrumentBrowserPanel->setBounds(browserBounds);
    }

    // Phase 9: ArrangerComponent takes the remaining central area
    if (arrangerComponent != nullptr)
        arrangerComponent->setBounds(bounds);
}

void MainComponent::timerCallback()
{
    // Update CPU usage
    double cpuUsage = engine.getCpuUsage();
    cpuLabel.setText("CPU: " + juce::String(cpuUsage, 1) + "%", juce::dontSendNotification);

    // Update audio device info
    auto deviceInfo = engine.getAudioDeviceInfo();
    audioDeviceLabel.setText("Audio: " + deviceInfo, juce::dontSendNotification);

    // C4: Update track count (dirty-checked)
    refreshTrackCountLabel();
}

//==============================================================================
// C4: Track count monitoring (read-only, dirty-checked)
//==============================================================================

void MainComponent::refreshTrackCountLabel()
{
    // Message-thread read only
    const int count = engine.getNumTracks();
    if (count == lastTrackCount_)
        return;

    lastTrackCount_ = count;
    // No heavy formatting, no repaint storm
    trackCountLabel.setText("Tracks: " + juce::String(count), juce::dontSendNotification);
}

//==============================================================================
// Integration: Piano roll opener
//==============================================================================

void MainComponent::openPianoRoll(const juce::String& trackId, const juce::String& clipId)
{
    DBG("MainComponent: Opening piano roll for " + trackId + "/" + clipId);

    // Create new piano roll editor window
    // Note: Window deletes itself when closed (see PianoRollEditor::closeButtonPressed)
    new PianoRollEditor(projectState, trackId, clipId);
}

//==============================================================================
// Phase 1: Audio Import
//==============================================================================

void MainComponent::handleImportAudio()
{
    // Create file chooser for audio files
    auto chooser = std::make_shared<juce::FileChooser>(
        "Import Audio File",
        juce::File{},
        "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg");

    // Open file chooser (async)
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (!file.existsAsFile())
            return;

        DBG("Importing audio file: " + file.getFullPathName());

        // Ensure we have at least one track
        if (engine.getNumTracks() == 0)
        {
            DBG("Creating first track for audio import");
            engine.addTestTracks(1);
        }

        // Get the first track
        const auto& tracks = engine.tracks();
        if (tracks.empty())
        {
            DBG("ERROR: Failed to get track after creation");
            return;
        }

        auto* track = tracks[0].get();
        if (track == nullptr)
        {
            DBG("ERROR: Track is null");
            return;
        }

        // Create a new clip
        auto clip = std::make_unique<zenith::Track::Clip>();
        clip->setType(zenith::Track::Clip::Type::Audio);
        clip->setName(file.getFileNameWithoutExtension());

        // Load audio file through pool (message thread - safe to do I/O)
        auto& pool = engine.getAudioFilePool();
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

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    // Create audio engine first
    engine = std::make_unique<Engine>();

    // Create project state
    projectState = std::make_unique<ProjectState>();

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

    // Create main content (Phase 10: Mixer + Phase 9: Arranger + Wingman AI)
    mainComponent = std::make_unique<MainComponent>(*engine, *commandAPI, *aiBridgeClient, *projectState);

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

    DBG("MainWindow created and initialized");
}

MainWindow::~MainWindow()
{
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

void MainWindow::closeButtonPressed()
{
    // TODO: Check for unsaved changes
    // TODO: Show save dialog if needed

    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::showAboutDialog()
{
    juce::String aboutMessage;
    aboutMessage << "Zenith DAW\n\n";
    aboutMessage << "A professional digital audio workstation\n\n";
    aboutMessage << "Version: 0.1.0\n";
    aboutMessage << "Built with JUCE 8.0.9\n\n";
    aboutMessage << "For documentation and installation instructions, see:\n";
    aboutMessage << "• docs/README.md\n";
    aboutMessage << "• docs/INSTALL_WINDOWS.md";

    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "About Zenith DAW",
        aboutMessage,
        "OK"
    );
}

//==============================================================================
// ZenithMenuBar Implementation
//==============================================================================

MainWindow::ZenithMenuBar::ZenithMenuBar(MainWindow& mainWindow)
    : owner(mainWindow)
{
}

juce::StringArray MainWindow::ZenithMenuBar::getMenuBarNames()
{
    return { "File", "Help" };
}

juce::PopupMenu MainWindow::ZenithMenuBar::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)  // File menu
    {
        #if ! (JUCE_IOS || JUCE_ANDROID)
            menu.addItem(quit, "Quit", true, false);
        #endif
    }
    else if (topLevelMenuIndex == 1)  // Help menu
    {
        menu.addItem(aboutZenith, "About Zenith DAW...", true, false);
    }

    return menu;
}

void MainWindow::ZenithMenuBar::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    switch (menuItemID)
    {
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
