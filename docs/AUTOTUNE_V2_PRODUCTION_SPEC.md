# Zenith Auto-Tune V2: Production Specification

## 🎯 SCOPE REDUCTION: What Actually Ships in v1.0

**After honest assessment, here's what actually works and what to ship:**

---

## ✅ CORE FEATURES (v1.0 - Actually Works)

### 1. TWO MODES ONLY

#### Mode A: Classic (The "Auto-Tune" Sound)
- **Algorithm:** Custom recreation of Auto-Tune 5
- **Sound:** Hard quantization, fast retune, iconic robot effect
- **Use case:** T-Pain, Cher effect, hip-hop
- **Parameters:**
  - Retune Speed (0-100, where 0=instant/robot)
  - Correction Amount (0-100%)
  - Humanize (0-100%)
  - Formant Preserve (on/off)
- **Presets:** Robot, T-Pain, Cher, Subtle

#### Mode B: Modern (Transparent)
- **Algorithm:** Rubber Band library
- **Sound:** Natural, transparent correction
- **Use case:** Polished vocals, gentle correction
- **Parameters:**
  - Retune Speed (0-800ms)
  - Correction Amount (0-100%)
  - Humanize (0-100%)
  - Formant Preserve (0-100%)
- **Presets:** Natural, Transparent, Tight

### 2. SCALE/KEY
- 12 scales: Major, Minor, Pentatonic, etc.
- 12 keys: C, C#, D, etc.
- Auto-detect (basic implementation)

### 3. MIDI INPUT
- MIDI note sets target pitch
- Pitch bend for microtonal
- Optional (off by default)

### 4. THROAT MODELING (Simplified)
- 5 presets only: Default, Soprano, Tenor, Bass, Monster
- Simple formant shifting
- Not physical modeling (too complex)

---

## ❌ CUT FOR v1.1

### Graph Mode
**Why cut:** Needs 2+ more weeks, ARA2 integration, extensive testing
**Replacement:** Basic "Pitch Display" showing detected pitch

### Harmony Generation
**Why cut:** Not core pitch correction feature
**Replacement:** Focus on making pitch correction perfect first

### Advanced Drift Editing
**Why cut:** Complex UI, needs extensive testing
**Replacement:** Good humanize parameter does 80% of the work

### Scale Auto-Detection
**Why cut:** Basic implementation exists but needs more testing
**Replacement:** Manual scale selection (12 options)

---

## 🎚️ PRESETS (8 Total)

### Classic Mode
1. **Robot** - Retune 0, Correction 100%, Formant On
2. **T-Pain** - Retune 15, Correction 100%, Formant On
3. **Cher** - Retune 30, Correction 90%, Formant On
4. **Subtle Classic** - Retune 50, Correction 70%, Formant Off

### Modern Mode
5. **Natural** - Retune 50ms, Correction 80%, Humanize 60%
6. **Transparent** - Retune 150ms, Correction 40%, Humanize 90%
7. **Tight** - Retune 20ms, Correction 100%, Humanize 30%

### Throat
8. **Monster** - Extreme formant shift

---

## 🔧 TECHNICAL IMPLEMENTATION

### Classic Algorithm (Custom)
```cpp
// Simple but effective:
// 1. Detect pitch
// 2. Quantize to scale (hard)
// 3. Smooth transition (exponential moving average)
// 4. Apply pitch shift (basic resampling)
// 5. Optional formant preservation (simple EQ)
```

**Why not Rubber Band for Classic?**
- Rubber Band sounds too good/transparent
- Classic mode needs the "steppy" artifacts
- Custom algorithm can recreate the specific sound

### Modern Algorithm (Rubber Band)
```cpp
// Rubber Band library
// Phase-locked pitch shifting
// Better for natural correction
```

### MIDI Input
```cpp
// Simple implementation:
// If MIDI note received, use as target pitch
// Otherwise, use scale detection
// Pitch bend = microtonal offset
```

---

## 📊 PERFORMANCE TARGETS

### Latency
- Classic Mode: < 50ms
- Modern Mode: < 30ms (Rubber Band)

### CPU Usage
- < 5% on modern CPU (single voice)
- < 15% with MIDI + Throat

### Quality
- Classic: Matches Auto-Tune Access 90%+
- Modern: Matches Waves Tune Real-Time 90%+

---

## 🎤 TESTING CHECKLIST

### Before Shipping
- [ ] A/B vs Auto-Tune Access (Classic mode)
- [ ] A/B vs Waves Tune (Modern mode)
- [ ] Various vocal types (male, female, breathy, powerful)
- [ ] MIDI input with keyboard
- [ ] All 12 scales
- [ ] All 8 presets
- [ ] Latency measurement
- [ ] CPU usage profiling

### Demo Video Scenes
1. Before/After: Raw vocal → Classic Robot preset
2. Before/After: Raw vocal → Modern Natural preset
3. MIDI control: Playing notes on keyboard
4. Scale switching: Major vs Minor
5. Throat modeling: Normal → Monster voice

---

## 💰 PRICING & POSITIONING

### What to Say
```
"Zenith Auto-Tune: Professional pitch correction included FREE 
with your $100 DAW purchase.

Get the classic Auto-Tune sound AND modern transparent correction.
No subscription. No extra cost. Just pro results."
```

### What NOT to Say
```
❌ "Better than Auto-Tune Pro" (it's not)
❌ "Melodyne killer" (Graph Mode isn't ready)
❌ "AI-powered pitch correction" (it's not AI)
```

### Honest Comparison
```
✅ Matches Auto-Tune Access ($49)
✅ Matches Waves Tune ($79)
✅ FREE with DAW (saves $79-128)
✅ Two algorithms for price of none
```

---

## 📅 REVISED TIMELINE

### Week 1 (This Week)
- [ ] Implement Classic algorithm (custom)
- [ ] Integrate Rubber Band (Modern)
- [ ] Basic UI with mode switching

### Week 2
- [ ] MIDI input implementation
- [ ] Throat modeling (5 presets)
- [ ] All 8 presets programmed

### Week 3
- [ ] Testing and A/B comparisons
- [ ] Latency optimization
- [ ] Bug fixes

### Week 4
- [ ] Integration with mixer
- [ ] Final testing
- [ ] Documentation

**Launch: 4 weeks**

---

## 🎯 SUCCESS METRICS

### Technical
- Classic mode sounds 90% like Auto-Tune 5
- Modern mode sounds 90% like Waves Tune
- Latency under 50ms
- No crashes in 1 hour stress test

### User
- 1000+ downloads in first month
- 80%+ users can get good sound in <5 minutes
- <20% support requests for "how to use"

### Market
- Featured on BedroomProducersBlog
- 3+ YouTube reviews
- Mentioned on Gearslutz/Reddit

---

## 🔮 V2.0 ROADMAP (After Launch)

### v1.1 (3 months after launch)
- Graph Mode (visual editor)
- Undo/Redo
- Better scale auto-detect

### v1.2 (6 months after launch)
- Harmony generation
- Advanced drift editing
- ARA2 integration

### v2.0 (1 year after launch)
- Polyphonic pitch correction
- Stem separation
- AI vocal effects

---

## ✅ DECISION: SHIP v1.0 WITH 2 MODES

**The honest truth:** 
- Two solid modes > Six half-baked features
- Users want "Auto-Tune that works" not "everything"
- Can add Graph Mode later if users ask
- Better to ship and iterate than delay forever

**What ships:** Classic + Modern + MIDI + Throat
**What doesn't:** Graph Mode, Harmony, Scale Detect, Advanced features

**User story:**
```
"I bought Zenith for $100. I got a DAW with Auto-Tune built in.
The Classic mode sounds like T-Pain. The Modern mode fixes my 
vocals naturally. That's all I needed. Saved me $400."
```

That's a win.
