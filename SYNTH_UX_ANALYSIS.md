# Zenith PolySynth - User Experience Analysis

**Analysis Date:** 2025-11-27  
**Perspective:** Real-world musician/producer usage

---

## 🎹 **AS A USER: WHAT WORKS WELL**

### ✅ **Strengths**

1. **Solid Foundation**
   - 3 oscillators with good waveform selection (Sine, Saw, Square, Triangle, Noise, Supersaw)
   - Dual filters with serial/parallel routing
   - Full ADSR envelopes (amp + mod)
   - Working effects (distortion, chorus)
   - Pitch wheel support
   - Modulation matrix (8 slots)

2. **Performance Features**
   - Quality presets (Low/Medium/High) for CPU optimization
   - Up to 7 unison voices for thick sounds
   - PolyBLEP antialiasing (clean oscillators)
   - RT-safe audio processing

3. **Modern Features**
   - Modulation matrix (flexible routing)
   - Dual filter routing
   - Per-voice effects

---

## ❌ **CRITICAL UX ISSUES**

### 1. **LFO System is Confusing** 🔴
**Problem:** Dual routing system (LFOTarget vs ModulationMatrix)

**User Experience:**
```
User: "I want LFO1 to modulate filter cutoff"
Current: Set LFO1Target to "FilterCutoff" OR use modulation matrix slot
Result: CONFUSING! Which one do I use? Do both work? Do they stack?
```

**Expected:** One clear system. Either:
- Use ONLY modulation matrix (modern approach)
- OR make LFOTarget auto-populate modulation matrix

**Impact:** Users will be confused about how to route LFOs

---

### 2. **No LFO Waveform Selection** 🔴
**Problem:** LFOs are hardcoded to sine waves only

**User Experience:**
```cpp
// In computeModulation():
lfo1Value_ = std::sin(lfo1Phase_ * ...);  // ❌ ALWAYS SINE!
lfo2Value_ = std::sin(lfo2Phase_ * ...);  // ❌ ALWAYS SINE!
```

**What Users Expect:**
- Sine (smooth vibrato)
- Triangle (linear sweep)
- Square (stepped/gated effects)
- Saw Up/Down (ramp effects)
- Sample & Hold (random stepped)

**Impact:** Can't create common effects like:
- Square wave tremolo (gated volume)
- Sample & Hold filter (random filter jumps)
- Saw LFO (ramp up/down effects)

**Competitors Have:** Serum, Vital, Massive X all have LFO waveforms

---

### 3. **No Filter Envelope Amount** 🔴
**Problem:** Can't control how much Env2 affects filter

**User Experience:**
```
User: "I want a subtle filter sweep"
Current: Must use modulation matrix, set amount manually
Expected: Simple "Filter Env Amount" knob (like every other synth)
```

**What's Missing:**
```cpp
// Should have:
float filterEnvAmount_ = 0.5f;  // 0 = no envelope, 1 = full envelope
float envMod = modEnvelope_.getNextSample() * filterEnvAmount_;
float cutoff = baseCutoff + (envMod * 10000.0f);
```

**Impact:** Filter sweeps are harder to set up than competitors

---

### 4. **Oscillator Sync Missing** 🔴
**Problem:** No hard sync or FM between oscillators

**User Experience:**
```
User: "I want classic hard sync lead sounds"
Current: NOT POSSIBLE
Expected: Osc2 can sync to Osc1 (standard feature)
```

**What's Missing:**
- Hard sync (Osc2 resets phase when Osc1 cycles)
- Ring modulation (Osc1 * Osc2)
- FM (Osc1 modulates Osc2 frequency)

**Impact:** Can't create classic analog sync sounds (very common in EDM/synthwave)

**Competitors Have:** Literally every modern synth (Serum, Vital, Massive, Diva)

---

### 5. **No Sub Oscillator** ⚠️
**Problem:** No dedicated sub bass oscillator

**User Experience:**
```
User: "I want a fat bass with sub"
Current: Use Osc3 as sub, tune down 1-2 octaves manually
Expected: Dedicated "Sub" knob (sine wave, -1 or -2 octaves)
```

**What's Missing:**
```cpp
// Should have:
float subOscAmount_ = 0.0f;
float subOsc = std::sin(phase * 0.5);  // One octave down
mixed += subOsc * subOscAmount_;
```

**Impact:** Bass sounds lack weight compared to competitors

---

### 6. **Chorus is Too Simple** ⚠️
**Problem:** Single delay line, no stereo spread control

**User Experience:**
```cpp
// Current chorus (ZenithEffects):
float delaySamplesL = baseDelay + modDelay;
float delaySamplesR = baseDelay - modDelay;  // ❌ Fixed stereo spread
```

**What Users Expect:**
- Chorus rate control (currently hardcoded to 0.5 Hz)
- Chorus depth control (currently hardcoded)
- Chorus feedback
- Multiple delay taps (2-4 voices)

**Impact:** Chorus sounds thin compared to dedicated chorus plugins

---

### 7. **No Reverb** ⚠️
**Problem:** Most synths have built-in reverb

**User Experience:**
```
User: "I want a lush pad with reverb"
Current: Must add external reverb plugin
Expected: Built-in reverb (size, damping, mix)
```

**Impact:** Workflow friction, especially for pads/leads

---

### 8. **No Delay** ⚠️
**Problem:** No delay effect (very common in synths)

**User Experience:**
```
User: "I want a ping-pong delay lead"
Current: Must add external delay plugin
Expected: Built-in delay (time, feedback, ping-pong)
```

**Impact:** Can't create classic delay-based sounds internally

---

### 9. **No Noise Oscillator Level Control** 🟡
**Problem:** Noise waveform is full volume, can't blend

**User Experience:**
```
User: "I want to add a bit of noise to my pad"
Current: Set Osc1 to Noise, adjust Osc1Mix
Problem: Noise is either ON or OFF, can't layer with other waveforms
Expected: Dedicated "Noise" knob (0-100%) that layers with oscillators
```

**What's Missing:**
```cpp
float noiseLevel_ = 0.0f;
float noise = (random.nextFloat() * 2.0f - 1.0f) * noiseLevel_;
mixed += noise;  // Add to oscillator mix
```

---

### 10. **No Arpeggiator** 🟡
**Problem:** No built-in arp (common in modern synths)

**User Experience:**
```
User: "I want an arpeggiated sequence"
Current: Must use external MIDI arp or draw notes
Expected: Built-in arp (up, down, up/down, random, rate, gate)
```

**Impact:** Less immediate for electronic music production

---

## 🤔 **FUNCTIONS WEIRDLY / UNEXPECTEDLY**

### 1. **Filter Drive is Pre-Filter** 😕
**Current Behavior:**
```cpp
input *= drive_;        // Apply drive
input = std::tanh(input);  // Saturate
// THEN filter
```

**User Expectation:**
Most synths have drive AFTER the filter (post-filter saturation)

**Why It Matters:**
- Pre-filter: Distorts signal before filtering (changes filter character)
- Post-filter: Adds warmth/saturation to filtered signal (more common)

**Recommendation:** Add option for pre/post filter drive

---

### 2. **Unison Detune is Percentage, Not Cents** 😕
**Current:**
```cpp
float detune = (v - effectiveUnisonVoices / 2.0f) * unisonDetune_;
// unisonDetune_ is 0-100 (percentage)
```

**User Expectation:**
Most synths use cents (0-50 cents is typical)

**Why It Matters:**
- Percentage is vague ("what does 50% mean?")
- Cents is precise ("10 cents = slightly detuned")

---

### 3. **Modulation Matrix Amounts are Bipolar (-1 to +1)** 😕
**Current:**
```cpp
float amount = 0.0f; // -1 to +1
```

**User Expectation:**
Some synths use 0-100%, some use -100% to +100%

**Current is Fine:** But should be documented clearly in UI

---

### 4. **No Visual Feedback for Modulation** 😕
**Problem:** Users can't see what's being modulated

**User Experience:**
```
User: "Is LFO1 actually modulating the filter?"
Current: No visual indication
Expected: Parameter knobs wiggle/highlight when modulated
```

**Impact:** Hard to debug modulation routing

---

## 🎯 **WHAT COULD BE ADDED**

### **Essential Features (High Priority)**

1. **LFO Waveforms** 🔴
   - Sine, Triangle, Square, Saw Up, Saw Down, Sample & Hold
   - Per-LFO waveform selection

2. **Filter Envelope Amount** 🔴
   - Simple knob: 0% = no envelope, 100% = full envelope
   - Should be separate from modulation matrix

3. **Oscillator Sync** 🔴
   - Hard sync (Osc2 → Osc1)
   - Sync amount control

4. **Sub Oscillator** ⚠️
   - Dedicated sub (sine wave, -1 octave)
   - Sub level control

5. **Reverb** ⚠️
   - Algorithmic reverb (size, damping, mix)
   - Pre-delay

6. **Delay** ⚠️
   - Stereo/ping-pong delay
   - Time (sync to tempo), feedback, mix

### **Nice-to-Have Features (Medium Priority)**

7. **Ring Modulation**
   - Osc1 × Osc2 (classic metallic sounds)

8. **FM Synthesis**
   - Osc1 modulates Osc2 frequency
   - FM amount control

9. **Noise Level Control**
   - Dedicated noise knob (layers with oscillators)

10. **Arpeggiator**
    - Up, Down, Up/Down, Random patterns
    - Rate, gate, octave range

11. **Improved Chorus**
    - Multiple voices (2-4)
    - Rate, depth, feedback controls
    - Stereo width control

12. **Waveshaping/Saturation**
    - Per-oscillator waveshaping
    - Multiple saturation algorithms (soft clip, hard clip, tube, etc.)

### **Advanced Features (Low Priority)**

13. **Wavetable Oscillators**
    - Currently has "WavetablePos" destination but no wavetables
    - Add wavetable support to oscillators

14. **Step Sequencer**
    - Built-in step sequencer for modulation
    - Per-step pitch, velocity, modulation

15. **MPE Support**
    - Per-note pitch bend, pressure, timbre
    - Already stores MIDI note, good foundation

16. **Envelope Followers**
    - Audio-rate envelope following
    - Sidechain-style ducking

17. **More Filter Types**
    - Notch, Comb, Formant filters
    - Multi-pole options (12dB, 24dB, 48dB)

18. **Modulation Envelope Shapes**
    - Currently only ADSR
    - Add: AHDSR, multi-stage, looping envelopes

---

## 📊 **CONVENIENCE SCORE**

| Category | Score | Notes |
|----------|-------|-------|
| **Ease of Use** | 6/10 | Confusing LFO routing, missing common features |
| **Sound Quality** | 8/10 | Good oscillators, clean filters |
| **Feature Completeness** | 5/10 | Missing sync, sub, reverb, delay, LFO shapes |
| **Workflow** | 6/10 | No visual feedback, some unintuitive behaviors |
| **Modulation** | 7/10 | Good matrix, but LFO system is confusing |
| **Effects** | 4/10 | Basic distortion/chorus, missing reverb/delay |
| **Overall** | **6/10** | Solid foundation, needs polish & features |

---

## 🎵 **COMPARISON TO COMPETITORS**

### **vs. Serum (Industry Standard)**
- ❌ Missing: Wavetables, visual feedback, advanced LFOs, better effects
- ✅ Has: Similar modulation matrix, good filter routing

### **vs. Vital (Free Alternative)**
- ❌ Missing: Wavetables, visual modulation, better effects, arpeggiator
- ✅ Has: Similar architecture, good performance

### **vs. Massive X**
- ❌ Missing: Advanced routing, better effects, more oscillator types
- ✅ Has: Simpler, more CPU-efficient

### **vs. Diva (Analog Emulation)**
- ❌ Missing: Oscillator sync, sub oscillator, vintage character
- ✅ Has: Modern modulation matrix, better performance

---

## 🔧 **PRIORITY FIX LIST**

### **Must Fix (Breaks User Expectations)**
1. 🔴 Add LFO waveform selection
2. 🔴 Add filter envelope amount knob
3. 🔴 Add oscillator hard sync
4. 🔴 Clarify/fix LFO routing (remove dual system)

### **Should Fix (Missing Standard Features)**
5. ⚠️ Add sub oscillator
6. ⚠️ Add reverb effect
7. ⚠️ Add delay effect
8. ⚠️ Add noise level control

### **Nice to Fix (Improves Workflow)**
9. 🟡 Add visual modulation feedback
10. 🟡 Change unison detune to cents
11. 🟡 Add post-filter drive option
12. 🟡 Improve chorus (multi-voice, more controls)

---

## 💡 **USER QUOTES (Imagined)**

> "Why can't I make the LFO a square wave? Every synth has this..." - EDM Producer

> "Where's the reverb? I have to add a plugin for every pad sound?" - Sound Designer

> "The filter envelope is confusing. I just want a simple 'Env Amount' knob." - Beginner

> "No hard sync? That's like... synth 101." - Synthwave Artist

> "It sounds good but feels incomplete compared to Vital or Serum." - Professional Producer

---

## ✅ **FINAL VERDICT**

**The Good:**
- Solid technical foundation
- Good sound quality
- Efficient performance
- Working modulation matrix

**The Bad:**
- Missing essential features (LFO shapes, sync, sub, reverb, delay)
- Confusing LFO routing system
- Limited effects
- No visual feedback

**The Ugly:**
- Feels like a "version 0.5" synth
- Users will immediately notice missing features
- Not competitive with free alternatives (Vital, Surge)

**Recommendation:**
Focus on adding the "Must Fix" items first. These are features users expect in ANY modern synth. Without them, the synth feels incomplete and will frustrate users.

**Overall Rating:** 6/10 - Good foundation, needs essential features to be competitive.
