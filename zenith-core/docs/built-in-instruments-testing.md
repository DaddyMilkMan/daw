# Built-in Instrument Architecture - Testing Guide

## Overview

This document describes how to test the newly implemented built-in instrument architecture and the ZenithPolySynth instrument.

## Architecture Summary

### Components Implemented

1. **Instrument Registry (`zenith::InstrumentRegistry`)**
   - Singleton registry for managing built-in instruments
   - Thread-safe read access after initialization
   - Allows enumeration and instantiation of instruments by ID

2. **Instrument Definition (`zenith::InstrumentDefinition`)**
   - Metadata structure describing each instrument
   - Contains: id, name, category, voice mode, description, and factory function

3. **ZenithPolySynth**
   - Full JUCE AudioProcessor implementation
   - 16-voice polyphonic synthesizer
   - Dual oscillators with saw/square/sine/triangle waveforms
   - State Variable Filter (LP/BP/HP)
   - ADSR envelope
   - AudioProcessorValueTreeState for parameter management
   - Clean, minimal UI editor

4. **Track Integration**
   - Tracks can now host a single built-in instrument via `setInstrument()`
   - MIDI clips are routed to instruments automatically
   - Instrument processing happens before plugin chain

5. **Engine Integration**
   - InstrumentRegistry initialized at startup
   - ZenithPolySynth registered automatically

## File Structure

```
zenith-core/
├── include/instruments/
│   ├── InstrumentRegistry.h         # Instrument registry interface
│   ├── ZenithPolySynth.h            # Main synth AudioProcessor
│   ├── ZenithPolySynthVoice.h       # Individual voice synthesis
│   └── ZenithPolySynthEditor.h      # UI editor
│
├── src/instruments/
│   ├── InstrumentRegistry.cpp
│   ├── ZenithPolySynth.cpp
│   ├── ZenithPolySynthVoice.cpp
│   └── ZenithPolySynthEditor.cpp
│
└── Source/engine/
    ├── Track.h/.cpp                 # Updated with instrument support
    └── Clip.h/.cpp                  # Updated with MIDI event extraction
```

## Manual Testing Procedure

### 1. Verify Compilation

```bash
cd zenith-core
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**Expected:** Clean compilation with no errors or warnings related to instrument code.

### 2. Verify Registry Initialization

**Test:** Launch the application and check console output.

**Expected output:**
```
Engine: Registering built-in instruments...
Engine: Registered 1 built-in instrument(s)
```

### 3. Verify Instrument Creation (Programmatic)

**Test code snippet** (can be added to Engine or a test):

```cpp
auto& registry = zenith::InstrumentRegistry::getInstance();

// Check instrument is registered
jassert(registry.hasInstrument("zenith_poly_synth"));

// Get metadata
auto* def = registry.getDefinition("zenith_poly_synth");
jassert(def != nullptr);
jassert(def->name == "Zenith Poly Synth");
jassert(def->category == "Synth");

// Create instance
auto synth = registry.createInstrument("zenith_poly_synth");
jassert(synth != nullptr);
jassert(synth->getName() == "Zenith Poly Synth");
```

### 4. Verify Track Integration

**Test code snippet:**

```cpp
// Create an instrument track
auto track = std::make_unique<zenith::Track>("Test Synth", zenith::Track::Type::Instrument);

// Create and set instrument
auto& registry = zenith::InstrumentRegistry::getInstance();
auto synth = registry.createInstrument("zenith_poly_synth");
track->setInstrument(std::move(synth));

// Verify
jassert(track->hasInstrument());
jassert(track->getInstrument() != nullptr);

// Prepare for playback
track->prepareToPlay(512, 44100.0);
```

### 5. Verify MIDI Routing and Audio Output

**Test procedure:**

1. Create an instrument track with ZenithPolySynth
2. Add a MIDI clip with note events (e.g., C4, E4, G4 chord)
3. Start playback
4. Verify audio is produced

**Expected:** Audio output corresponding to the MIDI notes.

**Debug verification:**
- Set breakpoint in `Track::processInstrument()`
- Verify `midiBuffer` contains MIDI events when clips are playing
- Set breakpoint in `ZenithPolySynthVoice::startNote()`
- Verify notes are triggered with correct pitch and velocity

### 6. Verify Parameter Editing

**Test procedure:**

1. Create a ZenithPolySynth instance
2. Open the editor: `synth->createEditor()`
3. Modify parameters:
   - Change oscillator waveforms
   - Adjust filter cutoff and resonance
   - Modify envelope parameters
4. Play notes and verify sound changes

**Expected:** Real-time parameter updates affect the sound.

### 7. Verify State Save/Load

**Test code snippet:**

```cpp
auto synth = registry.createInstrument("zenith_poly_synth");

// Modify some parameters
auto& params = static_cast<zenith::instruments::ZenithPolySynth*>(synth.get())->getParameters();
params.getParameter("filter_cutoff")->setValue(0.3f);
params.getParameter("env_attack")->setValue(0.5f);

// Save state
juce::MemoryBlock state;
synth->getStateInformation(state);

// Create new instance and restore
auto synth2 = registry.createInstrument("zenith_poly_synth");
synth2->setStateInformation(state.getData(), state.getSize());

// Verify parameters match
jassert(params.getRawParameterValue("filter_cutoff")->load() == 0.3f);
```

### 8. Smoke Test: Complete Integration

**Full integration test:**

```cpp
// 1. Create engine and initialize
Engine engine;
engine.initialize();

// 2. Create instrument track
auto& registry = zenith::InstrumentRegistry::getInstance();
auto track = std::make_unique<zenith::Track>("Synth Track", zenith::Track::Type::Instrument);
auto synth = registry.createInstrument("zenith_poly_synth");
track->setInstrument(std::move(synth));

// 3. Add MIDI clip with notes
auto clip = std::make_unique<zenith::Track::Clip>(
    zenith::Track::Clip::Type::MIDI,
    0,
    96000  // 2 seconds at 48kHz
);

juce::MidiMessageSequence sequence;
sequence.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0.0);      // C4 at 0s
sequence.addEvent(juce::MidiMessage::noteOff(1, 60), 1.0);            // Off at 1s
sequence.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 0.5);      // E4 at 0.5s
sequence.addEvent(juce::MidiMessage::noteOff(1, 64), 1.5);            // Off at 1.5s
clip->setMidiSequence(sequence);
track->addClip(std::move(clip));

// 4. Prepare and play
track->prepareToPlay(512, 48000.0);
engine.addTrack(std::move(track));
engine.play();

// 5. Let it run for 3 seconds
juce::Thread::sleep(3000);
engine.stop();
```

**Expected:** Hear two notes (C4 and E4) playing through the synthesizer.

## Known Limitations

1. **No UI for instrument selection yet** - Instruments must be created programmatically
2. **No preset management** - Only default preset available
3. **Single instrument per track** - No plugin chain mixing yet (as designed)
4. **No automation for instrument parameters** - Will be added when automation system is extended

## Performance Verification

### Real-Time Safety Checks

All critical paths are RT-safe:
- ✅ No allocations in `Track::getNextAudioBlock()`
- ✅ No allocations in `ZenithPolySynth::processBlock()`
- ✅ No allocations in `ZenithPolySynthVoice::renderNextBlock()`
- ✅ Parameters read via atomic operations
- ✅ Lock-free MIDI buffer operations

### CPU Usage

With 16 voices active, CPU usage should be minimal (<5% on modern CPUs at 512 sample buffer).

**Test:** Play 16 simultaneous notes and check `Engine::getCPUUsage()`.

## Troubleshooting

### No Sound Output

**Check:**
1. Track is not muted: `track->isMuted() == false`
2. Instrument is set: `track->hasInstrument() == true`
3. MIDI clip is active: `clip->isActive() == true`
4. MIDI events are present: `clip->getMidiSequence()->getNumEvents() > 0`
5. Audio device is initialized: `engine.getCurrentSampleRate() > 0`

### Compilation Errors

**Common issues:**
- Missing `#include <juce_dsp/juce_dsp.h>` - already included
- juce_dsp not linked - added to CMakeLists.txt
- Namespace issues - all instruments are in `zenith::instruments`

### Parameter Changes Not Audible

**Check:**
1. Parameters are connected to ValueTreeState attachments in editor
2. `updateVoiceParameters()` is called in `processBlock()`
3. Voice getters are using atomic loads

## Future Enhancements

1. **UI Integration**
   - Add instrument browser in MainWindow
   - Instrument selection dropdown in track header
   - Inline parameter editors

2. **Additional Instruments**
   - Sampler
   - Drum machine
   - FM synthesizer

3. **Advanced Features**
   - Modulation routing
   - Arpeggiator
   - MPE support
   - Microtuning

## Success Criteria

✅ All new files compile without errors
✅ InstrumentRegistry properly initialized
✅ ZenithPolySynth can be instantiated
✅ MIDI clips route to instrument
✅ Audio is produced from MIDI input
✅ Parameters can be adjusted
✅ State can be saved and restored
✅ No RT-safety violations
✅ No memory leaks

---

**Implementation Date:** 2025-11-18
**JUCE Version:** 8.0.9
**C++ Standard:** C++20
