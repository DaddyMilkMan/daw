/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../include/PianoRollEditor.h"
#include "../Source/engine/Track.h"

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng, ProjectState& ps)
    : engine(eng), projectState(ps)
{
    // Set size
    setSize(1400, 800);

    // Status label
    statusLabel.setText("Zenith DAW - Phase 0: Foundation", juce::dontSendNotification);
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
    recordButton.setEnabled(false);  // Phase 1
    addAndMakeVisible(recordButton);

    // Phase 1: Import Audio button
    importButton.setButtonText("Import Audio...");
    importButton.onClick = [this]() {
        handleImportAudio();
    };
    addAndMakeVisible(importButton);

    // Integration: Create ArrangerView
    arrangerView = std::make_unique<ArrangerView>(projectState);
    arrangerView->setOpenPianoRollCallback([this](juce::String trackId, juce::String clipId) {
        openPianoRoll(trackId, clipId);
    });
    addAndMakeVisible(arrangerView.get());

    // Integration: Create automation buttons container
    addAndMakeVisible(automationButtonsContainer);

    // Create automation toggle buttons for demo tracks
    // (In real implementation, would create dynamically as tracks are added)
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
    {
        for (auto track : tracksNode)
        {
            juce::String trackId = track[ProjectState::PROP_ID].toString();
            auto button = std::make_unique<juce::TextButton>("A");
            button->setTooltip("Toggle automation for " + track[ProjectState::PROP_NAME].toString());
            button->onClick = [this, trackId]() {
                bool visible = arrangerView->isTrackAutomationVisible(trackId);
                arrangerView->setTrackAutomationVisible(trackId, !visible);
            };
            automationButtonsContainer.addAndMakeVisible(button.get());
            automationButtons[trackId] = std::move(button);
        }
    }

    // Start timer for CPU monitoring (60 Hz)
    startTimer(16);
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::paint(juce::Graphics& g)
{
    // Background
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

    // Integration: ArrangerView takes remaining space
    auto arrangerBounds = bounds;

    // Automation buttons (left side, 30 pixels wide)
    auto automationButtonArea = arrangerBounds.removeFromLeft(30);
    automationButtonsContainer.setBounds(automationButtonArea);

    // Layout automation buttons vertically
    int buttonY = 0;
    for (auto& [trackId, button] : automationButtons)
    {
        button->setBounds(0, buttonY, 30, 30);
        buttonY += 60;  // Match track height from ArrangerView
    }

    if (arrangerView)
    {
        arrangerView->setBounds(arrangerBounds);
    }
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

    // Phase 13: Connect project state to engine for automation
    engine->setProjectState(projectState.get());

    // Integration: Create clip synchronizer
    clipSynchronizer = std::make_unique<ClipSynchronizer>(*projectState, *engine);

    // Add some demo tracks for testing UI integration
    projectState->addTrack("MIDI Track 1", "midi");
    projectState->addTrack("Audio Track 1", "audio");
    projectState->addTrack("MIDI Track 2", "midi");

    // Create main content
    mainComponent = std::make_unique<MainComponent>(*engine, *projectState);

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

    // Integration: Start clip synchronizer
    // (In real implementation, would start when recording is enabled)
    // clipSynchronizer->start(30);  // 30 Hz update rate

    DBG("MainWindow created and initialized");
}

MainWindow::~MainWindow()
{
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
