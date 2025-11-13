/**
 * @file MainComponent.cpp
 * @brief Implementation of main UI component
 */

#include "MainComponent.h"

#if defined(ZENITH_USE_CRASHPAD) && defined(JUCE_WINDOWS)
    #include "../win/CrashpadInit.h"
#endif

//==============================================================================
MainComponent::MainComponent(Engine& eng)
    : engine(eng),
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

    // Setup callbacks between components
    setupCallbacks();

    // W5: Inject stress test data (100 tracks × 50 clips)
    injectTestSessionData();

    // W6.1 & W8: Initialize ApplicationProperties (used for HUD persistence & crash reporting settings)
    juce::PropertiesFile::Options options;
    options.applicationName = "ZenithDAW";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "ZenithDAW";
    appProperties.setStorageParameters(options);

    #if defined(ZENITH_USE_CRASHPAD) && defined(JUCE_WINDOWS)
        // W8: Initialize Crashpad crash reporting (opt-in via settings)
        auto* userSettings = appProperties.getUserSettings();
        bool crashReportsEnabled = userSettings->getBoolValue("diagnostics.crashReportsEnabled", false);

        if (crashReportsEnabled)
        {
            auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("ZenithDAW");
            auto dbDir = appData.getChildFile("crashpad_db");

            #ifndef CRASHPAD_HANDLER_PATH
                DBG("Crashpad: CRASHPAD_HANDLER_PATH not defined at compile time");
            #else
                juce::File handler(CRASHPAD_HANDLER_PATH);

                if (handler.existsAsFile())
                {
                    juce::StringPairArray annotations;
                    annotations.set("product", "Zenith DAW");
                    annotations.set("version", "0.1.0");
                    annotations.set("build", juce::String(JUCE_DEBUG ? "Debug" : "Release"));
                    annotations.set("platform", "Windows");

                    bool success = zenith::diag::initCrashpad(handler, dbDir, annotations);

                    if (success)
                    {
                        DBG("W8: Crashpad initialized successfully");
                        DBG("  Handler: " + handler.getFullPathName());
                        DBG("  Database: " + dbDir.getFullPathName());
                    }
                    else
                    {
                        DBG("W8: Crashpad initialization failed (see logs above)");
                    }
                }
                else
                {
                    DBG("Crashpad: handler not found at " + handler.getFullPathName());
                }
            #endif
        }
        else
        {
            DBG("W8: Crash reporting disabled (user opt-in required)");
            DBG("  Enable in: Help → Diagnostics → Enable Crash Reports");
        }
    #endif

    #if JUCE_DEBUG
        // W6: Create stats overlay (DEBUG-only)
        statsOverlay = std::make_unique<StatsOverlay>(engine);
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
