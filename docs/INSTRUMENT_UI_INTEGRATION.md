# Instrument UI Integration Guide

This document describes how the instrument UIs integrate with the main DAW and the preset management system.

## Overview

The Zenith DAW now includes comprehensive JUCE-based UIs for built-in instruments:

1. **ZenithPolySynthEditor** - Full-featured synthesizer UI with oscillators, filters, envelopes, LFOs, and macro controls
2. **ZenithSamplerEditor** - Sampler UI with sample map table, envelope, filter, and global controls
3. **PresetBrowserComponent** - Reusable preset browser with filtering and save/load capabilities

## Architecture

### Component Hierarchy

```
MainWindow
  └─ MainComponent
       └─ [Instrument Hosting Options]
            ├─ Direct Component Embedding
            ├─ PluginEditorWindow (for AudioProcessorEditor instances)
            └─ Separate DocumentWindow
```

### Integration Flow

```
User Action
    ↓
Main UI / CommandAPI
    ↓
Instrument Instance
    ↓
Create Editor (with ZenithPresetManager)
    ↓
Editor ← → InstrumentPresetManager ← → Preset Files (.zpreset)
    ↓
Parameter Updates via APVTS or Instrument API
```

## Opening Instrument Editors

### Method 1: From CommandAPI (Programmatic)

The CommandAPI provides endpoints to open instrument editors:

```cpp
// Example: Open PolySynth editor via CommandAPI
commandAPI.openInstrumentEditor("zenith_poly_synth", instanceId);
```

### Method 2: From Main UI (User Action)

Users can open instrument editors through:

**Option A**: Instrument Browser/Manager Panel
```cpp
// In MainComponent or InstrumentManager
void MainComponent::onInstrumentDoubleClick(const juce::String& instrumentId)
{
    // Get or create instrument instance
    auto* instrument = instrumentRegistry.getInstance(instrumentId);

    // Create editor
    if (auto* polySynth = dynamic_cast<ZenithPolySynth*>(instrument))
    {
        auto editor = std::make_unique<ZenithPolySynthEditor>(
            *polySynth,
            presetManager  // Global preset manager
        );

        // Show in window
        showInstrumentEditorWindow(std::move(editor), polySynth->getMetadata().name);
    }
}
```

**Option B**: Track Inspector/Channel Strip
```cpp
// When user clicks "Edit Instrument" on a track
void TrackInspector::onEditInstrumentClicked()
{
    auto* track = getCurrentTrack();
    auto* instrument = track->getInstrument();

    if (auto* sampler = dynamic_cast<ZenithSampler*>(instrument))
    {
        auto processor = sampler->getAudioProcessor();
        auto editor = std::make_unique<ZenithSamplerEditor>(
            *dynamic_cast<ZenithSamplerProcessor*>(processor),
            *sampler,
            globalPresetManager
        );

        showInstrumentEditorWindow(std::move(editor), "Zenith Sampler");
    }
}
```

### Method 3: Using PluginEditorWindow (Recommended)

For AudioProcessorEditor-based editors like ZenithSamplerEditor:

```cpp
// ZenithSampler.h - Add createEditor override
class ZenithSamplerProcessor : public juce::AudioProcessor
{
public:
    juce::AudioProcessorEditor* createEditor() override
    {
        // Need access to ZenithSampler instance and preset manager
        // This requires a reference to be stored during construction
        return new ZenithSamplerEditor(*this, instrument_, presetManager_);
    }

private:
    ZenithSampler& instrument_;
    ZenithPresetManager& presetManager_;
};
```

Then use the existing PluginEditorWindow system:

```cpp
// In MainComponent or PluginEditorWindowManager
void openSamplerEditor(ZenithSamplerProcessor* processor)
{
    pluginEditorWindowManager.openEditor(processor);
}
```

## Preset Management Integration

### How Editors Talk to InstrumentPresetManager

#### 1. Initialization

When an editor is created, it receives a reference to `ZenithPresetManager`:

```cpp
ZenithPolySynthEditor::ZenithPolySynthEditor(
    ZenithPolySynth& instrument,
    ZenithPresetManager& presetManager)
    : instrument_(instrument)
    , presetManager_(presetManager)
{
    // Create preset browser
    presetBrowser_ = std::make_unique<PresetBrowserComponent>(
        instrument.getMetadata().instrumentId,
        presetManager_);

    // Set up callbacks
    presetBrowser_->setLoadPresetCallback([this](const auto& preset) {
        onPresetLoaded(preset);
    });

    presetBrowser_->setCaptureStateCallback([this]() {
        return captureCurrentState();
    });
}
```

#### 2. Loading Presets

When a preset is loaded from the browser:

```cpp
void ZenithPolySynthEditor::onPresetLoaded(const ZenithInstrumentPreset& preset)
{
    // Apply all parameters from preset
    for (const auto& [paramId, value] : preset.parameters)
    {
        instrument_.setParameter(juce::String(paramId), value);
    }

    // Apply macros
    for (const auto& [macroId, value] : preset.macros)
    {
        instrument_.setMacro(juce::String(macroId), value);
    }
}
```

#### 3. Saving Presets

When user clicks "Save As" in the preset browser:

```cpp
std::map<std::string, float> ZenithPolySynthEditor::captureCurrentState()
{
    std::map<std::string, float> state;

    // Capture all parameters from metadata
    const auto& metadata = instrument_.getMetadata();
    for (const auto& param : metadata.parameters)
    {
        float value = instrument_.getParameter(param.id);
        state[param.id.toStdString()] = value;
    }

    return state;
}
```

The PresetBrowserComponent then:
1. Creates a new `ZenithInstrumentPreset` with captured state
2. Prompts user for name, tags, description
3. Calls `presetManager_.saveUserPreset(newPreset)`
4. Refreshes the preset list

### Preset File Storage

Presets are stored in the filesystem:

```
~/Zenith/Instruments/
├── Factory/
│   ├── zenith_poly_synth/
│   │   ├── Init Basic Pad.zpreset
│   │   ├── Bright Pluck.zpreset
│   │   └── LoFi Keys 01.zpreset
│   └── zenith_sampler/
│       ├── Piano.zpreset
│       └── Strings.zpreset
└── User/
    ├── zenith_poly_synth/
    │   └── My Custom Patch.zpreset
    └── zenith_sampler/
        └── My Drums.zpreset
```

### CommandAPI Integration (Non-UI Access)

The CommandAPI can query and load presets without opening the UI:

```cpp
// Get available presets for an instrument
juce::var CommandAPI::getPresetsForInstrument(const juce::String& instrumentId)
{
    auto presets = presetManager.getPresetsForInstrument(instrumentId.toStdString());

    juce::Array<juce::var> presetArray;
    for (const auto& preset : presets)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", preset.id);
        obj->setProperty("name", preset.name);
        obj->setProperty("author", preset.author);
        // ... other properties
        presetArray.add(juce::var(obj));
    }

    return presetArray;
}

// Load a preset programmatically
bool CommandAPI::loadPreset(const juce::String& instrumentId, const juce::String& presetId)
{
    auto* instrument = instrumentRegistry.getInstance(instrumentId);
    if (!instrument)
        return false;

    // Load preset from manager
    auto presets = presetManager.getPresetsForInstrument(instrumentId.toStdString());
    for (const auto& preset : presets)
    {
        if (preset.id == presetId.toStdString())
        {
            // Apply parameters
            for (const auto& [paramId, value] : preset.parameters)
            {
                instrument->setParameter(juce::String(paramId), value);
            }

            return true;
        }
    }

    return false;
}
```

## UI Components

### PresetBrowserComponent

**Features:**
- Search by name
- Filter by category (Factory/User)
- Filter by tags
- Load preset on click
- Save current state as new preset

**Usage:**
```cpp
auto presetBrowser = std::make_unique<PresetBrowserComponent>(
    "zenith_poly_synth",
    presetManager);

presetBrowser->setLoadPresetCallback([](const auto& preset) {
    // Apply preset
});

presetBrowser->setCaptureStateCallback([]() {
    // Return current parameter state
    return captureState();
});
```

### ZenithPolySynthEditor

**Sections:**
- Oscillator: Waveform, detune, level, unison
- Filter: Type, cutoff, resonance, drive
- Envelopes: Amp ADSR, Filter ADSR
- LFOs: 2 LFOs with routing and targets
- Global: Master gain, mono/poly, glide, voices
- Macros: Smart macro knobs (dynamically loaded from metadata)
- Preset Browser: Toggle-able preset browser panel

**Size:** 900x700 pixels

### ZenithSamplerEditor

**Sections:**
- Sample Map Table: Shows loaded samples with key/velocity ranges
- Envelope: ADSR controls
- Filter: Cutoff and resonance
- Global: Tune, gain, character
- Patch Selector: Load .zpatch files
- Preset Browser: Toggle-able preset browser panel

**Size:** 900x600 pixels

## Best Practices

### 1. Global Preset Manager

Create one global `ZenithPresetManager` instance in your main application:

```cpp
// In MainWindow or Application
class MainWindow
{
private:
    ZenithPresetManager presetManager_;  // Single instance
};
```

Pass it by reference to all editors.

### 2. Thread Safety

- Preset loading/saving happens on the message thread
- Parameter updates use atomic APVTS parameters (thread-safe)
- No crashes if presets are loaded during playback

### 3. Error Handling

```cpp
// Handle missing preset directory gracefully
if (presetManager.getPresetsForInstrument("zenith_poly_synth").empty())
{
    // Show message: "No presets installed"
    // Don't crash
}
```

### 4. Preset Versioning

The `ZenithInstrumentPreset` includes a `version` field. Future updates can handle version migrations:

```cpp
if (preset.version != "1.0.0")
{
    // Migrate preset to current version
    migratePreset(preset, "1.0.0");
}
```

## Example: Complete Integration

Here's a complete example of integrating everything:

```cpp
// In MainComponent.cpp

class MainComponent : public juce::Component
{
public:
    MainComponent(Engine& engine, ProjectState& state)
        : engine_(engine)
        , projectState_(state)
        , instrumentRegistry_()
        , presetManager_()  // Global preset manager
    {
        // Register instruments
        instrumentRegistry_.registerInstrument("zenith_poly_synth",
            []() { return std::make_unique<ZenithPolySynth>(); });

        instrumentRegistry_.registerInstrument("zenith_sampler",
            []() { return std::make_unique<ZenithSampler>(); });

        // Create instrument browser button
        addAndMakeVisible(openPolySynthButton_);
        openPolySynthButton_.onClick = [this] { openPolySynthEditor(); };
    }

private:
    void openPolySynthEditor()
    {
        // Get or create instrument instance
        auto* instrument = instrumentRegistry_.getInstance("zenith_poly_synth");
        auto* polySynth = dynamic_cast<ZenithPolySynth*>(instrument);

        // Create editor
        auto editor = std::make_unique<ZenithPolySynthEditor>(
            *polySynth,
            presetManager_);  // Pass global preset manager

        // Show in window
        auto* window = new juce::DocumentWindow(
            "Zenith PolySynth",
            juce::Colours::black,
            juce::DocumentWindow::allButtons);

        window->setContentOwned(editor.release(), true);
        window->setVisible(true);
        window->setUsingNativeTitleBar(true);
        window->centreWithSize(900, 700);

        editorWindows_.add(window);
    }

    Engine& engine_;
    ProjectState& projectState_;
    InstrumentRegistry instrumentRegistry_;
    ZenithPresetManager presetManager_;
    juce::OwnedArray<juce::DocumentWindow> editorWindows_;

    juce::TextButton openPolySynthButton_;
};
```

## Summary

- **Opening Editors**: Create editor instances with references to instrument and preset manager, show in DocumentWindow or PluginEditorWindow
- **Preset Management**: PresetBrowserComponent handles UI, ZenithPresetManager handles file I/O
- **CommandAPI**: Can query and load presets programmatically without UI
- **Thread Safety**: All operations are message-thread safe
- **Graceful Degradation**: Missing presets show friendly messages, no crashes

The system is designed to be:
1. **Modular**: Reusable components (PresetBrowserComponent)
2. **Extensible**: Easy to add new instruments
3. **DAW-Grade**: Production-ready UX with proper error handling
4. **AI-Friendly**: CommandAPI provides programmatic access to all preset operations
