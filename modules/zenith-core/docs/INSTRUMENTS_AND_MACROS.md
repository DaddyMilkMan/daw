# Built-In Instruments: Macro & Preset System

## Overview

Zenith DAW includes a comprehensive macro/modulation and preset system for built-in instruments. This system provides:

- **Smart Macros**: 4 high-level controls per instrument that map to multiple parameters
- **Preset Management**: Save and load instrument states including parameter values and macro settings
- **Data-Driven Schema**: Clean metadata API for AI and CommandAPI integration
- **Audio-Thread Safe**: All modulation calculations are precomputed and lock-free

## Architecture

### Components

1. **InstrumentMetadata**: Describes instrument parameters, macros, and their relationships
2. **MacroEngine**: Audio-thread-safe macro value storage and contribution calculation
3. **InstrumentRegistry**: Central registry for all built-in instruments
4. **PresetManager**: File-based preset storage and retrieval
5. **Built-in Instruments**: ZenithPolySynth and ZenithSampler

### File Structure

```
zenith-core/
├── include/instruments/
│   ├── InstrumentMetadata.h          # Metadata schema and MacroEngine
│   ├── InstrumentPreset.h            # Preset data structures and manager
│   ├── InstrumentRegistry.h          # Instrument registry and base class
│   ├── InstrumentRegistration.h      # Registration functions
│   ├── ZenithPolySynth.h            # Polyphonic synthesizer
│   ├── ZenithPolySynthEditor.h      # PolySynth UI with macro knobs
│   ├── ZenithSampler.h              # Sample playback instrument
│   └── ZenithSamplerEditor.h        # Sampler UI with macro knobs
│
└── src/instruments/
    ├── InstrumentRegistration.cpp    # Registers instruments and creates factory presets
    ├── ZenithPolySynth.cpp
    ├── ZenithPolySynthEditor.cpp
    ├── ZenithSampler.cpp
    └── ZenithSamplerEditor.cpp
```

## Built-In Instruments

### ZenithPolySynth

Simple polyphonic synthesizer with:
- Oscillator (Sine, Saw, Square, Triangle)
- Lowpass filter with resonance
- ADSR envelope
- 8-voice polyphony

**Smart Macros:**
1. **Warmth**: Controls filter cutoff and resonance for overall tone warmth
2. **Space**: Affects envelope release time for spatial characteristics
3. **Bite**: Increases filter resonance for edge and character
4. **Movement**: Adds detune and modulates attack time for animation

**Factory Presets:**
- Warm Pad
- Plucky Lead
- Deep Bass

### ZenithSampler

Sample playback instrument with:
- Audio file loading (WAV, AIFF, MP3, OGG, FLAC)
- Sample start/end controls
- Filter with resonance
- ADSR envelope

**Smart Macros:**
1. **Body**: Controls filter for tone fullness and body
2. **Snap**: Adjusts attack time and brightness for transient emphasis
3. **LoFi**: Simulates lo-fi character via filter darkening
4. **Tone**: Overall tonal balance from dark to bright

**Factory Presets:**
- Natural (clean playback)
- Punchy (tight attack)
- LoFi Vinyl (warm vintage character)

## Usage

### Initializing the System

In your application startup code:

```cpp
#include "instruments/InstrumentRegistration.h"

void Application::initialise()
{
    // Register all built-in instruments
    zenith::registerAllInstruments();

    // Optional: Create factory presets (first run only)
    // zenith::createFactoryPresets();
}
```

### Creating Instrument Instances

```cpp
#include "instruments/InstrumentRegistry.h"

auto& registry = zenith::InstrumentRegistry::getInstance();

// Create a PolySynth instance
auto polySynth = registry.createInstrument("zenith_poly_synth");

// Create a Sampler instance
auto sampler = registry.createInstrument("zenith_sampler");
```

### Accessing Metadata

```cpp
auto& registry = zenith::InstrumentRegistry::getInstance();

// Get metadata for an instrument
const auto* metadata = registry.getMetadata("zenith_poly_synth");

// Iterate through parameters
for (const auto& param : metadata->parameters)
{
    std::cout << "Parameter: " << param.name
              << " (ID: " << param.id << ")\n";
}

// Iterate through macros
for (const auto& macro : metadata->macros)
{
    std::cout << "Macro: " << macro.name
              << " - " << macro.description << "\n";

    // Show macro targets
    for (const auto& target : macro.targets)
    {
        std::cout << "  -> " << target.parameterId
                  << " (amount: " << target.amount << ")\n";
    }
}
```

### Working with Presets

```cpp
// Get all presets for an instrument
auto presets = registry.getPresetsForInstrument("zenith_poly_synth");

// Load a preset
if (!presets.empty())
{
    auto* instrument = dynamic_cast<zenith::ZenithPolySynth*>(polySynth.get());
    if (instrument)
    {
        instrument->loadPreset(presets[0]);
    }
}

// Save current state as preset
auto currentPreset = instrument->getCurrentPreset();
currentPreset.name = "My Custom Sound";
currentPreset.author = "User";
currentPreset.tags = {"custom", "lead"};

auto& presetManager = registry.getPresetManager();
presetManager.saveUserPreset(currentPreset);
```

### Using Macros

Macros are exposed as parameters in the `AudioProcessorValueTreeState`:

```cpp
auto* instrument = dynamic_cast<zenith::ZenithPolySynth*>(polySynth.get());

// Set macro value (0.0 to 1.0, centered at 0.5)
auto& params = instrument->getParameters();
auto* macro0 = params.getParameter("macro_0");  // Warmth
macro0->setValueNotifyingHost(0.7f);  // Increase warmth

// Macros automatically apply their contributions to target parameters
// No additional code needed - happens in processBlock()
```

### Preset File Format

Presets are stored as XML files with `.zpreset` extension:

```
~/Library/Application Support/Zenith/Instruments/
├── Factory/
│   ├── zenith_poly_synth/
│   │   ├── Warm Pad.zpreset
│   │   ├── Plucky Lead.zpreset
│   │   └── Deep Bass.zpreset
│   └── zenith_sampler/
│       ├── Natural.zpreset
│       ├── Punchy.zpreset
│       └── LoFi Vinyl.zpreset
└── User/
    ├── zenith_poly_synth/
    │   └── My Custom Sound.zpreset
    └── zenith_sampler/
        └── My Sample Settings.zpreset
```

## Technical Details

### Macro Application

Macros work by computing **contributions** to parameters based on macro values:

1. Each macro has a value [0..1], centered at 0.5
2. Macro value is converted to [-1..+1] range: `normalizedMacro = (macroValue - 0.5) * 2.0`
3. Contribution to each target parameter: `contribution = normalizedMacro * amount * parameterRange`
4. Final parameter value: `effectiveValue = baseValue + contribution`

Example:
- Warmth macro = 0.7 (70%)
- Normalized: (0.7 - 0.5) * 2.0 = 0.4
- Filter cutoff range: 0.0 to 1.0
- Target amount: 0.5
- Contribution: 0.4 * 0.5 * 1.0 = 0.2
- If base cutoff = 0.6, effective cutoff = 0.6 + 0.2 = 0.8

### Audio Thread Safety

The macro engine is designed for real-time audio processing:

- **No allocations**: All storage pre-allocated during initialization
- **No locks**: Uses simple atomic operations and pre-computed values
- **Cache-friendly**: Linear memory layout for fast iteration
- **Minimal branching**: Straightforward calculation path

### Extending the System

To add a new instrument:

1. Create a class inheriting from `ZenithInstrumentProcessor`
2. Implement `getInstrumentMetadata()` to return metadata
3. Implement `loadPreset()` and `getCurrentPreset()`
4. Implement `MacroEngine` integration in `processBlock()`
5. Register in `InstrumentRegistration.cpp`

Example metadata definition:

```cpp
InstrumentMetadata MyInstrument::createMetadata()
{
    InstrumentMetadata metadata("my_instrument", "My Instrument");

    // Add parameters
    metadata.addParameter(InstrumentParameterInfo(
        "param1", "Parameter 1", ParameterType::Float,
        ParameterCategory::Global, 0.0f, 1.0f, 0.5f
    ));

    // Define macro
    InstrumentMacroInfo macro("macro_custom", "Custom", "My custom macro");
    macro.addTarget("param1", 0.8f);
    metadata.addMacro(macro);

    return metadata;
}
```

## Future Enhancements

This system is designed to be extended with:

1. **Full Modulation Matrix**: Add LFOs, envelopes, and custom modulation sources
2. **CommandAPI Integration**: Expose presets and macros via JSON API for AI control
3. **Preset Browser UI**: Visual preset browser with search and tags
4. **Preset Morphing**: Interpolate between presets in real-time
5. **User Macro Creation**: Allow users to create custom macro mappings
6. **MPE Support**: Per-note modulation via MIDI Polyphonic Expression

## Testing

To test the system:

1. Build the project: `cmake --build zenith-core/build`
2. Run ZenithDAW
3. Call `registerAllInstruments()` from your initialization code
4. Create instrument instances and test:
   - Parameter changes
   - Macro controls
   - Preset loading/saving
   - Audio output

## Support

For questions or issues with the instrument/macro system:
- Check the inline documentation in header files
- Review the factory preset creation code for examples
- Examine `InstrumentRegistration.cpp` for usage patterns

---

**Zenith DAW** - The Perfect DAW with AI Integration
Built-in Instruments and Macro System - v1.0.0
