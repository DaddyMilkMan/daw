/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../include/ui/ArrangerView.h"
#include "../include/ui/PianoRollEditor.h"

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

    // Test buttons for UI
    addTrackButton.setButtonText("Add MIDI Track");
    addTrackButton.onClick = [this]() {
        auto trackId = projectState.addTrack("MIDI " + juce::String(projectState.getNumTracks() + 1), "midi");
        DBG("Added track: " + trackId);
    };
    addAndMakeVisible(addTrackButton);

    testPianoRollButton.setButtonText("Test Piano Roll");
    testPianoRollButton.onClick = [this]() {
        // Create a test track and clip if none exist
        if (projectState.getNumTracks() == 0)
        {
            auto trackId = projectState.addTrack("Test MIDI", "midi");
            auto clipId = projectState.addClip(trackId, 0.0, 8.0, "Test Clip");
            openPianoRoll(trackId, clipId);
        }
        else
        {
            // Find first MIDI clip
            auto state = projectState.getState();
            auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);
            for (auto track : tracksNode)
            {
                if (track[ProjectState::PROP_TYPE].toString() == "midi")
                {
                    auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
                    if (clipsNode.isValid() && clipsNode.getNumChildren() > 0)
                    {
                        auto clip = clipsNode.getChild(0);
                        openPianoRoll(track[ProjectState::PROP_ID].toString(),
                                    clip[ProjectState::PROP_ID].toString());
                        return;
                    }
                    else
                    {
                        // Create a clip on this track
                        auto trackId = track[ProjectState::PROP_ID].toString();
                        auto clipId = projectState.addClip(trackId, 0.0, 8.0, "Test Clip");
                        openPianoRoll(trackId, clipId);
                        return;
                    }
                }
            }
            DBG("No MIDI tracks found");
        }
    };
    addAndMakeVisible(testPianoRollButton);

    // Create arranger view
    arrangerView = std::make_unique<ArrangerView>(projectState);
    addAndMakeVisible(*arrangerView);

    // Start timer for CPU monitoring (60 Hz)
    startTimer(16);
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::paint(juce::Graphics& g)
{
    // Background (arranger fills most of the space now)
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Top bar (status)
    auto topBar = bounds.removeFromTop(40);
    statusLabel.setBounds(topBar.removeFromLeft(300).reduced(10, 8));

    // Test buttons
    addTrackButton.setBounds(topBar.removeFromLeft(120).reduced(5, 5));
    testPianoRollButton.setBounds(topBar.removeFromLeft(120).reduced(5, 5));

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

    // Arranger view fills the center
    if (arrangerView)
        arrangerView->setBounds(bounds);
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

void MainComponent::openPianoRoll(const juce::String& trackId, const juce::String& clipId)
{
    // Close existing piano roll window if open
    if (pianoRollWindow)
        pianoRollWindow.reset();

    // Create new piano roll editor
    auto pianoRoll = std::make_unique<PianoRollEditor>(projectState, trackId, clipId);

    // Create window
    pianoRollWindow = std::make_unique<juce::DocumentWindow>(
        "Piano Roll - " + clipId,
        juce::Colours::darkgrey,
        juce::DocumentWindow::allButtons
    );

    pianoRollWindow->setContentOwned(pianoRoll.release(), true);
    pianoRollWindow->setResizable(true, false);
    pianoRollWindow->setUsingNativeTitleBar(true);
    pianoRollWindow->centreWithSize(800, 600);
    pianoRollWindow->setVisible(true);

    DBG("Opened piano roll for clip " + clipId);
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
