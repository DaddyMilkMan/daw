/**
 * @file MainComponent.cpp
 * @brief Implementation of main UI component
 */

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(Engine& eng)
    : engine(eng),
      editorState(eng),
      arrangerComponent(editorState),
      transportBar(eng)
{
    // Apply custom LookAndFeel to this component and all children
    setLookAndFeel(&zenithLookAndFeel);

    // Set size
    setSize(1400, 800);

    // Add all UI components
    addAndMakeVisible(topBar);
    addAndMakeVisible(sidebar);
    addAndMakeVisible(arrangerComponent);
    addAndMakeVisible(transportBar);

    // Setup callbacks between components
    setupCallbacks();

    // W5: Inject stress test data (100 tracks × 50 clips)
    injectTestSessionData();

    #if JUCE_DEBUG
        // W6.1: Initialize ApplicationProperties for HUD persistence
        juce::PropertiesFile::Options options;
        options.applicationName = "ZenithDAW";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName = "ZenithDAW";
        appProperties.setStorageParameters(options);

        // W6: Create stats overlay (DEBUG-only)
        statsOverlay = std::make_unique<StatsOverlay>();
        addChildComponent(statsOverlay.get());

        // W6.1: Load persisted HUD visibility (default ON if not set)
        auto* userSettings = appProperties.getUserSettings();
        bool showPerfHUD = userSettings->getBoolValue("debug.showPerfHUD", true);
        statsOverlay->setVisible(showPerfHUD);

        DBG("W6.1: HUD visibility loaded from settings: " + juce::String(showPerfHUD));
        DBG("W6.1: Settings file: " + userSettings->getFile().getFullPathName());

        #if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
            // Phase 1: Create debug HUD overlay (F12 toggle)
            phase1DebugOverlay = std::make_unique<Phase1DebugOverlay>(engine);
            addChildComponent(phase1DebugOverlay.get());

            // Start hidden by default
            phase1DebugOverlay->setVisible(false);

            DBG("Phase 1 Debug HUD initialized (press F12 to toggle)");
        #endif
    #endif

    // W6: Enable keyboard input for Ctrl+F10 toggle
    setWantsKeyboardFocus(true);
}

MainComponent::~MainComponent()
{
    // Remove custom LookAndFeel before destruction
    setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Background (should be covered by child components)
    g.fillAll(ZenithColours::backgroundDark);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Top bar at top
    topBar.setBounds(bounds.removeFromTop(topBarHeight));

    // Transport bar at bottom
    transportBar.setBounds(bounds.removeFromBottom(transportBarHeight));

    // Sidebar on left
    sidebar.setBounds(bounds.removeFromLeft(sidebarWidth));

    // Mixer on right (if visible)
    // auto mixer = bounds.removeFromRight(mixerWidth);

    // Remaining space is for ArrangerComponent
    arrangerComponent.setBounds(bounds);

    #if JUCE_DEBUG
        // W6: Position stats overlay in top-right corner
        if (statsOverlay != nullptr)
        {
            int overlayWidth = 220;
            int overlayHeight = 140;
            int margin = 10;
            statsOverlay->setBounds(getWidth() - overlayWidth - margin,
                                   topBarHeight + margin,
                                   overlayWidth, overlayHeight);
        }

        #if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
            // Phase 1: Position debug HUD in bottom-left corner
            if (phase1DebugOverlay != nullptr)
            {
                int overlayWidth = 300;
                int overlayHeight = 200;
                int margin = 10;
                phase1DebugOverlay->setBounds(sidebarWidth + margin,
                                             getHeight() - transportBarHeight - overlayHeight - margin,
                                             overlayWidth, overlayHeight);
            }
        #endif
    #endif
}

//==============================================================================
void MainComponent::setupCallbacks()
{
    // TopBar callbacks
    topBar.onProjectNameChanged = [this](const juce::String& name)
    {
        DBG("Project name changed: " + name);
        // TODO: Update project state
    };

    topBar.onSettingsClicked = [this]()
    {
        DBG("Settings clicked");
        toggleAudioSettings();
    };

    #ifdef _WIN32
        // Create audio settings panel (Windows only)
        if (audioSettingsPanel == nullptr)
        {
            // Access the AudioDeviceManager through Engine
            // Note: Engine exposes deviceManager as private, so we'll access it indirectly
            // For W3 stub, we'll use a temporary approach
            audioSettingsPanel = std::make_unique<AudioSettingsWindows>(engine.getDeviceManager());

            // Set up callbacks
            audioSettingsPanel->onSettingsChanged = [this](
                juce::String deviceType,
                juce::String outputDevice,
                juce::String inputDevice,
                double sampleRate,
                int bufferSize)
            {
                DBG("Audio settings changed:");
                DBG("  Device Type: " + deviceType);
                DBG("  Output: " + outputDevice);
                DBG("  Input: " + inputDevice);
                DBG("  Sample Rate: " + juce::String(sampleRate));
                DBG("  Buffer Size: " + juce::String(bufferSize));
                // W3 stub: no engine mutation yet
            };

            audioSettingsPanel->onOpenAsioPanel = [this]()
            {
                DBG("ASIO control panel requested");
                // TODO: Call device->showControlPanel() for ASIO devices
            };

            audioSettingsPanel->setVisible(false);
            addChildComponent(audioSettingsPanel.get());
        }
    #endif

    topBar.onAIToggleChanged = [this](bool enabled)
    {
        DBG("AI toggle: " + juce::String(enabled ? "ON" : "OFF"));
        // TODO: Enable/disable Wingman AI panel
    };

    // Sidebar callbacks
    sidebar.onTrackSelected = [this](int trackIndex)
    {
        DBG("Track selected: " + juce::String(trackIndex));
        // TODO: Highlight track in TrackView
    };

    sidebar.onFileSelected = [this](const juce::File& file)
    {
        DBG("File selected: " + file.getFullPathName());
        // TODO: Load audio file into project
    };

    // ArrangerComponent doesn't use callbacks - it interacts directly with editorState

    // TransportBar callbacks
    transportBar.onPlay = [this]()
    {
        DBG("Transport: Play");
        editorState.playFromPlayhead();
    };

    transportBar.onStop = [this]()
    {
        DBG("Transport: Stop");
        editorState.stop();
    };

    transportBar.onRecord = [this]()
    {
        DBG("Transport: Record");
        // TODO: Start recording
    };

    transportBar.onLoopToggle = [this](bool enabled)
    {
        DBG("Transport: Loop " + juce::String(enabled ? "ON" : "OFF"));
        // TODO: Enable/disable loop in engine
    };

    transportBar.onMetronomeToggle = [this](bool enabled)
    {
        DBG("Transport: Metronome " + juce::String(enabled ? "ON" : "OFF"));
        // TODO: Enable/disable metronome in engine
    };

    transportBar.onBPMChanged = [this](double bpm)
    {
        DBG("Transport: BPM changed to " + juce::String(bpm, 1));
        // TODO: Update engine BPM (v0.1: tempo not implemented yet)
    };
}

//==============================================================================
void MainComponent::injectTestSessionData()
{
    // v0.1: ArrangerComponent uses real ProjectModel data from editorState
    // No need to inject dummy test data like the old TrackView did
    // Users will load/create projects via File menu or drag-and-drop

    DBG("MainComponent: Using real ProjectModel (no dummy test data for v0.1)");
}

//==============================================================================
void MainComponent::toggleAudioSettings()
{
    #ifdef _WIN32
        if (audioSettingsPanel == nullptr)
            return;

        bool isCurrentlyVisible = audioSettingsPanel->isVisible();

        if (isCurrentlyVisible)
        {
            // Hide settings panel
            audioSettingsPanel->setVisible(false);
            DBG("Audio settings panel hidden");
        }
        else
        {
            // Show settings panel as overlay (centered)
            int panelWidth = 500;
            int panelHeight = 400;
            int x = (getWidth() - panelWidth) / 2;
            int y = (getHeight() - panelHeight) / 2;

            audioSettingsPanel->setBounds(x, y, panelWidth, panelHeight);
            audioSettingsPanel->setVisible(true);
            audioSettingsPanel->toFront(true); // Bring to front
            audioSettingsPanel->refreshDevices(); // Refresh device list when shown

            DBG("Audio settings panel shown");
        }
    #else
        DBG("Audio settings panel not available on this platform");
    #endif
}

//==============================================================================
// W6: Keyboard input and stats overlay
//==============================================================================

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Spacebar: toggle play/pause
    if (key == juce::KeyPress::spaceKey)
    {
        if (editorState.isPlaying())
        {
            editorState.pause();
            DBG("Spacebar: Paused");
        }
        else
        {
            editorState.playFromPlayhead();
            DBG("Spacebar: Playing from current position");
        }
        return true;
    }

    #if JUCE_DEBUG
        // Ctrl+F10 toggle stats overlay
        if (key == juce::KeyPress::F10Key && key.getModifiers().isCtrlDown())
        {
            toggleStatsOverlay();
            return true;
        }

        #if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
            // F12 toggle Phase 1 debug HUD
            if (key == juce::KeyPress::F12Key)
            {
                if (phase1DebugOverlay != nullptr)
                {
                    phase1DebugOverlay->setVisible(!phase1DebugOverlay->isVisible());
                    DBG("Phase 1 Debug HUD: " + juce::String(phase1DebugOverlay->isVisible() ? "ON" : "OFF"));
                }
                return true;
            }
        #endif
    #endif

    // Cmd/Ctrl+I: Import Audio
    if (key == juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0))
    {
        startImportAudio();
        return true;
    }

    // Cmd+E / Ctrl+E: Export WAV
    if (key == juce::KeyPress('e', juce::ModifierKeys::commandModifier, 0))
    {
        startExportWav();
        return true;
    }

    return false;  // Let other components handle key
}

#if JUCE_DEBUG
void MainComponent::toggleStatsOverlay()
{
    if (statsOverlay != nullptr)
    {
        bool isVisible = statsOverlay->isVisible();
        bool newVisibility = !isVisible;
        statsOverlay->setOverlayVisible(newVisibility);

        // W6.1: Persist HUD visibility state (message thread only, no allocations in paint)
        auto* userSettings = appProperties.getUserSettings();
        if (userSettings != nullptr)
        {
            userSettings->setValue("debug.showPerfHUD", newVisibility);
            userSettings->saveIfNeeded();
        }

        DBG("Stats overlay " + juce::String(isVisible ? "hidden" : "shown"));
        DBG("W6.1: HUD visibility persisted: " + juce::String(newVisibility));
    }
}

void MainComponent::updateStatsOverlay()
{
    if (statsOverlay != nullptr && statsOverlay->isVisible())
    {
        // Get TrackView paint stats
        int visibleTracks = 0;
        int visibleClips = 0;
        trackView.getLastPaintStats(visibleTracks, visibleClips);

        // Update overlay
        statsOverlay->updateTrackViewStats(visibleTracks, visibleClips);
    }
}
#endif

//==============================================================================
// Audio Import
//==============================================================================

void MainComponent::startImportAudio()
{
    // Create file chooser for audio files
    auto chooser = std::make_shared<juce::FileChooser>(
        "Import Audio File",
        juce::File{},
        "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg"
    );

    // Launch async file chooser
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                handleImportAudioFile(file);
            }
        }
    );
}

void MainComponent::handleImportAudioFile(const juce::File& file)
{
    // Determine target track (always track 0 for v0.1)
    const int trackIndex = 0;

    // Determine start position (current playhead)
    const auto startSample = editorState.getTransportSamples();

    // Insert clip
    if (editorState.insertClipFromFile(trackIndex, startSample, file))
    {
        DBG("Imported audio file: " + file.getFileName());

        // Optionally select the new clip in arranger
        // (For v0.1, we'll just let it appear without selecting)

        // Repaint arranger to show new clip
        arrangerComponent.repaint();
    }
    else
    {
        DBG("Failed to import audio file: " + file.getFileName());

        // Show error to user
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Import Failed",
            "Could not import audio file: " + file.getFileName()
        );
    }
}

//==============================================================================
// Export Management
//==============================================================================

void MainComponent::startExportWav()
{
    // Check if already exporting
    if (isExporting_)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Export already in progress",
            "Please wait for the current export to finish.");
        return;
    }

    // Check if project has any clips
    const auto& proj = editorState.getProject();
    bool hasAudio = false;
    for (const auto& t : proj.tracks)
    {
        if (!t.clips.empty())
        {
            hasAudio = true;
            break;
        }
    }

    if (!hasAudio)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Nothing to export",
            "This project has no audio clips to render.");
        return;
    }

    // Determine default directory
    juce::File defaultDir;
    if (editorState.hasProjectFile())
        defaultDir = editorState.getCurrentProjectFile().getParentDirectory();
    else
        defaultDir = juce::File::getSpecialLocation(juce::File::userMusicDirectory);

    // Launch file chooser
    auto chooser = std::make_shared<juce::FileChooser>(
        "Export mixdown as WAV",
        defaultDir,
        "*.wav");

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File())
                return; // user cancelled

            // Ensure .wav extension
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".wav");

            // Start background job
            isExporting_ = true;
            setEnabled(false); // v0.1 brute force: disable UI during export

            exportJob_ = std::make_unique<ExportWavJob>(editorState, file);
            exportJob_->startThread();

            DBG("Export started: " + file.getFullPathName());

            // Start polling for completion
            pollExportCompletion(file);
        });
}

void MainComponent::pollExportCompletion(juce::File file)
{
    if (!exportJob_ || !exportJob_->isThreadRunning())
    {
        // Export finished
        bool ok = exportJob_ ? exportJob_->wasSuccessful() : false;
        auto msg = ok ? juce::String() : (exportJob_ ? exportJob_->getErrorMessage() : "Unknown error");

        exportJob_.reset();
        isExporting_ = false;
        setEnabled(true);

        onExportFinished(ok, msg, file);
    }
    else if (isExporting_)
    {
        // Still running, poll again in 100ms
        juce::Timer::callAfterDelay(100, [this, file]()
        {
            pollExportCompletion(file);
        });
    }
}

void MainComponent::onExportFinished(bool ok, const juce::String& message, juce::File file)
{
    if (ok)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Export complete",
            "Exported mixdown to:\n" + file.getFullPathName());
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Export failed",
            message.isNotEmpty() ? message : "Unknown error while exporting.");
    }
}
