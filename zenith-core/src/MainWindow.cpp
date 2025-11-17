/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"

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
    recordButton.setEnabled(false);  // Phase 1
    addAndMakeVisible(recordButton);

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

    // Phase 13: Connect project state to engine for automation
    engine->setProjectState(projectState.get());

    // Create main content
    mainComponent = std::make_unique<MainComponent>(*engine);

    // Create menu bar
    menuBarModel = std::make_unique<MenuBarModel>(*this);
    menuBar.setModel(menuBarModel.get());
    setMenuBar(&menuBar);

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
    // Remove menu bar before destroying components
    setMenuBar(nullptr);
    menuBarModel.reset();

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

//==============================================================================
// Menu Bar Implementation
//==============================================================================

MainWindow::MenuBarModel::MenuBarModel(MainWindow& owner)
    : owner_(owner)
{
}

juce::StringArray MainWindow::MenuBarModel::getMenuBarNames()
{
    return { "File", "Edit", "View", "Help" };
}

juce::PopupMenu MainWindow::MenuBarModel::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0) // File menu
    {
        menu.addSubMenu("Export", []() {
            juce::PopupMenu exportMenu;
            exportMenu.addItem(MainWindow::exportMixdownID, "Export Mixdown (WAV...)");
            return exportMenu;
        }());
        menu.addSeparator();
        menu.addItem(MainWindow::quitID, "Quit", true, false);
    }
    else if (topLevelMenuIndex == 1) // Edit menu
    {
        menu.addItem(2000, "Undo", false);
        menu.addItem(2001, "Redo", false);
    }
    else if (topLevelMenuIndex == 2) // View menu
    {
        menu.addItem(3000, "Mixer", false);
        menu.addItem(3001, "Arrangement", false);
    }
    else if (topLevelMenuIndex == 3) // Help menu
    {
        menu.addItem(4000, "About", false);
    }

    return menu;
}

void MainWindow::MenuBarModel::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(topLevelMenuIndex);

    switch (menuItemID)
    {
        case MainWindow::exportMixdownID:
            owner_.handleExportMixdown();
            break;

        case MainWindow::quitID:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;

        default:
            break;
    }
}

void MainWindow::handleExportMixdown()
{
    DBG("MainWindow: Export mixdown requested");

    // TODO: Replace with FileChooser for proper file selection
    // For now, use a hard-coded path with timestamp to avoid overwriting
    auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    auto outputPath = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
        .getChildFile("zenith_export_" + timestamp + ".wav");

    DBG("MainWindow: Exporting to " + outputPath.getFullPathName());

    // Show a message that export is starting
    // (In a production app, this would be a progress dialog)
    // For now, just log it

    // Call engine export (blocking)
    juce::String errorMessage;
    bool success = engine->exportProjectToWav(
        outputPath,
        0.0,    // Start at beginning
        0.0,    // Auto-detect end (or default to 10s if no clips)
        errorMessage);

    // Show result to user
    if (success)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Export Complete",
            "Project exported successfully to:\n" + outputPath.getFullPathName(),
            "OK");
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Export Failed",
            "Failed to export project:\n" + errorMessage,
            "OK");
    }
}
