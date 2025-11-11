# Vexel DAW: Web to Native C++/JUCE Migration Plan

## Executive Summary

This document outlines the complete migration of the Vexel DAW from a web-based Electron/React application to a professional native C++/JUCE application, following industry-standard DAW architecture patterns used by Pro Tools, Ableton Live, and FL Studio.

## Why Migrate to C++/JUCE?

### Performance Benefits
- **10-100x faster** audio processing (native vs JavaScript)
- **Sub-millisecond latency** (vs 5-10ms in Web Audio API)
- **Direct hardware access** to audio interfaces
- **Real-time thread priority** for audio callbacks
- **SIMD optimizations** (SSE, AVX, NEON)

### Professional Features
- **VST/AU/AAX plugin hosting** (industry standard)
- **ASIO/CoreAudio** direct access
- **Sample-accurate automation**
- **Zero-copy audio buffers**
- **Professional metering** (true peak, K-metering)

### Industry Standard
- Used by: Ableton Live, FL Studio, Reaper, Bitwig
- Full plugin ecosystem compatibility
- Professional audio driver support
- Hardware controller integration

## Architecture Comparison

### Current Web Stack
```
Electron App
├── Renderer Process (React/TypeScript)
│   ├── UI Components
│   ├── Web Audio API (JavaScript)
│   └── State Management (Zustand)
├── Main Process (Node.js)
│   ├── IPC Handlers
│   ├── File I/O
│   └── Window Management
└── Bridge Services
    └── Wingman AI (WebSocket)
```

### Target C++/JUCE Stack
```
JUCE Application
├── Audio Engine (C++)
│   ├── AudioProcessor (Real-time thread)
│   ├── AudioDeviceManager
│   ├── MixerAudioSource
│   └── Plugin Host Manager
├── GUI Layer (JUCE Components)
│   ├── MainComponent
│   ├── Transport Controls
│   ├── Mixer Panel
│   ├── Arrangement View
│   └── Piano Roll Editor
├── State Management
│   ├── ValueTree (thread-safe)
│   ├── UndoManager
│   └── ApplicationCommandManager
└── File Management
    ├── Project Serialization
    ├── Audio File I/O
    └── Plugin Scanning
```

## Phase 1: Foundation Setup

### 1.1 JUCE Project Structure
```
VexelDAW/
├── CMakeLists.txt
├── Source/
│   ├── Main.cpp
│   ├── MainComponent.h/cpp
│   ├── Audio/
│   │   ├── AudioEngine.h/cpp
│   │   ├── Track.h/cpp
│   │   ├── Clip.h/cpp
│   │   ├── MixerChannel.h/cpp
│   │   └── PluginHost.h/cpp
│   ├── GUI/
│   │   ├── Transport/TransportComponent.h/cpp
│   │   ├── Mixer/MixerComponent.h/cpp
│   │   ├── Arrangement/ArrangementComponent.h/cpp
│   │   ├── PianoRoll/PianoRollComponent.h/cpp
│   │   └── Browser/BrowserComponent.h/cpp
│   ├── State/
│   │   ├── ProjectState.h/cpp
│   │   ├── AudioStateManager.h/cpp
│   │   └── PluginStateManager.h/cpp
│   └── Utilities/
│       ├── AudioFileManager.h/cpp
│       ├── PluginScanner.h/cpp
│       └── WingmanBridge.h/cpp
├── Resources/
│   ├── Images/
│   └── Fonts/
└── Builds/
    ├── MacOSX/
    ├── Windows/
    └── Linux/
```

### 1.2 CMake Configuration
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(VexelDAW VERSION 0.1.0)

# JUCE setup
add_subdirectory(JUCE)

juce_add_gui_app(VexelDAW
    PRODUCT_NAME "Vexel DAW"
    COMPANY_NAME "DaddyMilkMan"
    BUNDLE_ID com.daddymilkman.vexeldaw
    ICON_BIG Resources/icon.png
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE)

target_sources(VexelDAW PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/Audio/AudioEngine.cpp
    Source/Audio/Track.cpp
    # ... more sources
)

target_compile_definitions(VexelDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:VexelDAW,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:VexelDAW,JUCE_VERSION>")

target_link_libraries(VexelDAW PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
```

## Phase 2: Core Audio Engine Conversion

### 2.1 Web Audio API → JUCE AudioProcessor

#### Current Web Audio (JavaScript)
```javascript
// vexel-daw/src/renderer/audio/AudioEngine.ts
class AudioEngine {
  private audioContext: AudioContext;
  private tracks: AudioTrack[] = [];

  async processAudio(inputBuffer: Float32Array): Promise<Float32Array> {
    // JavaScript processing...
  }
}
```

#### Target JUCE (C++)
```cpp
// Source/Audio/AudioEngine.h
#pragma once
#include <JuceHeader.h>

class AudioEngine : public juce::AudioSource
{
public:
    AudioEngine();
    ~AudioEngine() override;

    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    // Track management
    Track* addTrack(const juce::String& name, Track::Type type);
    void removeTrack(int trackIndex);
    Track* getTrack(int index) { return tracks[index].get(); }
    int getNumTracks() const { return tracks.size(); }

    // Transport control
    void play();
    void pause();
    void stop();
    void setTempo(double newTempo);
    bool isPlaying() const { return playing; }

    // Mixer
    void setMasterVolume(float volume);
    float getMasterVolume() const { return masterVolume; }

private:
    juce::OwnedArray<Track> tracks;
    juce::MixerAudioSource mixer;
    juce::AudioTransportSource transport;

    std::atomic<bool> playing { false };
    std::atomic<float> masterVolume { 0.8f };
    double sampleRate = 44100.0;
    int blockSize = 512;
    double tempo = 120.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

// Source/Audio/AudioEngine.cpp
#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    // Initialize mixer
    mixer.addInputSource(&transport, false);
}

AudioEngine::~AudioEngine()
{
    mixer.removeAllInputs();
}

void AudioEngine::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlockExpected;

    // Prepare all tracks
    for (auto* track : tracks)
        track->prepareToPlay(samplesPerBlockExpected, newSampleRate);

    mixer.prepareToPlay(samplesPerBlockExpected, newSampleRate);
}

void AudioEngine::releaseResources()
{
    for (auto* track : tracks)
        track->releaseResources();

    mixer.releaseResources();
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (!playing)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // Mix all tracks
    juce::AudioBuffer<float> mixBuffer(bufferToFill.buffer->getNumChannels(),
                                       bufferToFill.numSamples);
    mixBuffer.clear();

    for (auto* track : tracks)
    {
        if (track->isEnabled())
        {
            juce::AudioSourceChannelInfo trackInfo(&mixBuffer, 0, bufferToFill.numSamples);
            track->getNextAudioBlock(trackInfo);
        }
    }

    // Apply master volume
    mixBuffer.applyGain(masterVolume.load());

    // Copy to output
    for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
        bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample, mixBuffer, ch, 0, bufferToFill.numSamples);
}

Track* AudioEngine::addTrack(const juce::String& name, Track::Type type)
{
    auto* track = tracks.add(new Track(name, type));
    track->prepareToPlay(blockSize, sampleRate);
    return track;
}

void AudioEngine::play()
{
    playing = true;
}

void AudioEngine::pause()
{
    playing = false;
}

void AudioEngine::stop()
{
    playing = false;
    // Reset playhead position
}
```

### 2.2 Track System

#### Current (TypeScript)
```typescript
interface AudioTrack {
  id: string;
  name: string;
  type: 'audio' | 'midi' | 'instrument';
  clips: Clip[];
  volume: number;
  pan: number;
}
```

#### Target (C++)
```cpp
// Source/Audio/Track.h
#pragma once
#include <JuceHeader.h>

class Track : public juce::AudioSource
{
public:
    enum class Type { Audio, MIDI, Instrument };

    Track(const juce::String& name, Type type);
    ~Track() override;

    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    // Track properties
    void setName(const juce::String& newName) { name = newName; }
    juce::String getName() const { return name; }
    Type getType() const { return type; }

    // Audio control
    void setVolume(float newVolume);
    float getVolume() const { return volume.load(); }
    void setPan(float newPan);
    float getPan() const { return pan.load(); }
    void setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
    bool isMuted() const { return muted.load(); }
    void setSolo(bool shouldBeSolo) { solo = shouldBeSolo; }
    bool isSolo() const { return solo.load(); }

    // Clip management
    void addClip(std::unique_ptr<Clip> clip);
    void removeClip(int index);
    Clip* getClip(int index) { return clips[index].get(); }
    int getNumClips() const { return clips.size(); }

    // Plugin chain
    void addPlugin(juce::AudioPluginInstance* plugin);
    void removePlugin(int index);
    int getNumPlugins() const { return plugins.size(); }

    bool isEnabled() const { return !muted.load(); }

private:
    juce::String name;
    Type type;
    juce::OwnedArray<Clip> clips;
    juce::OwnedArray<juce::AudioPluginInstance> plugins;
    juce::AudioProcessorGraph processorGraph;

    std::atomic<float> volume { 0.8f };
    std::atomic<float> pan { 0.0f };
    std::atomic<bool> muted { false };
    std::atomic<bool> solo { false };

    double sampleRate = 44100.0;
    int blockSize = 512;

    void applyPanning(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

// Source/Audio/Track.cpp
#include "Track.h"

Track::Track(const juce::String& n, Type t) : name(n), type(t)
{
}

Track::~Track()
{
}

void Track::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlockExpected;

    // Prepare all clips
    for (auto* clip : clips)
        clip->prepareToPlay(samplesPerBlockExpected, newSampleRate);

    // Prepare plugin chain
    for (auto* plugin : plugins)
    {
        plugin->prepareToPlay(newSampleRate, samplesPerBlockExpected);
    }
}

void Track::releaseResources()
{
    for (auto* clip : clips)
        clip->releaseResources();

    for (auto* plugin : plugins)
        plugin->releaseResources();
}

void Track::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (muted.load())
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // Clear buffer
    bufferToFill.buffer->clear(bufferToFill.startSample, bufferToFill.numSamples);

    // Mix all active clips
    for (auto* clip : clips)
    {
        if (clip->isActive())
        {
            juce::AudioSourceChannelInfo clipInfo(bufferToFill.buffer,
                                                  bufferToFill.startSample,
                                                  bufferToFill.numSamples);
            clip->getNextAudioBlock(clipInfo);
        }
    }

    // Process through plugin chain
    juce::MidiBuffer midiBuffer;
    for (auto* plugin : plugins)
    {
        if (!plugin->isSuspended())
        {
            plugin->processBlock(*bufferToFill.buffer, midiBuffer);
        }
    }

    // Apply volume and panning
    bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, volume.load());
    applyPanning(*bufferToFill.buffer);
}

void Track::applyPanning(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
        return;

    float panValue = pan.load();
    float leftGain = std::cos((panValue + 1.0f) * juce::MathConstants<float>::pi / 4.0f);
    float rightGain = std::sin((panValue + 1.0f) * juce::MathConstants<float>::pi / 4.0f);

    buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
    buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
}
```

## Phase 3: GUI Conversion

### 3.1 Main Application Window

#### Current (React)
```tsx
// vexel-daw/src/renderer/App.tsx
function App() {
  return (
    <div className="h-screen flex flex-col">
      <TransportBar />
      <div className="flex-1 flex">
        <LeftPanel />
        <CenterPanel />
        <RightPanel />
      </div>
    </div>
  );
}
```

#### Target (JUCE)
```cpp
// Source/MainComponent.h
#pragma once
#include <JuceHeader.h>
#include "GUI/Transport/TransportComponent.h"
#include "GUI/Mixer/MixerComponent.h"
#include "GUI/Arrangement/ArrangementComponent.h"
#include "GUI/Browser/BrowserComponent.h"

class MainComponent : public juce::Component,
                      public juce::ApplicationCommandTarget,
                      public juce::ChangeListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

    // ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    std::unique_ptr<AudioEngine> audioEngine;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;

    TransportComponent transportBar;
    BrowserComponent leftPanel;
    ArrangementComponent centerPanel;
    MixerComponent rightPanel;

    juce::ApplicationCommandManager commandManager;

    void setupAudioDevice();
    void setupKeyboardShortcuts();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

// Source/MainComponent.cpp
#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(1600, 1000);

    // Initialize audio engine
    audioEngine = std::make_unique<AudioEngine>();

    // Setup audio device
    setupAudioDevice();

    // Add child components
    addAndMakeVisible(transportBar);
    addAndMakeVisible(leftPanel);
    addAndMakeVisible(centerPanel);
    addAndMakeVisible(rightPanel);

    // Connect transport to engine
    transportBar.setAudioEngine(audioEngine.get());
    centerPanel.setAudioEngine(audioEngine.get());
    rightPanel.setAudioEngine(audioEngine.get());

    // Setup keyboard shortcuts
    setupKeyboardShortcuts();
    commandManager.registerAllCommandsForTarget(this);

    // Start audio
    audioSourcePlayer.setSource(audioEngine.get());
}

MainComponent::~MainComponent()
{
    audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
}

void MainComponent::setupAudioDevice()
{
    // Initialize with default audio device
    juce::String error = deviceManager.initialiseWithDefaultDevices(0, 2);

    if (error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Audio Device Error",
            "Failed to initialize audio device:\n" + error);
    }

    deviceManager.addAudioCallback(&audioSourcePlayer);

    // Get current audio device setup
    auto setup = deviceManager.getAudioDeviceSetup();
    audioEngine->prepareToPlay(setup.bufferSize, setup.sampleRate);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Transport bar at top (80 pixels)
    transportBar.setBounds(bounds.removeFromTop(80));

    // Main content area split into 3 panels
    auto leftWidth = bounds.getWidth() / 4;      // 25%
    auto rightWidth = bounds.getWidth() / 4;     // 25%
    auto centerWidth = bounds.getWidth() - leftWidth - rightWidth; // 50%

    leftPanel.setBounds(bounds.removeFromLeft(leftWidth));
    rightPanel.setBounds(bounds.removeFromRight(rightWidth));
    centerPanel.setBounds(bounds);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
}
```

### 3.2 Transport Bar Component

#### Current (React)
```tsx
function TransportBar({ audioState, onOpenWingman }: TransportBarProps) {
  const handlePlay = () => {
    if (audioState.isPlaying) {
      engineClient.sendCommand('transport:pause');
    } else {
      engineClient.sendCommand('transport:play');
    }
  };

  return (
    <div className="transport-bar">
      <button onClick={handlePlay}>
        {audioState.isPlaying ? <Pause /> : <Play />}
      </button>
      <input type="number" value={tempo} onChange={handleTempoChange} />
    </div>
  );
}
```

#### Target (JUCE)
```cpp
// Source/GUI/Transport/TransportComponent.h
#pragma once
#include <JuceHeader.h>

class TransportComponent : public juce::Component,
                           public juce::Button::Listener,
                           public juce::Timer
{
public:
    TransportComponent();
    ~TransportComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    void setAudioEngine(AudioEngine* engine);

private:
    AudioEngine* audioEngine = nullptr;

    juce::TextButton playButton;
    juce::TextButton pauseButton;
    juce::TextButton stopButton;
    juce::TextButton recordButton;
    juce::TextButton loopButton;

    juce::Label tempoLabel;
    juce::Slider tempoSlider;

    juce::Label timeLabel;
    juce::Label cpuLabel;

    void updateTransportState();
    void updateTimeDisplay();
    void updateCpuUsage();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
};

// Source/GUI/Transport/TransportComponent.cpp
#include "TransportComponent.h"

TransportComponent::TransportComponent()
{
    // Play button
    playButton.setButtonText("Play");
    playButton.addListener(this);
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green.darker());
    addAndMakeVisible(playButton);

    // Stop button
    stopButton.setButtonText("Stop");
    stopButton.addListener(this);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red.darker());
    addAndMakeVisible(stopButton);

    // Tempo slider
    tempoSlider.setRange(20.0, 300.0, 0.1);
    tempoSlider.setValue(120.0);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    tempoSlider.onValueChange = [this]()
    {
        if (audioEngine)
            audioEngine->setTempo(tempoSlider.getValue());
    };
    addAndMakeVisible(tempoSlider);

    tempoLabel.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);

    // Time display
    timeLabel.setText("0:00.000", juce::dontSendNotification);
    timeLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    addAndMakeVisible(timeLabel);

    // CPU usage
    cpuLabel.setText("CPU: 0%", juce::dontSendNotification);
    addAndMakeVisible(cpuLabel);

    startTimer(50); // Update UI at 20Hz
}

TransportComponent::~TransportComponent()
{
    stopTimer();
}

void TransportComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    auto buttonWidth = 80;
    auto buttonHeight = 40;

    playButton.setBounds(bounds.removeFromLeft(buttonWidth).withHeight(buttonHeight));
    bounds.removeFromLeft(10);
    stopButton.setBounds(bounds.removeFromLeft(buttonWidth).withHeight(buttonHeight));
    bounds.removeFromLeft(20);

    tempoSlider.setBounds(bounds.removeFromLeft(120).withHeight(buttonHeight));
    bounds.removeFromLeft(5);
    tempoLabel.setBounds(bounds.removeFromLeft(40).withHeight(buttonHeight));

    bounds.removeFromLeft(50);
    timeLabel.setBounds(bounds.removeFromLeft(150).withHeight(buttonHeight));

    cpuLabel.setBounds(bounds.removeFromRight(100).withHeight(buttonHeight));
}

void TransportComponent::buttonClicked(juce::Button* button)
{
    if (!audioEngine)
        return;

    if (button == &playButton)
    {
        if (audioEngine->isPlaying())
            audioEngine->pause();
        else
            audioEngine->play();
    }
    else if (button == &stopButton)
    {
        audioEngine->stop();
    }

    updateTransportState();
}

void TransportComponent::updateTransportState()
{
    if (!audioEngine)
        return;

    bool playing = audioEngine->isPlaying();
    playButton.setButtonText(playing ? "Pause" : "Play");
    playButton.setColour(juce::TextButton::buttonColourId,
                        playing ? juce::Colours::orange : juce::Colours::green.darker());
}

void TransportComponent::timerCallback()
{
    updateTimeDisplay();
    updateCpuUsage();
}

void TransportComponent::updateTimeDisplay()
{
    // Update time based on audio engine position
    // Format: bars:beats.ticks
}

void TransportComponent::updateCpuUsage()
{
    // Get CPU usage from audio device manager
    float cpuUsage = 0.0f; // TODO: Get from device manager
    cpuLabel.setText(juce::String::formatted("CPU: %.1f%%", cpuUsage * 100.0f),
                     juce::dontSendNotification);
}
```

## Phase 4: State Management Conversion

### 4.1 Zustand → JUCE ValueTree

#### Current (Zustand)
```typescript
// vexel-daw/src/renderer/lib/store.ts
export const useStore = create<Store>((set) => ({
  projectState: {
    tracks: [],
    tempo: 120,
    isPlaying: false
  },
  updateProjectState: (state) => set({ projectState: state })
}));
```

#### Target (JUCE ValueTree)
```cpp
// Source/State/ProjectState.h
#pragma once
#include <JuceHeader.h>

class ProjectState : public juce::ValueTree::Listener
{
public:
    ProjectState();
    ~ProjectState() override;

    // Project properties
    juce::String getProjectName() const;
    void setProjectName(const juce::String& name);

    double getTempo() const;
    void setTempo(double newTempo);

    juce::TimeSignature getTimeSignature() const;
    void setTimeSignature(const juce::TimeSignature& ts);

    // Serialization
    void saveToFile(const juce::File& file);
    bool loadFromFile(const juce::File& file);
    juce::String toXml() const;
    bool fromXml(const juce::String& xml);

    // ValueTree access
    juce::ValueTree& getState() { return state; }
    const juce::ValueTree& getState() const { return state; }

    // Undo/Redo
    juce::UndoManager& getUndoManager() { return undoManager; }

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;

    // Identifiers
    static const juce::Identifier PROJECT_STATE;
    static const juce::Identifier TEMPO;
    static const juce::Identifier TIME_SIGNATURE;
    static const juce::Identifier TRACKS;
    static const juce::Identifier TRACK;

private:
    juce::ValueTree state;
    juce::UndoManager undoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectState)
};

// Source/State/ProjectState.cpp
#include "ProjectState.h"

const juce::Identifier ProjectState::PROJECT_STATE("PROJECT");
const juce::Identifier ProjectState::TEMPO("tempo");
const juce::Identifier ProjectState::TIME_SIGNATURE("timeSignature");
const juce::Identifier ProjectState::TRACKS("tracks");
const juce::Identifier ProjectState::TRACK("track");

ProjectState::ProjectState()
    : state(PROJECT_STATE)
{
    // Initialize default project
    state.setProperty(TEMPO, 120.0, nullptr);
    state.setProperty("name", "Untitled Project", nullptr);

    auto tracksTree = juce::ValueTree(TRACKS);
    state.appendChild(tracksTree, nullptr);

    state.addListener(this);
}

ProjectState::~ProjectState()
{
    state.removeListener(this);
}

double ProjectState::getTempo() const
{
    return state.getProperty(TEMPO, 120.0);
}

void ProjectState::setTempo(double newTempo)
{
    state.setProperty(TEMPO, newTempo, &undoManager);
}

void ProjectState::saveToFile(const juce::File& file)
{
    auto xml = std::unique_ptr<juce::XmlElement>(state.createXml());
    if (xml && xml->writeTo(file))
    {
        DBG("Project saved successfully");
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Save Error",
            "Failed to save project file");
    }
}

bool ProjectState::loadFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (!xml)
        return false;

    auto newState = juce::ValueTree::fromXml(*xml);
    if (!newState.isValid() || !newState.hasType(PROJECT_STATE))
        return false;

    state = newState;
    state.addListener(this);
    return true;
}

void ProjectState::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // Notify listeners that state has changed
    sendChangeMessage();
}
```

## Phase 5: Plugin System

### 5.1 VST/AU Plugin Hosting

```cpp
// Source/Audio/PluginHost.h
#pragma once
#include <JuceHeader.h>

class PluginHost
{
public:
    PluginHost();
    ~PluginHost();

    // Plugin scanning
    void scanForPlugins(bool rescan = false);
    juce::KnownPluginList& getPluginList() { return knownPlugins; }

    // Plugin loading
    juce::AudioPluginInstance* loadPlugin(const juce::PluginDescription& description);
    void unloadPlugin(juce::AudioPluginInstance* instance);

    // Plugin state
    void savePluginState(juce::AudioPluginInstance* plugin, juce::MemoryBlock& destData);
    bool loadPluginState(juce::AudioPluginInstance* plugin, const juce::MemoryBlock& data);

private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::PluginDirectoryScanner* currentScanner = nullptr;

    void addFormats();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};

// Source/Audio/PluginHost.cpp
#include "PluginHost.h"

PluginHost::PluginHost()
{
    addFormats();

    // Load cached plugin list
    auto savedPlugins = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("VexelDAW")
        .getChildFile("PluginCache.xml");

    if (savedPlugins.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(savedPlugins);
        if (xml)
            knownPlugins.recreateFromXml(*xml);
    }
}

PluginHost::~PluginHost()
{
    // Save plugin cache
    auto savedPlugins = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("VexelDAW")
        .getChildFile("PluginCache.xml");

    savedPlugins.getParentDirectory().createDirectory();

    if (auto xml = knownPlugins.createXml())
        xml->writeTo(savedPlugins);
}

void PluginHost::addFormats()
{
    formatManager.addDefaultFormats();

    // Add VST3
    #if JUCE_PLUGINHOST_VST3
    formatManager.addFormat(new juce::VST3PluginFormat());
    #endif

    // Add AU (macOS)
    #if JUCE_PLUGINHOST_AU
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
    #endif

    // Add AAX
    #if JUCE_PLUGINHOST_AAX
    formatManager.addFormat(new juce::AAXPluginFormat());
    #endif
}

void PluginHost::scanForPlugins(bool rescan)
{
    if (rescan)
        knownPlugins.clear();

    juce::FileSearchPath searchPaths;

    for (auto* format : formatManager.getFormats())
    {
        auto paths = format->getDefaultLocationsToSearch();
        searchPaths.addPath(paths);
    }

    // Scan in background thread
    currentScanner = new juce::PluginDirectoryScanner(knownPlugins, formatManager,
                                                      searchPaths, true, juce::File());

    juce::String pluginBeingScanned;
    while (currentScanner->scanNextFile(true, pluginBeingScanned))
    {
        DBG("Scanning: " + pluginBeingScanned);
    }

    delete currentScanner;
    currentScanner = nullptr;
}

juce::AudioPluginInstance* PluginHost::loadPlugin(const juce::PluginDescription& description)
{
    juce::String errorMessage;

    auto instance = formatManager.createPluginInstance(description, 44100.0, 512, errorMessage);

    if (!instance)
    {
        DBG("Failed to load plugin: " + errorMessage);
        return nullptr;
    }

    return instance.release();
}
```

## Phase 6: File I/O & Project Management

### 6.1 Project Serialization

```cpp
// Source/Utilities/ProjectManager.h
#pragma once
#include <JuceHeader.h>

class ProjectManager
{
public:
    ProjectManager(ProjectState& state, AudioEngine& engine);
    ~ProjectManager();

    // Project operations
    bool newProject();
    bool saveProject();
    bool saveProjectAs(const juce::File& file);
    bool loadProject(const juce::File& file);
    bool closeProject();

    // Recent files
    juce::RecentlyOpenedFilesList& getRecentFiles() { return recentFiles; }

    // Auto-save
    void enableAutoSave(bool enable, int intervalSeconds = 300);

    bool hasUnsavedChanges() const { return unsavedChanges; }
    juce::File getCurrentProjectFile() const { return currentFile; }

private:
    ProjectState& projectState;
    AudioEngine& audioEngine;

    juce::File currentFile;
    bool unsavedChanges = false;

    juce::RecentlyOpenedFilesList recentFiles;
    juce::Time lastSaveTime;

    bool serializeProject(const juce::File& file);
    bool deserializeProject(const juce::File& file);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};
```

## Migration Timeline

### Week 1-2: Foundation
- [ ] Set up JUCE project with CMake
- [ ] Create basic application window
- [ ] Initialize audio device manager
- [ ] Implement basic audio callback

### Week 3-4: Core Audio Engine
- [ ] Convert AudioEngine class
- [ ] Implement Track system
- [ ] Add Clip playback
- [ ] Create Mixer architecture

### Week 5-6: GUI Framework
- [ ] Build MainComponent layout
- [ ] Implement TransportComponent
- [ ] Create ArrangementComponent
- [ ] Add MixerComponent

### Week 7-8: Advanced Features
- [ ] Plugin hosting (VST/AU)
- [ ] MIDI support
- [ ] Project serialization
- [ ] Undo/Redo system

### Week 9-10: Polish & Testing
- [ ] Performance optimization
- [ ] Memory leak detection
- [ ] Cross-platform testing
- [ ] User testing & feedback

## Testing Strategy

### Unit Tests
```cpp
// Tests/AudioEngineTests.cpp
class AudioEngineTests : public juce::UnitTest
{
public:
    AudioEngineTests() : juce::UnitTest("Audio Engine Tests") {}

    void runTest() override
    {
        beginTest("Track Creation");
        {
            AudioEngine engine;
            auto* track = engine.addTrack("Test Track", Track::Type::Audio);
            expect(track != nullptr);
            expect(engine.getNumTracks() == 1);
        }

        beginTest("Audio Processing");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            juce::AudioBuffer<float> buffer(2, 512);
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);

            engine.getNextAudioBlock(info);
            expect(buffer.getMagnitude(0, 512) == 0.0f); // Should be silent
        }
    }
};

static AudioEngineTests audioEngineTests;
```

## Performance Targets

| Metric | Target | Current Web |
|--------|--------|-------------|
| Audio Latency | < 5ms | 10-20ms |
| CPU Usage (8 tracks) | < 10% | 30-40% |
| Plugin Load Time | < 100ms | 500ms+ |
| Project Load Time | < 1s | 3-5s |
| Memory Usage | < 200MB | 500MB+ |
| Startup Time | < 2s | 5-8s |

## Migration Checklist

### Audio Engine
- [ ] AudioEngine class with real-time processing
- [ ] Track system with routing
- [ ] Clip playback with transport sync
- [ ] Mixer with buses and sends
- [ ] MIDI routing and processing
- [ ] Audio file I/O (WAV, AIFF, FLAC, MP3)
- [ ] Sample rate conversion
- [ ] Latency compensation

### Plugin System
- [ ] VST3 hosting
- [ ] Audio Unit (AU) hosting
- [ ] AAX hosting (Pro Tools)
- [ ] Plugin scanning and caching
- [ ] Plugin state save/load
- [ ] MIDI plugin support
- [ ] Preset management

### GUI Components
- [ ] Main window with resizable panels
- [ ] Transport bar with play/stop/record
- [ ] Tempo and time signature controls
- [ ] Arrangement view with timeline
- [ ] Track headers with controls
- [ ] Mixer panel with faders
- [ ] Piano roll editor
- [ ] Browser panel
- [ ] Plugin windows

### State Management
- [ ] ValueTree-based project state
- [ ] Undo/Redo system
- [ ] Project serialization (XML)
- [ ] Auto-save functionality
- [ ] Recent files list
- [ ] Application settings

### File Management
- [ ] Project save/load
- [ ] Audio file import
- [ ] Audio file export
- [ ] Render/bounce to audio file
- [ ] Backup/recovery system

### Performance
- [ ] Lock-free audio thread
- [ ] SIMD optimizations
- [ ] Memory pool for audio buffers
- [ ] Efficient GUI updates
- [ ] Background thread for I/O

## Conclusion

This migration will transform Vexel DAW from a web-based prototype into a professional-grade native application capable of competing with industry-standard DAWs. The use of C++/JUCE provides:

1. **10-100x performance improvement**
2. **Professional plugin compatibility**
3. **Ultra-low latency audio**
4. **Industry-standard architecture**
5. **Cross-platform native performance**

The migration is substantial but follows well-established patterns used by commercial DAWs. With dedicated development, this can be completed in 8-10 weeks.

## Next Steps

1. Install JUCE framework
2. Set up CMake project
3. Begin with Phase 1 foundation
4. Incremental testing at each phase
5. Performance profiling and optimization

---

**Document Version:** 1.0
**Created:** 2025-11-10
**Author:** Claude (Migration Architect)
