/*
  ==============================================================================

    MainComponent.cpp
    Implementation of main application component

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Set initial size
    setSize (1600, 1000);

    // Initialize core systems
    audioEngine = std::make_unique<AudioEngine>();
    projectState = std::make_unique<ProjectState>();
    projectManager = std::make_unique<ProjectManager>(*projectState, *audioEngine);

    // Setup audio device
    setupAudioDevice();

    // Add and make visible all child components
    addAndMakeVisible (transportBar);
    addAndMakeVisible (leftPanel);
    addAndMakeVisible (centerPanel);
    addAndMakeVisible (rightPanel);

    // Connect components to engine
    transportBar.setAudioEngine (audioEngine.get());
    centerPanel.setAudioEngine (audioEngine.get());
    rightPanel.setAudioEngine (audioEngine.get());

    // Setup keyboard shortcuts
    setupKeyboardShortcuts();
    commandManager.registerAllCommandsForTarget (this);
    addKeyListener (commandManager.getKeyMappings());

    // Listen to state changes
    projectState->addChangeListener (this);

    // Start UI update timer (20Hz)
    startTimer (50);

    // Start audio
    audioSourcePlayer.setSource (audioEngine.get());
}

MainComponent::~MainComponent()
{
    stopTimer();

    // Stop audio first
    audioSourcePlayer.setSource (nullptr);
    deviceManager.removeAudioCallback (&audioSourcePlayer);

    projectState->removeChangeListener (this);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Dark background color (matching web app)
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Transport bar at top (80 pixels)
    transportBar.setBounds (bounds.removeFromTop (80));

    // Main content area split into 3 panels
    auto leftWidth = juce::roundToInt (bounds.getWidth() * 0.20f);   // 20% for browser
    auto rightWidth = juce::roundToInt (bounds.getWidth() * 0.25f);  // 25% for mixer
    // Remaining space for arrangement (55%)

    leftPanel.setBounds (bounds.removeFromLeft (leftWidth));
    rightPanel.setBounds (bounds.removeFromRight (rightWidth));
    centerPanel.setBounds (bounds);  // Takes remaining space
}

//==============================================================================
void MainComponent::setupAudioDevice()
{
    // Initialize with default audio device (2 channels out, 0 channels in by default)
    juce::String error = deviceManager.initialiseWithDefaultDevices (0, 2);

    if (error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                "Audio Device Error",
                                                "Failed to initialize audio device:\\n" + error);
    }

    // Add audio callback
    deviceManager.addAudioCallback (&audioSourcePlayer);

    // Get current audio device setup and prepare engine
    auto setup = deviceManager.getAudioDeviceSetup();
    audioEngine->prepareToPlay (setup.bufferSize, setup.sampleRate);

    DBG ("Audio device initialized:");
    DBG ("  Sample Rate: " + juce::String (setup.sampleRate) + " Hz");
    DBG ("  Buffer Size: " + juce::String (setup.bufferSize) + " samples");
}

void MainComponent::setupKeyboardShortcuts()
{
    // Transport controls
    commandManager.getKeyMappings()->addKeyPress (CommandIDs::play, juce::KeyPress::spaceKey, 0);
    commandManager.getKeyMappings()->addKeyPress (CommandIDs::stop, juce::KeyPress::returnKey, 0);

    // File operations
    commandManager.getKeyMappings()->addKeyPress (CommandIDs::newProject,
                                                  juce::KeyPress ('n', juce::ModifierKeys::commandModifier, 0));
    commandManager.getKeyMappings()->addKeyPress (CommandIDs::openProject,
                                                  juce::KeyPress ('o', juce::ModifierKeys::commandModifier, 0));
    commandManager.getKeyMappings()->addKeyPress (CommandIDs::saveProject,
                                                  juce::KeyPress ('s', juce::ModifierKeys::commandModifier, 0));

    // Edit operations
    commandManager.getKeyMappings()->addKeyPress (CommandIDs::undo,
                                                  juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0));
    commandManager.getKeyMappings()->addKeyPress (CommandIDs::redo,
                                                  juce::KeyPress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0));
}

//==============================================================================
// ApplicationCommandTarget implementation
juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
    return nullptr;
}

void MainComponent::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    commands.add (CommandIDs::newProject);
    commands.add (CommandIDs::openProject);
    commands.add (CommandIDs::saveProject);
    commands.add (CommandIDs::saveProjectAs);
    commands.add (CommandIDs::closeProject);

    commands.add (CommandIDs::undo);
    commands.add (CommandIDs::redo);

    commands.add (CommandIDs::play);
    commands.add (CommandIDs::stop);
    commands.add (CommandIDs::record);
    commands.add (CommandIDs::loop);

    commands.add (CommandIDs::createAudioTrack);
    commands.add (CommandIDs::createMIDITrack);
    commands.add (CommandIDs::createInstrumentTrack);
}

void MainComponent::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case CommandIDs::newProject:
            result.setInfo ("New Project", "Create a new project", "File", 0);
            result.addDefaultKeypress ('n', juce::ModifierKeys::commandModifier);
            break;

        case CommandIDs::openProject:
            result.setInfo ("Open Project...", "Open an existing project", "File", 0);
            result.addDefaultKeypress ('o', juce::ModifierKeys::commandModifier);
            break;

        case CommandIDs::saveProject:
            result.setInfo ("Save Project", "Save the current project", "File", 0);
            result.addDefaultKeypress ('s', juce::ModifierKeys::commandModifier);
            break;

        case CommandIDs::play:
            result.setInfo (audioEngine->isPlaying() ? "Pause" : "Play",
                          "Start or pause playback", "Transport", 0);
            result.addDefaultKeypress (juce::KeyPress::spaceKey, 0);
            break;

        case CommandIDs::stop:
            result.setInfo ("Stop", "Stop playback", "Transport", 0);
            result.addDefaultKeypress (juce::KeyPress::returnKey, 0);
            break;

        case CommandIDs::undo:
            result.setInfo ("Undo", "Undo last action", "Edit", 0);
            result.addDefaultKeypress ('z', juce::ModifierKeys::commandModifier);
            result.setActive (projectState->getUndoManager().canUndo());
            break;

        case CommandIDs::redo:
            result.setInfo ("Redo", "Redo last undone action", "Edit", 0);
            result.addDefaultKeypress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            result.setActive (projectState->getUndoManager().canRedo());
            break;

        default:
            break;
    }
}

bool MainComponent::perform (const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case CommandIDs::newProject:
            createNewProject();
            return true;

        case CommandIDs::openProject:
            openProject();
            return true;

        case CommandIDs::saveProject:
            saveCurrentProject();
            return true;

        case CommandIDs::play:
            if (audioEngine->isPlaying())
                audioEngine->pause();
            else
                audioEngine->play();
            return true;

        case CommandIDs::stop:
            audioEngine->stop();
            return true;

        case CommandIDs::undo:
            projectState->getUndoManager().undo();
            return true;

        case CommandIDs::redo:
            projectState->getUndoManager().redo();
            return true;

        case CommandIDs::createAudioTrack:
            audioEngine->addTrack ("Audio " + juce::String (audioEngine->getNumTracks() + 1),
                                  Track::Type::Audio);
            return true;

        case CommandIDs::createMIDITrack:
            audioEngine->addTrack ("MIDI " + juce::String (audioEngine->getNumTracks() + 1),
                                  Track::Type::MIDI);
            return true;

        default:
            return false;
    }
}

//==============================================================================
void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == projectState.get())
    {
        // Project state changed - update UI
        repaint();
    }
}

void MainComponent::timerCallback()
{
    // Update UI components periodically
    transportBar.updateDisplay();
    rightPanel.updateMeters();
}

//==============================================================================
// File operations
bool MainComponent::checkForUnsavedChanges()
{
    if (!projectManager->hasUnsavedChanges())
        return true;

    int result = juce::AlertWindow::showYesNoCancelBox (juce::AlertWindow::QuestionIcon,
                                                       "Unsaved Changes",
                                                       "Do you want to save your changes before continuing?",
                                                       "Save", "Don't Save", "Cancel");

    if (result == 0)  // Cancel
        return false;

    if (result == 1)  // Save
        return projectManager->saveProject();

    return true;  // Don't save
}

void MainComponent::createNewProject()
{
    if (!checkForUnsavedChanges())
        return;

    projectManager->newProject();
    centerPanel.clearArrangement();
    rightPanel.clearMixer();
}

void MainComponent::openProject()
{
    if (!checkForUnsavedChanges())
        return;

    juce::FileChooser chooser ("Open Project",
                              juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                              "*.vxl");

    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        if (projectManager->loadProject (file))
        {
            centerPanel.refreshArrangement();
            rightPanel.refreshMixer();
        }
    }
}

void MainComponent::saveCurrentProject()
{
    if (projectManager->getCurrentProjectFile() == juce::File())
        saveProjectAs();
    else
        projectManager->saveProject();
}

void MainComponent::saveProjectAs()
{
    juce::FileChooser chooser ("Save Project As",
                              juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                              "*.vxl");

    if (chooser.browseForFileToSave (true))
    {
        auto file = chooser.getResult();
        projectManager->saveProjectAs (file);
    }
}
