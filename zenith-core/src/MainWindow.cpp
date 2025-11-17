/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng, ProjectState& state)
    : engine(eng), projectState(state)
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

    // Piano Roll button
    pianoRollButton.setButtonText("Open Demo Piano Roll");
    pianoRollButton.onClick = [this]() {
        openDemoPianoRoll();
    };
    addAndMakeVisible(pianoRollButton);

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

    // Draw welcome message
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(48.0f, juce::Font::bold));

    auto bounds = getLocalBounds().reduced(40);
    g.drawText("Welcome to Zenith DAW",
               bounds.removeFromTop(100),
               juce::Justification::centred,
               true);

    // Draw phase info
    g.setFont(juce::Font(20.0f));
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Phase 0: Foundation - Basic audio engine operational",
               bounds.removeFromTop(40),
               juce::Justification::centred,
               true);

    // Draw feature list
    g.setFont(juce::Font(16.0f));
    g.setColour(juce::Colours::grey);

    auto featuresBounds = bounds.removeFromTop(200).reduced(100, 0);
    juce::String features =
        "✓ JUCE 8.0.9 audio engine\n"
        "✓ Audio device management\n"
        "✓ Transport controls (play/stop)\n"
        "✓ CPU monitoring\n"
        "✓ Project state management (ValueTree)\n"
        "\n"
        "Coming in Phase 1:\n"
        "• Multi-track recording\n"
        "• VST3 plugin hosting\n"
        "• MIDI support\n"
        "• Timeline view";

    g.drawMultiLineText(features,
                       featuresBounds.getX(),
                       featuresBounds.getY(),
                       featuresBounds.getWidth());
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

    // Center transport buttons
    auto transportSection = bottomBar.reduced(10, 8);
    int buttonWidth = 100;
    int totalWidth = buttonWidth * 3 + 20;  // 3 buttons + spacing
    int startX = transportSection.getCentreX() - totalWidth / 2;

    playButton.setBounds(startX, transportSection.getY(), buttonWidth, transportSection.getHeight());
    stopButton.setBounds(startX + buttonWidth + 10, transportSection.getY(), buttonWidth, transportSection.getHeight());
    recordButton.setBounds(startX + (buttonWidth + 10) * 2, transportSection.getY(), buttonWidth, transportSection.getHeight());

    // Piano Roll button on the right
    pianoRollButton.setBounds(bottomBar.removeFromRight(200).reduced(10, 8));

    // Piano Roll editor (if visible)
    if (pianoRollEditor)
    {
        // Take the center area
        pianoRollEditor->setBounds(bounds.reduced(10));
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
// Piano Roll
//==============================================================================

void MainComponent::openPianoRollForClip(const juce::String& trackId, const juce::String& clipId)
{
    DBG("MainComponent: Opening Piano Roll for track " + trackId + ", clip " + clipId);

    // Create piano roll editor
    pianoRollEditor = std::make_unique<PianoRollEditor>(projectState, trackId, clipId);
    addAndMakeVisible(pianoRollEditor.get());

    // Trigger resize
    resized();
}

void MainComponent::openDemoPianoRoll()
{
    // Create a demo MIDI track and clip if they don't exist
    // This is just for testing purposes

    // Check if we have a MIDI track
    juce::String trackId;
    juce::String clipId;

    // For simplicity, just create a new track and clip each time
    trackId = projectState.addTrack("MIDI Demo", "midi");
    DBG("Created demo track: " + trackId);

    // Create a clip manually by accessing the state tree
    auto track = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (track.isValid())
    {
        for (auto t : track)
        {
            if (t[ProjectState::PROP_ID].toString() == trackId)
            {
                auto clipsNode = t.getChildWithName(ProjectState::ID_CLIPS);
                if (clipsNode.isValid())
                {
                    juce::ValueTree clip(ProjectState::ID_CLIP);
                    clipId = "clip_demo_" + juce::String(juce::Random::getSystemRandom().nextInt());
                    clip.setProperty(ProjectState::PROP_ID, clipId, nullptr);
                    clip.setProperty(ProjectState::PROP_START, 0.0, nullptr);
                    clip.setProperty(ProjectState::PROP_LENGTH, 16.0, nullptr);
                    clip.setProperty(ProjectState::PROP_TYPE, "midi", nullptr);

                    clipsNode.appendChild(clip, &projectState.getUndoManager());
                    DBG("Created demo clip: " + clipId);
                }
                break;
            }
        }
    }

    if (!trackId.isEmpty() && !clipId.isEmpty())
    {
        openPianoRollForClip(trackId, clipId);
    }
    else
    {
        DBG("Failed to create demo track/clip");
    }
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
