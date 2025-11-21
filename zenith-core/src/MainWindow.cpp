/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../Source/commands/CommandAPI.h"
#include "ArrangerComponent.h"
#include "ArrangementComponent.h"
#include "WingmanPanel.h"
#include "InstrumentBrowserPanel.h"
#include "AIBridgeClient.h"
#include "../include/PianoRollEditor.h"
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"
#include "../Source/ui/MasterOutputComponent.h"
#include "../Source/ui/ProjectSettingsComponent.h"
#include "../Source/ui/TransportControlComponent.h"
#include "../Source/ui/ZenithLookAndFeel.h"
#include "../Source/ui/ZenithTransportBar.h"

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng, zenith::CommandAPI& api, zenith::AIBridgeClient& aiClient, ProjectState& state)
    : engine(eng), projectState(state), mixerComponent(state)
{
    // Set size
    setSize(1400, 800);

    // Add mixer component
    addAndMakeVisible(mixerComponent);

    // Register as key listener for undo/redo shortcuts
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Phase 1: Import Audio button
    importButton.setButtonText("Import Audio...");
    importButton.onClick = [this]() {
        handleImportAudio();
    };
    addAndMakeVisible(importButton);

    // Phase 9: Create ArrangerComponent with interactive clip editing
    arrangerComponent = std::make_unique<ArrangerComponent>(projectState);
    addAndMakeVisible(arrangerComponent.get());

    // Phase 7: Create Wingman AI console panel
    wingmanPanel = std::make_unique<WingmanPanel>(api, aiClient);
    addAndMakeVisible(wingmanPanel.get());

    // Create Instrument Browser Panel
    instrumentBrowserPanel = std::make_unique<zenith::InstrumentBrowserPanel>(engine, projectState);
    addAndMakeVisible(instrumentBrowserPanel.get());

    // Phase 11: Create Master Output Component
    masterOutputComponent = std::make_unique<zenith::MasterOutputComponent>(engine);
    addAndMakeVisible(masterOutputComponent.get());

    // Create Project Settings Component (hidden by default, shown in dialog)
    projectSettingsComponent = std::make_unique<zenith::ProjectSettingsComponent>(projectState);
    // DON'T add to main view - will be shown as dialog when needed

    // NEW: Create unified transport bar (replaces old transport + bottom buttons)
    zenithTransportBar = std::make_unique<zenith::ZenithTransportBar>(engine);
    addAndMakeVisible(zenithTransportBar.get());

    // NEW: Create unified status bar
    zenithStatusBar = std::make_unique<zenith::ZenithStatusBar>(engine);
    addAndMakeVisible(zenithStatusBar.get());

    // SKIA DEMO: Create test button to showcase Skia rendering
    #ifdef ZENITH_USE_SKIA
        skiaTestButton = std::make_unique<zenith::SkiaButtonComponent>(
            "Skia GPU Demo",
            zenith::SkiaButtonComponent::Style::Primary);
        skiaTestButton->onClick = [this]() {
            DBG("Skia button clicked!");
            if (zenithStatusBar)
            {
                zenithStatusBar->showMessage("🎨 Skia GPU rendering works! D3D12 backend active.", false, 3000);
            }
        };
        addAndMakeVisible(*skiaTestButton);
        DBG("Skia button component created and added to UI");
    #else
        DBG("Skia not enabled - build with -DZENITH_ENABLE_SKIA=ON to enable GPU rendering");
    #endif

    // Start timer for updates (60 Hz)
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
    if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown() && !key.getModifiers().isShiftDown())
    {
        if (projectState.canUndo())
        {
            projectState.undo();
            DBG("Keyboard shortcut: Undo");
            return true;
        }
    }

    // Ctrl+Shift+Z or Cmd+Shift+Z for redo
    if (key.getTextCharacter() == 'Z' && key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown())
    {
        if (projectState.canRedo())
        {
            projectState.redo();
            DBG("Keyboard shortcut: Redo");
            return true;
        }
    }

    // Ctrl+Y or Cmd+Y for redo (alternative)
    if (key.getTextCharacter() == 'y' && key.getModifiers().isCommandDown())
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
    // Background using Zenith design system
    g.fillAll(juce::Colour(zenith::ZenithLookAndFeel::Colors::backgroundDark));
}

void MainComponent::resized()
{
    using namespace zenith;
    auto bounds = getLocalBounds();

    // ==========================================================================
    // Top: Unified Transport Bar (fixed height)
    // ==========================================================================
    if (zenithTransportBar != nullptr)
    {
        auto transportBounds = bounds.removeFromTop(ZenithLookAndFeel::Metrics::transportBarHeight);
        zenithTransportBar->setBounds(transportBounds);
    }

    // ==========================================================================
    // SKIA DEMO: Test button (top right corner, temporary placement)
    // ==========================================================================
    #ifdef ZENITH_USE_SKIA
        if (skiaTestButton != nullptr)
        {
            auto demoArea = bounds.removeFromTop(50);
            skiaTestButton->setBounds(demoArea.removeFromRight(220).reduced(10));
        }
    #endif

    // ==========================================================================
    // Bottom: Status Bar (fixed height)
    // ==========================================================================
    if (zenithStatusBar != nullptr)
    {
        auto statusBounds = bounds.removeFromBottom(ZenithLookAndFeel::Metrics::statusBarHeight);
        zenithStatusBar->setBounds(statusBounds);
    }

    // ==========================================================================
    // Bottom: Mixer Panel (fixed height)
    // ==========================================================================
    auto mixerArea = bounds.removeFromBottom(ZenithLookAndFeel::Metrics::mixerHeight);
    mixerComponent.setBounds(mixerArea);

    // ==========================================================================
    // Bottom right corner: Import Audio button (temporary placement)
    // ==========================================================================
    // auto importButtonArea = bounds.removeFromBottom(ZenithLookAndFeel::Metrics::buttonHeightL + ZenithLookAndFeel::Metrics::spacingM);
    // importButton.setBounds(importButtonArea.removeFromLeft(160).reduced(ZenithLookAndFeel::Metrics::spacingM));
    // Hiding import button for now as it overlaps with layout - use Menu instead

    // ==========================================================================
    // Left: Browser Panel (fixed width, resizable in future)
    // ==========================================================================
    if (instrumentBrowserPanel != nullptr)
    {
        auto browserBounds = bounds.removeFromLeft(ZenithLookAndFeel::Metrics::browserPanelWidth);
        instrumentBrowserPanel->setBounds(browserBounds);
    }

    // ==========================================================================
    // Right: Wingman AI Panel + Master Output (fixed widths)
    // ==========================================================================
    
    // Master output strip (rightmost)
    if (masterOutputComponent != nullptr)
    {
        auto masterBounds = bounds.removeFromRight(ZenithLookAndFeel::Metrics::masterStripWidth);
        masterOutputComponent->setBounds(masterBounds);
    }

    // Wingman panel (right of center, before master)
    if (wingmanPanel != nullptr)
    {
        auto wingmanBounds = bounds.removeFromRight(ZenithLookAndFeel::Metrics::inspectorPanelWidth);
        wingmanPanel->setBounds(wingmanBounds);
    }

    // ==========================================================================
    // Center: Arranger / Main Content (takes remaining space)
    // ==========================================================================
    if (arrangerComponent != nullptr)
    {
        arrangerComponent->setBounds(bounds);
    }
}

void MainComponent::timerCallback()
{
    // Transport bar handles its own updates now
    // Nothing to do here for now
}


//==============================================================================
// Integration: Piano roll opener
//==============================================================================

void MainComponent::openPianoRoll(const juce::String& trackId, const juce::String& clipId)
{
    DBG("MainComponent: Opening piano roll for " + trackId + "/" + clipId);

    // Create new piano roll editor window
    // Note: Window deletes itself when closed (see PianoRollEditor::closeButtonPressed)
    std::make_unique<PianoRollEditor>(projectState, trackId, clipId);
}

//==============================================================================
// Phase 1: Audio Import
//==============================================================================

void MainComponent::handleImportAudio()
{
    // Create file chooser for audio files
    auto chooser = std::make_shared<juce::FileChooser>(
        "Import Audio File",
        juce::File{},
        "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg");

    // Open file chooser (async)
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (!file.existsAsFile())
            return;

        DBG("Importing audio file: " + file.getFullPathName());

        // Ensure we have at least one track
        if (engine.getNumTracks() == 0)
        {
            DBG("Creating first track for audio import");
            engine.addTestTracks(1);
        }

        // Get the first track
        const auto& tracks = engine.tracks();
        if (tracks.empty())
        {
            DBG("ERROR: Failed to get track after creation");
            return;
        }

        auto* track = tracks[0].get();
        if (track == nullptr)
        {
            DBG("ERROR: Track is null");
            return;
        }

        // Create a new clip
        auto clip = std::make_unique<zenith::Track::Clip>();
        clip->setType(zenith::Track::Clip::Type::Audio);
        clip->setName(file.getFileNameWithoutExtension());

        // Load audio file through pool (message thread - safe to do I/O)
        auto& pool = engine.getAudioFilePool();
        clip->setAudioFileFromPool(file, pool);

        // Set clip timing: start at position 0, play immediately
        clip->setStartPosition(0);
        clip->setPlaying(true);

        DBG("Clip created: " + clip->getName() +
            ", length: " + juce::String(clip->getLength()) + " samples");

        // Add clip to track
        track->addClip(std::move(clip));

        DBG("Audio import complete! Track now has " +
            juce::String(track->getNumClips()) + " clip(s)");
    });
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

    // Phase 13: Create automation synchronizer
    automationSync = std::make_unique<TrackAutomationSynchronizer>(*projectState, *engine);

    // Phase 5: Create Wingman command API
    commandAPI = std::make_unique<zenith::CommandAPI>(*engine, *projectState);

    // Phase 7: Create AI bridge client
    aiBridgeClient = std::make_unique<zenith::AIBridgeClient>();

    // Phase 13: Connect project state to engine for automation
    engine->setProjectState(projectState.get());

    // Integration: Create clip synchronizer
    clipSynchronizer = std::make_unique<ClipSynchronizer>(*projectState, *engine);

    // Add some demo tracks for testing (Phase 9 + existing features)
    projectState->addTrack("Audio 1", "audio");
    projectState->addTrack("MIDI 1", "midi");
    projectState->addTrack("Audio 2", "audio");

    // NEW: Create and apply Zenith design system
    zenithLookAndFeel = std::make_unique<zenith::ZenithLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(zenithLookAndFeel.get());

    // Create main content (Phase 14: Automation + Phase 10: Mixer + Phase 9: Arranger + Wingman AI)
    mainComponent = std::make_unique<MainComponent>(*engine, *commandAPI, *aiBridgeClient, *projectState);

    // Create menu bar
    menuBar = std::make_unique<ZenithMenuBar>(*this);
    setMenuBar(menuBar.get());

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

    // Start automation synchronizer (Phase 13)
    automationSync->start(60); // 60 Hz update rate

    DBG("MainWindow created and initialized");
}

MainWindow::~MainWindow()
{
    // Clear menu bar first
    setMenuBar(nullptr);
    menuBar.reset();

    // Shutdown audio engine before destroying components
    if (engine)
        engine->shutdown();

    // Clear content
    clearContentComponent();

    // Restore default look and feel before destroying our custom one
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    zenithLookAndFeel.reset();

    DBG("MainWindow destroyed");
}

void MainWindow::closeButtonPressed()
{
    // TODO(zenith-core#1): Check for unsaved changes
    // TODO(zenith-core#1): Show save dialog if needed

    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::showAboutDialog()
{
    juce::String aboutMessage;
    aboutMessage << "Zenith DAW\n\n";
    aboutMessage << "A professional digital audio workstation\n\n";
    aboutMessage << "Version: 0.1.0\n";
    aboutMessage << "Built with JUCE 8.0.9\n\n";
    aboutMessage << "For documentation and installation instructions, see:\n";
    aboutMessage << "• docs/README.md\n";
    aboutMessage << "• docs/INSTALL_WINDOWS.md";

    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "About Zenith DAW",
        aboutMessage,
        "OK"
    );
}

//==============================================================================
// ZenithMenuBar Implementation
//==============================================================================

MainWindow::ZenithMenuBar::ZenithMenuBar(MainWindow& mainWindow)
    : owner(mainWindow)
{
}

juce::StringArray MainWindow::ZenithMenuBar::getMenuBarNames()
{
    return { "File", "Help" };
}

juce::PopupMenu MainWindow::ZenithMenuBar::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)  // File menu
    {
        #if ! (JUCE_IOS || JUCE_ANDROID)
            menu.addItem(quit, "Quit", true, false);
        #endif
    }
    else if (topLevelMenuIndex == 1)  // Help menu
    {
        menu.addItem(aboutZenith, "About Zenith DAW...", true, false);
    }

    return menu;
}

void MainWindow::ZenithMenuBar::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    switch (menuItemID)
    {
        case aboutZenith:
            owner.showAboutDialog();
            break;

        case quit:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;

        default:
            break;
    \n    default: break;\n\n    default: break;\n}
}


