# Zenith PolySynth - Additional Issues Found

**Analysis Date:** 2025-11-27  
**Status:** 🟡 **8 Additional Issues Identified**

---

## 🔴 **CRITICAL ISSUES** (1)

### Issue #10: Pitch Wheel Not Implemented ❌
**Severity:** 🔴 CRITICAL  
**Location:** `ZenithPolySynth.cpp` line 321-323

**Problem:**
```cpp
void ZenithPolySynthVoice::pitchWheelMoved(int /*newPitchWheelValue*/) {
  // Handle pitch wheel if needed
}
```

The pitch wheel callback is empty! This means:
- Pitch bend doesn't work at all
- Standard MIDI pitch wheel messages are ignored
- Users can't use pitch bend for expression

**Impact:** Major usability issue - pitch wheel is a fundamental MIDI feature.

**Fix Required:**
```cpp
void ZenithPolySynthVoice::pitchWheelMoved(int newPitchWheelValue) {
  // Convert MIDI pitch wheel (0-16383, center=8192) to semitones
  // Typical range is ±2 semitones
  float pitchBendSemitones = ((newPitchWheelValue - 8192.0f) / 8192.0f) * 2.0f;
  
  // Apply to target frequency
  float pitchBendMultiplier = std::pow(2.0f, pitchBendSemitones / 12.0f);
  targetFrequency_ = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote_) * pitchBendMultiplier;
}
```

**Additional Required:**
- Add `int currentMidiNote_` member variable to store the MIDI note number
- Add `float pitchBendRange_` parameter (default 2.0 semitones)

---

## ⚠️ **MODERATE ISSUES** (4)

### Issue #11: LFO Target System Deprecated But Still Used ⚠️
**Severity:** ⚠️ MODERATE  
**Location:** `ZenithPolySynth.cpp` lines 726-729, `ZenithPolySynth.h` line 59

**Problem:**
The `LFOTarget` enum is marked as "legacy - now part of modulation matrix" but is still being used:

```cpp
voice->setLFO1(getVal(LFO1Rate), getVal(LFO1Amount),
               (LFOTarget)getChoice(LFO1Target));  // ❌ Using deprecated system
voice->setLFO2(getVal(LFO2Rate), getVal(LFO2Amount),
               (LFOTarget)getChoice(LFO2Target));  // ❌ Using deprecated system
```

**Impact:**
- Confusing dual routing system (LFOTarget vs ModulationMatrix)
- LFO routing might not work correctly
- Code maintenance burden

**Fix Options:**
1. **Remove LFOTarget entirely** and use only modulation matrix
2. **Keep both** but clarify their relationship
3. **Migrate LFOTarget to ModulationMatrix** on parameter change

**Recommended Fix:**
Remove `LFOTarget` and use modulation matrix exclusively. Update `setLFO1/setLFO2` to only set rate/amount.

---

### Issue #12: startNote() Doesn't Store MIDI Note Number ⚠️
**Severity:** ⚠️ MODERATE  
**Location:** `ZenithPolySynth.cpp` lines 281-308

**Problem:**
```cpp
void ZenithPolySynthVoice::startNote(int midiNoteNumber, float velocity, ...) {
  currentFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
  targetFrequency_ = currentFrequency_;
  // ❌ midiNoteNumber is NOT stored!
}
```

**Impact:**
- Can't implement pitch wheel (needs original note number)
- Can't implement note-based modulation
- Can't implement MPE (MIDI Polyphonic Expression)

**Fix Required:**
```cpp
// Add to header
int currentMidiNote_ = -1;

// In startNote()
currentMidiNote_ = midiNoteNumber;
currentFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
targetFrequency_ = currentFrequency_;
```

---

### Issue #13: No Filter2 Parameter Controls ⚠️
**Severity:** ⚠️ MODERATE  
**Location:** `ZenithPolySynth.cpp` lines 716-719, parameter layout

**Problem:**
Filter2 is used in the audio path (dual routing) but has NO exposed parameters:

```cpp
// Only Filter1 parameters exist:
voice->setFilterType((zenith::FilterType)getChoice(FilterType));
voice->setFilterCutoff(getVal(FilterCutoff));
voice->setFilterResonance(getVal(FilterResonance));
voice->setFilterDrive(getVal(FilterDrive));

// ❌ No Filter2 parameters!
// ❌ No filterSerial_ parameter!
```

**Impact:**
- Filter2 always uses default settings (1000Hz lowpass)
- Can't configure dual filter routing
- Dual filter feature is essentially broken

**Fix Required:**
Add to `createParameterLayout()`:
```cpp
// Filter 2
layout.add(std::make_unique<juce::AudioParameterChoice>(
    "filter2_type", "Filter 2 Type", filterTypes, 0));
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "filter2_cutoff", "Filter 2 Cutoff", 20.0f, 20000.0f, 1000.0f));
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "filter2_resonance", "Filter 2 Resonance", 0.0f, 1.0f, 0.0f));
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "filter2_drive", "Filter 2 Drive", 1.0f, 5.0f, 1.0f));
layout.add(std::make_unique<juce::AudioParameterBool>(
    "filter_serial", "Filter Serial", true));
```

---

### Issue #14: Effects Parameters Not Exposed ⚠️
**Severity:** ⚠️ MODERATE  
**Location:** Effects are used but not controllable

**Problem:**
The `ZenithEffects` class has distortion and chorus, but they're hardcoded:

```cpp
// In ZenithEffects::process()
if (distortionAmount_ > 0.01f) { ... }  // ❌ distortionAmount_ is never set!
if (chorusAmount_ > 0.01f) { ... }      // ❌ chorusAmount_ is never set!
```

**Impact:**
- Distortion and chorus effects are always OFF (amount = 0.0)
- Users can't enable or control effects
- Presets that set `distortion` and `chorus` don't work

**Fix Required:**
1. Add parameters to layout:
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "distortion", "Distortion", 0.0f, 1.0f, 0.0f));
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "chorus", "Chorus", 0.0f, 1.0f, 0.0f));
```

2. Add setters to voice:
```cpp
void setDistortion(float amount) { effects_.setDistortion(amount); }
void setChorus(float amount) { effects_.setChorus(amount); }
```

3. Wire up in `updateVoiceParameters()`:
```cpp
voice->setDistortion(getVal("distortion"));
voice->setChorus(getVal("chorus"));
```

---

## 🟡 **MINOR ISSUES** (3)

### Issue #15: Unison Oscillators Always Use currentFrequency_ 🟡
**Severity:** 🟡 MINOR  
**Location:** `ZenithPolySynth.cpp` line 427

**Problem:**
```cpp
unisonOutput += unisonOscillators_[v].getNextSample(currentFrequency_, oscShape_);
// ❌ Should use osc1Freq with pitch modulation!
```

**Impact:**
- Unison voices don't respond to pitch modulation
- Vibrato/pitch LFO doesn't affect unison
- Inconsistent with main oscillators

**Fix:**
```cpp
unisonOutput += unisonOscillators_[v].getNextSample(osc1Freq, oscShape_);
```

---

### Issue #16: No Velocity Sensitivity Control 🟡
**Severity:** 🟡 MINOR  
**Location:** Velocity is used but not configurable

**Problem:**
Velocity is always applied at 100%:
```cpp
filtered *= ampEnv * velocity_;  // ❌ No velocity curve or amount control
```

**Impact:**
- Can't adjust velocity sensitivity
- Can't create non-velocity-sensitive patches
- Less expressive control

**Fix:**
Add velocity amount parameter (0.0 = no velocity, 1.0 = full velocity):
```cpp
float velocityAmount = 1.0f; // From parameter
float effectiveVelocity = 1.0f - velocityAmount + (velocity_ * velocityAmount);
filtered *= ampEnv * effectiveVelocity;
```

---

### Issue #17: Filter Drive Not Applied to Filter2 🟡
**Severity:** 🟡 MINOR  
**Location:** `ZenithPolySynth.cpp` lines 444-455

**Problem:**
```cpp
if (filterSerial_) {
  float temp = filter1_.processSample(mixed);
  filtered = filter2_.processSample(temp);  // ❌ Filter2 has no drive control
}
```

**Impact:**
- Filter2 can't add saturation/warmth
- Asymmetric filter behavior

**Fix:**
Add `setFilter2Drive()` and wire it up like Filter1.

---

### Issue #18: Chorus Phase Not Reset on Note Start 🟡
**Severity:** 🟡 MINOR  
**Location:** `ZenithPolySynth.cpp` line 307, `ZenithEffects`

**Problem:**
```cpp
void ZenithPolySynthVoice::startNote(...) {
  // ...
  effects_.reset();  // ✅ Calls reset()
}

void ZenithEffects::reset() {
  delayPos_ = 0;
  delayBufferL_.fill(0.0f);
  delayBufferR_.fill(0.0f);
  // ❌ chorusPhase_ is NOT reset!
}
```

**Impact:**
- Chorus LFO phase is random on each note
- Less predictable/consistent sound
- Minor issue but affects reproducibility

**Fix:**
```cpp
void ZenithEffects::reset() {
  delayPos_ = 0;
  delayBufferL_.fill(0.0f);
  delayBufferR_.fill(0.0f);
  chorusPhase_ = 0.0f;  // Add this
}
```

---

## 📊 **SUMMARY**

| Issue # | Name | Severity | Impact | Fix Complexity |
|---------|------|----------|--------|----------------|
| 10 | Pitch wheel not implemented | 🔴 CRITICAL | No pitch bend | Medium |
| 11 | LFOTarget deprecated but used | ⚠️ MODERATE | Confusing routing | Medium |
| 12 | MIDI note not stored | ⚠️ MODERATE | Blocks pitch wheel | Easy |
| 13 | No Filter2 parameters | ⚠️ MODERATE | Filter2 unusable | Medium |
| 14 | Effects not exposed | ⚠️ MODERATE | Effects always off | Medium |
| 15 | Unison ignores pitch mod | 🟡 MINOR | Inconsistent modulation | Easy |
| 16 | No velocity sensitivity | 🟡 MINOR | Less expressive | Easy |
| 17 | Filter2 no drive | 🟡 MINOR | Asymmetric filters | Easy |
| 18 | Chorus phase not reset | 🟡 MINOR | Random chorus phase | Easy |

---

## 🎯 **PRIORITY FIX ORDER**

### **High Priority** (Should fix now)
1. **Issue #10**: Implement pitch wheel (critical MIDI feature)
2. **Issue #12**: Store MIDI note number (required for #10)
3. **Issue #14**: Expose effects parameters (presets need this)
4. **Issue #13**: Add Filter2 parameters (feature is broken)

### **Medium Priority** (Fix soon)
5. **Issue #11**: Clean up LFOTarget vs ModulationMatrix
6. **Issue #15**: Fix unison pitch modulation

### **Low Priority** (Nice to have)
7. **Issue #16**: Add velocity sensitivity control
8. **Issue #17**: Add Filter2 drive
9. **Issue #18**: Reset chorus phase

---

## 🔧 **ESTIMATED FIX TIME**

- **Critical fixes (10, 12)**: 30 minutes
- **Moderate fixes (13, 14)**: 45 minutes
- **LFO cleanup (11)**: 30 minutes
- **Minor fixes (15-18)**: 20 minutes

**Total**: ~2 hours to fix all issues

---

**Current Status:** Synth is functional but missing key features (pitch wheel, effects, filter2 control)
