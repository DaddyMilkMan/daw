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

    // Set up menu bar
    setMenuBar(this);

    // Initialize audio engine after window is visible
    engine->initialize();

    DBG("MainWindow created and initialized");
}

MainWindow::~MainWindow()
{
    // Remove menu bar
    setMenuBar(nullptr);

    // Close plugin browser if open
    pluginBrowserWindow.reset();

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
// MenuBarModel interface
//==============================================================================

juce::StringArray MainWindow::getMenuBarNames()
{
    return { "File", "Plugins", "Help" };
}

juce::PopupMenu MainWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)  // File
    {
        menu.addItem(100, "New Project");
        menu.addItem(101, "Open Project...");
        menu.addSeparator();
        menu.addItem(102, "Save Project");
        menu.addItem(103, "Save Project As...");
        menu.addSeparator();
        menu.addItem(104, "Exit");
    }
    else if (topLevelMenuIndex == 1)  // Plugins
    {
        menu.addItem(MenuScanPlugins, "Scan for Plugins...");
        menu.addItem(MenuOpenPluginBrowser, "Open Plugin Browser");
    }
    else if (topLevelMenuIndex == 2)  // Help
    {
        menu.addItem(200, "About Zenith DAW");
    }

    return menu;
}

void MainWindow::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(topLevelMenuIndex);

    switch (menuItemID)
    {
        case MenuScanPlugins:
            scanForPlugins();
            break;

        case MenuOpenPluginBrowser:
            openPluginBrowser();
            break;

        case 104:  // Exit
            closeButtonPressed();
            break;

        case 200:  // About
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "About Zenith DAW",
                "Zenith DAW\nVersion: Phase 0 (Foundation)\n\nA modern JUCE-based DAW",
                "OK");
            break;

        default:
            break;
    }
}

//==============================================================================
// Menu actions
//==============================================================================

void MainWindow::openPluginBrowser()
{
    if (pluginBrowserWindow == nullptr)
    {
        pluginBrowserWindow = std::make_unique<PluginBrowserWindow>(*engine);
    }

    pluginBrowserWindow->setVisible(true);
    pluginBrowserWindow->toFront(true);

    // Refresh to show current track list
    if (auto* browser = pluginBrowserWindow->getBrowserComponent())
    {
        browser->refresh();
    }
}

void MainWindow::scanForPlugins()
{
    // Show progress dialog
    auto* progressWindow = new juce::AlertWindow(
        "Scanning Plugins",
        "Scanning for VST3 plugins...\n\nThis may take a few minutes.",
        juce::AlertWindow::NoIcon);

    progressWindow->addButton("Cancel", 0);
    progressWindow->enterModalState(true, juce::ModalCallbackFunction::create(
        [progressWindow](int result)
        {
            delete progressWindow;
        }));

    // Scan asynchronously
    juce::Thread::launch([this, progressWindow]()
    {
        engine->scanForPlugins([progressWindow](const juce::String& pluginName, float progress)
        {
            // Update progress message
            juce::MessageManager::callAsync([progressWindow, pluginName, progress]()
            {
                if (progressWindow != nullptr)
                {
                    progressWindow->setMessage(
                        "Scanning for VST3 plugins...\n\n" +
                        juce::String(progress * 100.0f, 1) + "%\n\n" +
                        pluginName);
                }
            });
        });

        // Close progress window and show result
        juce::MessageManager::callAsync([this, progressWindow]()
        {
            if (progressWindow != nullptr)
            {
                progressWindow->exitModalState(1);
                delete progressWindow;
            }

            int numPlugins = engine->getKnownPluginList().getNumTypes();

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Scan Complete",
                "Found " + juce::String(numPlugins) + " VST3 plugins.\n\n" +
                "Plugin list saved to:\n" + engine->getPluginListFile().getFullPathName(),
                "OK");

            // Refresh plugin browser if open
            if (pluginBrowserWindow != nullptr)
            {
                if (auto* browser = pluginBrowserWindow->getBrowserComponent())
                {
                    browser->refresh();
                }
            }
        });
    });
}
