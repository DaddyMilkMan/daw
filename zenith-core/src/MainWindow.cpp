/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "ui/ArrangerComponent.h"
#include "ui/WingmanPanel.h"
#include "commands/CommandAPI.h"
#include "network/AIBridgeClient.h"

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng, zenith::CommandAPI& api, zenith::AIBridgeClient& aiClient, ProjectState& state)
    : engine(eng), projectState(state)
{
    // Set size
    setSize(1400, 800);

    // Register as key listener for undo/redo shortcuts
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Status label
    statusLabel.setText("Zenith DAW - Phase 7: Wingman AI Integration", juce::dontSendNotification);
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

    // Phase 4: Create arranger component
    arrangerComponent = std::make_unique<ArrangerComponent>(engine);
    addAndMakeVisible(arrangerComponent.get());

    // Phase 7: Create Wingman AI console panel
    wingmanPanel = std::make_unique<WingmanPanel>(api, aiClient);
    addAndMakeVisible(wingmanPanel.get());

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
    // Background (arranger handles its own painting)
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

    // Phase 5: Layout Wingman panel on the right (300px width)
    if (wingmanPanel != nullptr)
    {
        auto wingmanBounds = bounds.removeFromRight(400);
        wingmanPanel->setBounds(wingmanBounds);
    }

    // Phase 4: Layout arranger in remaining space
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

    // Create main content (Phase 7: pass AIBridgeClient for AI mode)
    mainComponent = std::make_unique<MainComponent>(*engine, *commandAPI, *aiBridgeClient, *projectState);

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
