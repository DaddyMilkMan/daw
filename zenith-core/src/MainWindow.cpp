/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../Source/engine/Track.h"  // U3: For track arm UI

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng)
    : engine(eng)
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
    recordButton.onClick = [this]() {
        // Toggle recording
        if (engine.isRecording())
        {
            engine.stopRecording();
            DBG("Record button clicked: Stop");
        }
        else
        {
            engine.startRecording();
            DBG("Record button clicked: Start");
        }
    };
    addAndMakeVisible(recordButton);

    // U3: Create Track button
    createTrackButton.setButtonText("+ Track");
    createTrackButton.onClick = [this]() {
        engine.addTestTracks(1);
        rebuildTrackList();
        DBG("Created new track");
    };
    addAndMakeVisible(createTrackButton);

    // U3: Track list viewport
    trackListViewport.setViewedComponent(&trackListContent, false);
    addAndMakeVisible(trackListViewport);

    // Build initial track list
    rebuildTrackList();

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

    // U3: Left panel for tracks (200px wide)
    auto leftPanel = bounds.removeFromLeft(250);

    // Create Track button at top of left panel
    createTrackButton.setBounds(leftPanel.removeFromTop(40).reduced(10, 5));

    // Track list viewport takes remaining space
    trackListViewport.setBounds(leftPanel);
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

    // U3: Update recording button state
    if (engine.isRecording())
    {
        recordButton.setButtonText("Recording...");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    }
    else
    {
        recordButton.setButtonText("Record");
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
// U3: Track list UI
//==============================================================================

void MainComponent::rebuildTrackList()
{
    // Clear existing track rows
    trackRows_.clear();

    const auto& tracks = engine.tracks();
    const int rowHeight = 40;
    int yPos = 0;

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto* track = tracks[i].get();
        if (track == nullptr)
            continue;

        // Create a container for this track row
        auto trackRow = std::make_unique<juce::Component>();

        // Track name label
        auto* nameLabel = new juce::Label();
        nameLabel->setText(track->getName(), juce::dontSendNotification);
        nameLabel->setBounds(5, 5, 120, 30);
        trackRow->addAndMakeVisible(nameLabel);

        // Arm toggle button
        auto* armButton = new juce::TextButton();
        armButton->setButtonText(track->isArmed() ? "ARM" : "arm");
        armButton->setClickingTogglesState(true);
        armButton->setToggleState(track->isArmed(), juce::dontSendNotification);
        armButton->setBounds(130, 5, 60, 30);

        // Color armed button red
        if (track->isArmed())
            armButton->setColour(juce::TextButton::buttonColourId, juce::Colours::red);

        // Wire up toggle
        armButton->onClick = [track, armButton]() {
            bool isArmed = armButton->getToggleState();
            const_cast<zenith::Track*>(track)->setArmed(isArmed);

            // Update button appearance
            armButton->setButtonText(isArmed ? "ARM" : "arm");
            armButton->setColour(juce::TextButton::buttonColourId,
                                isArmed ? juce::Colours::red : juce::Colours::darkgrey);

            DBG("Track " + track->getName() + " armed: " + juce::String(isArmed ? "YES" : "NO"));
        };

        trackRow->addAndMakeVisible(armButton);

        // Position the row
        trackRow->setBounds(0, yPos, 240, rowHeight);
        trackListContent.addAndMakeVisible(trackRow.get());

        trackRows_.push_back(std::move(trackRow));
        yPos += rowHeight;
    }

    // Set content size
    trackListContent.setSize(240, yPos);
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
    mainComponent = std::make_unique<MainComponent>(*engine);

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
