# ZenithPolySynth Parameter Schema

**Created:** 2025-11-29  
**Author:** Alex Chen (Synthesist)  
**Purpose:** Complete parameter reference for AI preset generation

---

## Parameter Categories

### 1. Oscillators (3x)

Each oscillator has identical parameters:

```json
{
  "osc1_waveform": {
    "type": "choice",
    "values": ["sine", "saw", "square", "triangle", "noise", "supersaw"],
    "default": "saw",
    "description": "Oscillator waveform type"
  },
  "osc1_detune": {
    "type": "float",
    "range": [-100.0, 100.0],
    "default": 0.0,
    "unit": "cents",
    "description": "Pitch detune in cents"
  },
  "osc1_mix": {
    "type": "float",
    "range": [0.0, 1.0],
    "default": 1.0,
    "description": "Oscillator mix level"
  }
}
```

**Parameters:** `osc1_*`, `osc2_*`, `osc3_*`

---

### 2. Unison

```json
{
  "unison_voices": {
    "type": "int",
    "range": [1, 7],
    "default": 1,
    "description": "Number of unison voices per note"
  },
  "unison_detune": {
    "type": "float",
    "range": [0.0, 100.0],
    "default": 0.0,
    "unit": "cents",
    "description": "Unison voice detune spread"
  }
}
```

---

### 3. Filters (2x)

```json
{
  "filter_type": {
    "type": "choice",
    "values": ["lowpass", "bandpass", "highpass"],
    "default": "lowpass",
    "description": "Filter type"
  },
  "filter_cutoff": {
    "type": "float",
    "range": [20.0, 20000.0],
    "default": 1000.0,
    "unit": "Hz",
    "description": "Filter cutoff frequency",
    "scale": "logarithmic"
  },
  "filter_resonance": {
    "type": "float",
    "range": [0.0, 1.0],
    "default": 0.0,
    "description": "Filter resonance/Q"
  },
  "filter_drive": {
    "type": "float",
    "range": [0.0, 10.0],
    "default": 1.0,
    "description": "Filter drive/saturation"
  }
}
```

**Parameters:** `filter_*` (filter 1), `filter2_*` (filter 2)

**Filter Routing:**
```json
{
  "filter_routing": {
    "type": "choice",
    "values": ["serial", "parallel"],
    "default": "serial",
    "description": "Filter 1 → Filter 2 (serial) or both in parallel"
  }
}
```

---

### 4. Envelopes (2x ADSR)

```json
{
  "amp_attack": {
    "type": "float",
    "range": [0.001, 5.0],
    "default": 0.01,
    "unit": "seconds",
    "description": "Amplitude envelope attack time"
  },
  "amp_decay": {
    "type": "float",
    "range": [0.001, 5.0],
    "default": 0.1,
    "unit": "seconds"
  },
  "amp_sustain": {
    "type": "float",
    "range": [0.0, 1.0],
    "default": 0.8,
    "description": "Amplitude envelope sustain level"
  },
  "amp_release": {
    "type": "float",
    "range": [0.001, 10.0],
    "default": 0.5,
    "unit": "seconds"
  }
}
```

**Parameters:** `amp_*` (amplitude envelope), `mod_*` (modulation envelope)

---

### 5. LFOs (2x)

```json
{
  "lfo1_rate": {
    "type": "float",
    "range": [0.1, 20.0],
    "default": 1.0,
    "unit": "Hz",
    "description": "LFO frequency"
  },
  "lfo1_amount": {
    "type": "float",
    "range": [0.0, 1.0],
    "default": 0.0,
    "description": "LFO modulation depth"
  },
  "lfo1_target": {
    "type": "choice",
    "values": ["filter_cutoff", "osc1_pitch", "osc2_pitch", "osc1_mix", "osc2_mix"],
    "default": "filter_cutoff",
    "description": "LFO modulation target"
  }
}
```

**Parameters:** `lfo1_*`, `lfo2_*`

---

### 6. Modulation Matrix

Up to 8 modulation slots:

```json
{
  "mod_slot_0": {
    "source": {
      "type": "choice",
      "values": ["none", "lfo1", "lfo2", "env1", "env2", "velocity", "modwheel", "aftertouch"],
      "default": "none"
    },
    "destination": {
      "type": "choice",
      "values": ["none", "filter_cutoff", "filter_resonance", "osc1_pitch", "osc2_pitch", 
                 "osc3_pitch", "wavetable_pos", "pan", "volume", "osc1_mix", "osc2_mix", 
                 "osc3_mix", "osc_shape"],
      "default": "none"
    },
    "amount": {
      "type": "float",
      "range": [-1.0, 1.0],
      "default": 0.0,
      "description": "Modulation depth"
    }
  }
}
```

**Slots:** `mod_slot_0` through `mod_slot_7`

---

### 7. Effects

```json
{
  "distortion": {
    "type": "float",
    "range": [0.0, 1.0],
    "default": 0.0,
    "description": "Distortion amount"
  },
  "chorus": {
    "type": "float",
    "range": [0.0, 1.0],
    "default": 0.0,
    "description": "Chorus effect amount"
  }
}
```

---

### 8. Global Settings

```json
{
  "glide_time": {
    "type": "float",
    "range": [0.0, 2.0],
    "default": 0.0,
    "unit": "seconds",
    "description": "Portamento/glide time"
  },
  "mono_mode": {
    "type": "bool",
    "default": false,
    "description": "Monophonic mode (legato)"
  },
  "master_gain": {
    "type": "float",
    "range": [0.0, 2.0],
    "default": 0.7,
    "description": "Master output gain"
  },
  "quality_setting": {
    "type": "choice",
    "values": ["low", "medium", "high"],
    "default": "medium",
    "description": "CPU quality preset"
  }
}
```

---

## Preset Generation Guidelines

### Sound Type Templates

#### **Pad**
- Slow attack (0.5-2.0s)
- Long release (1.0-5.0s)
- Multiple oscillators with slight detune
- Low-pass filter (500-1500 Hz)
- Low resonance
- Optional chorus

#### **Bass**
- Fast attack (0.001-0.01s)
- Short release (0.1-0.5s)
- Saw or square wave
- Low-pass filter (100-800 Hz)
- Medium resonance (0.3-0.6)
- Optional filter drive

#### **Lead**
- Medium attack (0.01-0.1s)
- Medium release (0.2-1.0s)
- Saw or square wave
- Band-pass or high-pass filter
- LFO on filter cutoff for movement

#### **Pluck**
- Very fast attack (0.001-0.005s)
- Fast decay (0.05-0.2s)
- Low sustain (0.0-0.3)
- Short release (0.05-0.2s)
- High-pass filter

#### **Arp/Sequence**
- Fast attack
- Short release
- Moderate filter cutoff
- Optional LFO on pitch or filter

---

## Parameter Count

**Total Parameters:** ~60-70 depending on modulation matrix usage

**Core Parameters:** 45
**Modulation Slots:** 8 × 3 = 24 (optional)

---

## Validation Rules

1. **Envelope times:** Attack + Decay should be reasonable (<10s total)
2. **Filter cutoff:** Should match sound type (bass = low, lead = mid-high)
3. **Oscillator mix:** Total mix should not exceed ~2.0 to avoid clipping
4. **Resonance:** High resonance (>0.8) requires careful filter cutoff
5. **Unison:** More voices = more CPU, limit to 3-5 for most presets

---

## Example Preset: "Warm Pad"

```json
{
  "name": "Warm Pad",
  "description": "Lush, warm pad with slow attack and gentle movement",
  "category": "pad",
  "parameters": {
    "osc1_waveform": "saw",
    "osc1_detune": 0.0,
    "osc1_mix": 0.7,
    "osc2_waveform": "sine",
    "osc2_detune": 7.0,
    "osc2_mix": 0.3,
    "osc3_mix": 0.0,
    "filter_type": "lowpass",
    "filter_cutoff": 800.0,
    "filter_resonance": 0.3,
    "amp_attack": 1.5,
    "amp_decay": 0.5,
    "amp_sustain": 0.8,
    "amp_release": 2.0,
    "lfo1_rate": 0.3,
    "lfo1_amount": 0.2,
    "lfo1_target": "filter_cutoff",
    "chorus": 0.3
  }
}
```

---

**This schema is ready for Grok AI to generate presets!**
