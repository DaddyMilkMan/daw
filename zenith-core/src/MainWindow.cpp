/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng, ProjectState& ps)
    : engine(eng), projectState(ps)
{
    // Set size
    setSize(1400, 800);

    // Phase 12: Create arranger component
    arrangerComponent = std::make_unique<ArrangerComponent>(projectState);
    addAndMakeVisible(arrangerComponent.get());

    // Add keyboard listener for undo/redo
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Status label
    statusLabel.setText("Zenith DAW - Phase 12: Recording UX", juce::dontSendNotification);
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
    recordButton.setClickingTogglesState(true);
    recordButton.onClick = [this]() {
        if (recordButton.getToggleState())
        {
            engine.startRecording();
            DBG("Record button clicked - started recording");
        }
        else
        {
            engine.stopRecording();
            DBG("Record button clicked - stopped recording");
        }
    };
    addAndMakeVisible(recordButton);

    // Start timer for CPU monitoring (60 Hz)
    startTimer(16);
}

MainComponent::~MainComponent()
{
    removeKeyListener(this);
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

    // Center transport buttons
    auto transportSection = bottomBar.reduced(10, 8);
    int buttonWidth = 100;
    int totalWidth = buttonWidth * 3 + 20;  // 3 buttons + spacing
    int startX = transportSection.getCentreX() - totalWidth / 2;

    playButton.setBounds(startX, transportSection.getY(), buttonWidth, transportSection.getHeight());
    stopButton.setBounds(startX + buttonWidth + 10, transportSection.getY(), buttonWidth, transportSection.getHeight());
    recordButton.setBounds(startX + (buttonWidth + 10) * 2, transportSection.getY(), buttonWidth, transportSection.getHeight());

    // Phase 12: Arranger view fills the middle
    if (arrangerComponent != nullptr)
    {
        arrangerComponent->setBounds(bounds);
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

    // Phase 12: Update record button state
    bool isRecording = engine.isRecording();
    if (recordButton.getToggleState() != isRecording)
    {
        recordButton.setToggleState(isRecording, juce::dontSendNotification);
    }

    // Update record button color
    if (isRecording)
    {
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    }
    else
    {
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    }
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
// KeyListener interface
//==============================================================================

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);

    // Cmd/Ctrl+Z: Undo
    if (key.isKeyCode(juce::KeyPress::deleteKey) ||
        (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Z'))
    {
        if (!key.getModifiers().isShiftDown())
        {
            projectState.undo();
            DBG("Undo");
            return true;
        }
        else
        {
            // Cmd/Ctrl+Shift+Z: Redo
            projectState.redo();
            DBG("Redo");
            return true;
        }
    }

    // Cmd/Ctrl+Y: Redo (alternative)
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Y')
    {
        projectState.redo();
        DBG("Redo");
        return true;
    }

    // R: Toggle record
    if (key.getKeyCode() == 'R' && !key.getModifiers().isAnyModifierKeyDown())
    {
        recordButton.triggerClick();
        return true;
    }

    // Space: Play/Stop
    if (key.getKeyCode() == juce::KeyPress::spaceKey && !key.getModifiers().isAnyModifierKeyDown())
    {
        if (engine.isPlaying())
        {
            stopButton.triggerClick();
        }
        else
        {
            playButton.triggerClick();
        }
        return true;
    }

    return false;
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

    // Connect engine to project state
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

    // Phase 12: Create some test tracks for recording
    projectState->addTrack("Audio 1", "audio");
    projectState->addTrack("Audio 2", "audio");
    projectState->addTrack("MIDI 1", "midi");

    // Arm first audio track by default
    auto trackIds = projectState->getTrackIds();
    if (!trackIds.isEmpty())
    {
        projectState->setTrackArmed(trackIds[0], true);
    }

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
