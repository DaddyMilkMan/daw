/**
 * @file MainComponent.cpp
 * @brief Implementation of main UI component
 */

#include "MainComponent.h"

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
