/*
  ==============================================================================

    MainComponent.h
    Main application component - holds all panels and audio engine

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Audio/AudioEngine.h"
#include "GUI/Transport/TransportComponent.h"
#include "GUI/Mixer/MixerComponent.h"
#include "GUI/Arrangement/ArrangementComponent.h"
#include "GUI/Browser/BrowserComponent.h"
#include "State/ProjectState.h"
#include "Utilities/ProjectManager.h"

//==============================================================================
/**
 * Main application component
 *
 * Layout:
 * +--------------------------------------------------------+
 * |  Transport Bar                                         |
 * +--------------------------------------------------------+
 * |          |                           |                 |
 * | Browser  |   Arrangement View        |  Mixer Panel    |
 * |  Panel   |   (Timeline/Session)      |                 |
 * |          |                           |                 |
 * +--------------------------------------------------------+
 */
class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      public juce::ChangeListener,
                      public juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    // ApplicationCommandTarget interface
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;

    //==============================================================================
    // ChangeListener interface
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    //==============================================================================
    // Timer callback for UI updates
    void timerCallback() override;

private:
    //==============================================================================
    // Command IDs
    enum CommandIDs
    {
        newProject      = 0x2001,
        openProject     = 0x2002,
        saveProject     = 0x2003,
        saveProjectAs   = 0x2004,
        closeProject    = 0x2005,

        undo            = 0x2010,
        redo            = 0x2011,

        play            = 0x2020,
        stop            = 0x2021,
        record          = 0x2022,
        loop            = 0x2023,

        createAudioTrack    = 0x2030,
        createMIDITrack     = 0x2031,
        createInstrumentTrack = 0x2032,

        showSettings    = 0x2040,
        showPluginManager = 0x2041
    };

    //==============================================================================
    // Core systems
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<ProjectState> projectState;
    std::unique_ptr<ProjectManager> projectManager;

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::ApplicationCommandManager commandManager;

    //==============================================================================
    // GUI Components
    TransportComponent transportBar;
    BrowserComponent leftPanel;
    ArrangementComponent centerPanel;
    MixerComponent rightPanel;

    //==============================================================================
    // Initialization methods
    void setupAudioDevice();
    void setupKeyboardShortcuts();
    void setupMenuBar();

    //==============================================================================
    // File operations
    bool checkForUnsavedChanges();
    void createNewProject();
    void openProject();
    void saveCurrentProject();
    void saveProjectAs();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
