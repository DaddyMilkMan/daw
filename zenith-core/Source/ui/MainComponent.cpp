/**
 * @file MainComponent.cpp
 * @brief Implementation of main UI component
 */

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(Engine& eng)
    : engine(eng),
      editorState(eng),
      transportBar(eng)
{
    // Apply custom LookAndFeel to this component and all children
    setLookAndFeel(&zenithLookAndFeel);

    // Set size
    setSize(1400, 800);

    // Add all UI components
    addAndMakeVisible(topBar);
    addAndMakeVisible(sidebar);
    addAndMakeVisible(trackView);
    addAndMakeVisible(transportBar);

    // v0.1: Create demo project
    zenith::ProjectModel demoProject;
    demoProject.name = "v0.1 Demo Project";
    demoProject.sampleRate = 48000.0;
    demoProject.blockSize = 512;
    demoProject.nextClipId = 1;

    // Create a demo track
    zenith::TrackModel track;
    track.id = 0;
    track.name = "Audio 1";
    track.gain = 1.0f;
    track.pan = 0.0f;
    track.muted = false;

    // Create a demo clip (stub - no actual audio file for now)
    zenith::ClipModel clip;
    clip.id = 1;
    clip.filePath = "";  // v0.1: empty for now (will need real file for playback)
    clip.startSample = 0;
    clip.lengthSamples = 48000; // 1 second @ 48kHz
    clip.srcOffset = 0;
    clip.gain = 1.0f;
    clip.fadeInSamples = 0;
    clip.fadeOutSamples = 0;
    clip.muted = false;
    clip.loopEnabled = false;

    track.clips.push_back(clip);
    demoProject.tracks.push_back(track);

    // Load demo project into editor state
    editorState.setProject(demoProject);

    // v0.1: Create minimal arranger UI
    arranger = std::make_unique<zenith::ArrangerComponent>(editorState);
    addAndMakeVisible(*arranger);

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

    // Remaining space is for TrackView
    trackView.setBounds(bounds);

    // v0.1: Arranger overlays on top of TrackView (for now)
    if (arranger != nullptr)
    {
        arranger->setBounds(bounds);
    }

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

    // TrackView callbacks
    trackView.onClipSelected = [this](int clipId)
    {
        DBG("Clip selected: " + juce::String(clipId));
        // TODO: Show clip editor
    };

    trackView.onTrackSelected = [this](int trackIndex)
    {
        DBG("Track selected in view: " + juce::String(trackIndex));
        // TODO: Update sidebar selection
    };

    trackView.onPlayheadClicked = [this](double position)
    {
        DBG("Playhead clicked at: " + juce::String(position));
        transportBar.setPosition(position);
        // TODO: Seek engine to position
    };

    #if JUCE_DEBUG
        // W6: TrackView paint complete callback (for performance monitoring)
        trackView.onPaintComplete = [this](double paintTimeMs)
        {
            if (statsOverlay != nullptr && statsOverlay->isVisible())
            {
                statsOverlay->recordPaint("TrackView", paintTimeMs);
                updateStatsOverlay();
            }
        };
    #endif

    // TransportBar callbacks
    transportBar.onPlay = [this]()
    {
        DBG("Transport: Play");
        engine.play();
    };

    transportBar.onStop = [this]()
    {
        DBG("Transport: Stop");
        engine.stop();
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
        trackView.setBPM(bpm);
        // TODO: Update engine BPM
    };
}

//==============================================================================
void MainComponent::injectTestSessionData()
{
    // W5: Create 100 tracks
    std::vector<Track> testTracks;
    testTracks.reserve(100);

    for (int i = 0; i < 100; ++i)
    {
        Track track;
        track.name = (i % 3 == 0) ? "Audio " : (i % 3 == 1) ? "MIDI " : "Aux ";
        track.name += juce::String(i + 1);
        track.lanes = 1;
        testTracks.push_back(track);
    }

    // W5: Create 50 clips per track (5000 total clips)
    std::vector<Clip> testClips;
    testClips.reserve(5000);

    juce::Random rng(12345);  // Seeded for reproducibility

    juce::Array<juce::Colour> clipColours = {
        ZenithColours::track1,
        ZenithColours::track2,
        ZenithColours::track3,
        ZenithColours::track4,
        juce::Colours::green,
        juce::Colours::orange,
        juce::Colours::purple,
        juce::Colours::cyan
    };

    for (int trackIdx = 0; trackIdx < 100; ++trackIdx)
    {
        for (int clipIdx = 0; clipIdx < 50; ++clipIdx)
        {
            Clip clip;
            clip.trackIndex = trackIdx;

            // Random start position (0-300 seconds)
            clip.startSamples = rng.nextInt64(juce::Range<juce::int64>(0, 300 * 44100));

            // Random length (0.5-5 seconds)
            clip.lengthSamples = rng.nextInt64(juce::Range<juce::int64>(22050, 220500));

            // Random color
            clip.colour = clipColours[rng.nextInt(clipColours.size())];

            // Random name
            clip.name = "Clip " + juce::String(clipIdx + 1);

            testClips.push_back(clip);
        }
    }

    // Inject into TrackView
    trackView.setSessionData(std::move(testTracks), std::move(testClips));

    DBG("W5: Injected test data - 100 tracks × 50 clips (5000 total clips)");
}

//==============================================================================
// v0.1: File operations
//==============================================================================

void MainComponent::handleNewProject()
{
    // v0.1: Simple implementation - just create new project without prompting
    // v0.2+: Check for unsaved changes, prompt user

    editorState.newProject(48000.0, "Untitled Project");

    // Repaint arranger to show empty project
    if (arranger != nullptr)
        arranger->repaint();

    DBG("New project created");
}

void MainComponent::handleOpenProject()
{
    // Create file chooser for .zenithproj files
    auto fileChooser = std::make_shared<juce::FileChooser>(
        "Open Project",
        juce::File{},
        "*.zenithproj"
    );

    // Show async file chooser (non-blocking)
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, fileChooser](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File{})
        {
            DBG("Open project cancelled");
            return; // User cancelled
        }

        // Load project
        juce::String error;
        if (!editorState.openProjectFromFile(file, &error))
        {
            DBG("Failed to open project: " << error);
            // v0.1: Just log error, v0.2+: Show alert dialog
            return;
        }

        // Repaint arranger to show loaded project
        if (arranger != nullptr)
            arranger->repaint();

        DBG("Project opened: " << file.getFullPathName());
    });
}

void MainComponent::handleSaveProject()
{
    // Try to save to current file
    juce::String error;
    if (editorState.saveIfHasFile(&error))
    {
        DBG("Project saved");
        return;
    }

    // No file set - fall back to Save As
    DBG("No file set, using Save As: " << error);
    handleSaveProjectAs();
}

void MainComponent::handleSaveProjectAs()
{
    // Create file chooser for .zenithproj files
    auto fileChooser = std::make_shared<juce::FileChooser>(
        "Save Project As",
        juce::File{},
        "*.zenithproj"
    );

    // Show async file chooser (non-blocking)
    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, fileChooser](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File{})
        {
            DBG("Save project cancelled");
            return; // User cancelled
        }

        // Ensure .zenithproj extension
        if (!file.hasFileExtension(".zenithproj"))
            file = file.withFileExtension(".zenithproj");

        // Save project
        juce::String error;
        if (!editorState.saveProjectToFile(file, &error))
        {
            DBG("Failed to save project: " << error);
            // v0.1: Just log error, v0.2+: Show alert dialog
            return;
        }

        DBG("Project saved: " << file.getFullPathName());
    });
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
    // v0.1: Spacebar for play/stop
    if (key == juce::KeyPress::spaceKey)
    {
        if (editorState.isPlaying())
            editorState.stop();
        else
            editorState.playFromCursor();

        // Repaint arranger to update playhead
        if (arranger != nullptr)
            arranger->repaint();

        return true;
    }

    // v0.1: File operations (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S)
    auto mods = key.getModifiers();

    // Ctrl+N - New project
    if (key == juce::KeyPress('n') && mods.isCommandDown() && !mods.isShiftDown())
    {
        handleNewProject();
        return true;
    }

    // Ctrl+O - Open project
    if (key == juce::KeyPress('o') && mods.isCommandDown() && !mods.isShiftDown())
    {
        handleOpenProject();
        return true;
    }

    // Ctrl+S - Save project
    if (key == juce::KeyPress('s') && mods.isCommandDown() && !mods.isShiftDown())
    {
        handleSaveProject();
        return true;
    }

    // Ctrl+Shift+S - Save As
    if (key == juce::KeyPress('s') && mods.isCommandDown() && mods.isShiftDown())
    {
        handleSaveProjectAs();
        return true;
    }

    #if JUCE_DEBUG
        // Ctrl+F10 toggle stats overlay
        if (key == juce::KeyPress::F10Key && key.getModifiers().isCtrlDown())
        {
            toggleStatsOverlay();
            return true;
        }
    #endif

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
