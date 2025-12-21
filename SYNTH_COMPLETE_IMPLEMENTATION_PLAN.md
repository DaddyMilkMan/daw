# Zenith PolySynth - Complete Implementation Plan

**Date:** 2025-11-27  
**Status:** Ready for UI Implementation

---

## ✅ **COMPLETED WORK**

### **Engine Fixes (16 total)**
1. ✅ Added `addSound()` - Synth produces audio
2. ✅ Fixed envelope modulation - Env1/Env2 work
3. ✅ Pitch modulation - Vibrato/LFOs work
4. ✅ Mix modulation - Oscillator crossfading
5. ✅ Resonance modulation - Dynamic filter Q
6. ✅ Pan/Volume modulation - Stereo effects
7. ✅ Glide/portamento - Smooth transitions
8. ✅ Filter2 routing - Serial/parallel modes
9. ✅ Mono mode - Legato playing
10. ✅ Pitch wheel - Full MIDI support
11. ✅ Effects parameters - Distortion, chorus
12. ✅ Filter2 parameters - Full control
13. ✅ Unison pitch modulation - Consistent behavior
14. ✅ Chorus phase reset - Reproducible
15. ✅ Filter envelope amount - Intuitive sweeps
16. 🚧 Sub oscillator - Members added, needs audio processing

### **UI Design (Completed)**
- ✅ Interviewed 3 personas
- ✅ Designed Simple Mode (600x400px)
- ✅ Designed Advanced Mode (800x600px)
- ✅ Defined control layout
- ✅ Identified essential vs advanced controls

---

## 🎯 **IMPLEMENTATION ROADMAP**

### **PHASE 1: Complete Quick Win Features (2-3 hours)**

#### 1.1 Finish Sub Oscillator (30 min)
```cpp
// In renderNextBlock(), after oscillators:
if (subOscLevel_ > 0.01f) {
  float subFreq = currentFrequency_ * 0.5f; // One octave down
  float subSample = std::sin(subOscPhase_ * juce::MathConstants<double>::twoPi);
  subOscPhase_ += subFreq / getSampleRate();
  if (subOscPhase_ >= 1.0) subOscPhase_ -= 1.0;
  
  mixed += subSample * subOscLevel_;
}
```

**Add parameter:**
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "sub_level", "Sub Level", 0.0f, 1.0f, 0.0f));
```

**Wire up:**
```cpp
voice->setSubOscLevel(getVal("sub_level"));
```

#### 1.2 Add Noise Level Control (20 min)
```cpp
// Add member:
float noiseLevel_ = 0.0f;
juce::Random noiseRandom_;

// In renderNextBlock():
if (noiseLevel_ > 0.01f) {
  float noise = (noiseRandom_.nextFloat() * 2.0f - 1.0f) * noiseLevel_;
  mixed += noise;
}
```

**Add parameter & wire up**

#### 1.3 Add LFO Waveform Selection (1-2 hrs)
```cpp
enum class LFOWaveform {
  Sine = 0,
  Triangle,
  Square,
  SawUp,
  SawDown,
  SampleHold,
  NumWaveforms
};

// Add members:
LFOWaveform lfo1Waveform_ = LFOWaveform::Sine;
LFOWaveform lfo2Waveform_ = LFOWaveform::Sine;
float lfo1SampleHold_ = 0.0f;
float lfo2SampleHold_ = 0.0f;

// In computeModulation():
switch (lfo1Waveform_) {
  case LFOWaveform::Sine:
    lfo1Value_ = std::sin(lfo1Phase_ * ...);
    break;
  case LFOWaveform::Triangle:
    lfo1Value_ = (lfo1Phase_ < 0.5f) 
      ? (4.0f * lfo1Phase_ - 1.0f)
      : (3.0f - 4.0f * lfo1Phase_);
    break;
  case LFOWaveform::Square:
    lfo1Value_ = (lfo1Phase_ < 0.5f) ? 1.0f : -1.0f;
    break;
  case LFOWaveform::SawUp:
    lfo1Value_ = 2.0f * lfo1Phase_ - 1.0f;
    break;
  case LFOWaveform::SawDown:
    lfo1Value_ = 1.0f - 2.0f * lfo1Phase_;
    break;
  case LFOWaveform::SampleHold:
    if (lfo1Phase_ < lastLfo1Phase_) { // Wrapped
      lfo1SampleHold_ = random_.nextFloat() * 2.0f - 1.0f;
    }
    lfo1Value_ = lfo1SampleHold_;
    break;
}
```

#### 1.4 Simplify LFO Routing (30 min)
**Remove LFOTarget enum, use only modulation matrix**

---

### **PHASE 2: Implement Skia UI (4-6 hours)**

#### 2.1 Create Base Component (1 hr)
```cpp
// File: ZenithPolySynthUI.h
class ZenithPolySynthUI : public SkiaComponent {
public:
  ZenithPolySynthUI(ZenithPolySynthProcessor& processor);
  
  void paint(SkCanvas* canvas) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  
  void setMode(UIMode mode); // Simple or Advanced
  
private:
  ZenithPolySynthProcessor& processor_;
  UIMode currentMode_ = UIMode::Simple;
  
  // UI Elements
  std::vector<std::unique_ptr<SkiaKnob>> knobs_;
  std::vector<std::unique_ptr<SkiaButton>> buttons_;
  std::vector<std::unique_ptr<SkiaDropdown>> dropdowns_;
  
  void paintSimpleMode(SkCanvas* canvas);
  void paintAdvancedMode(SkCanvas* canvas);
  void layoutSimpleMode();
  void layoutAdvancedMode();
};
```

#### 2.2 Create Skia Knob Component (1 hr)
```cpp
class SkiaKnob {
public:
  SkiaKnob(const std::string& label, float min, float max, float defaultVal);
  
  void paint(SkCanvas* canvas, int x, int y, int size);
  bool hitTest(int x, int y);
  void setValue(float value);
  float getValue() const;
  void setValueFromMouse(int deltaY);
  
private:
  std::string label_;
  float value_;
  float min_, max_;
  juce::Rectangle<int> bounds_;
  bool isHovered_ = false;
  bool isDragging_ = false;
};
```

#### 2.3 Implement Simple Mode Layout (2 hrs)
```cpp
void ZenithPolySynthUI::paintSimpleMode(SkCanvas* canvas) {
  // Background
  SkPaint bgPaint;
  bgPaint.setColor(SK_ColorBLACK);
  canvas->drawRect(SkRect::MakeWH(600, 400), bgPaint);
  
  // SOUND Section (y: 50-150)
  paintSection(canvas, "SOUND", 10, 50);
  knobs_[WAVE]->paint(canvas, 20, 80, 60);
  knobs_[SUB]->paint(canvas, 100, 80, 60);
  knobs_[NOISE]->paint(canvas, 180, 80, 60);
  
  // FILTER Section (y: 160-260)
  paintSection(canvas, "FILTER", 10, 160);
  knobs_[CUTOFF]->paint(canvas, 20, 190, 80); // Bigger knob
  knobs_[RESONANCE]->paint(canvas, 120, 190, 60);
  knobs_[FILTER_ENV]->paint(canvas, 200, 190, 60);
  paintFilterTypeButtons(canvas, 280, 190);
  
  // ENVELOPE Section (y: 270-370)
  paintSection(canvas, "ENVELOPE", 10, 270);
  knobs_[ATTACK]->paint(canvas, 20, 300, 50);
  knobs_[DECAY]->paint(canvas, 90, 300, 50);
  knobs_[SUSTAIN]->paint(canvas, 160, 300, 50);
  knobs_[RELEASE]->paint(canvas, 230, 300, 50);
  
  // EFFECTS Section (right side)
  paintSection(canvas, "EFFECTS", 320, 50);
  knobs_[DISTORTION]->paint(canvas, 330, 80, 60);
  knobs_[CHORUS]->paint(canvas, 410, 80, 60);
  knobs_[REVERB]->paint(canvas, 490, 80, 60);
  
  // Mode toggle
  paintModeToggle(canvas, 500, 10);
}
```

#### 2.4 Implement Advanced Mode Layout (1 hr)
```cpp
void ZenithPolySynthUI::paintAdvancedMode(SkCanvas* canvas) {
  // Expand window to 800x600
  // Paint grid layout with all controls
  // Show modulation matrix
  // Show dual filters
  // Show LFO controls
}
```

#### 2.5 Wire Up to Processor (1 hr)
```cpp
void ZenithPolySynthUI::knobValueChanged(int knobId, float newValue) {
  auto* param = processor_.getParameters().getParameter(knobId);
  if (param) {
    param->setValueNotifyingHost(newValue);
  }
}

void ZenithPolySynthUI::updateFromProcessor() {
  for (auto& knob : knobs_) {
    auto* param = processor_.getParameters().getParameter(knob->getId());
    if (param) {
      knob->setValue(param->getValue());
    }
  }
}
```

---

### **PHASE 3: Polish & Test (2-3 hours)**

#### 3.1 Add Tooltips
```cpp
void SkiaKnob::paintTooltip(SkCanvas* canvas) {
  if (isHovered_) {
    // Draw tooltip with description
    // Maya's request: "What does this do?"
  }
}
```

#### 3.2 Add Visual Modulation Feedback
```cpp
void SkiaKnob::setModulationAmount(float amount) {
  modulationAmount_ = amount;
  // Draw ring around knob showing modulation
}
```

#### 3.3 Add Preset Browser
```cpp
class SkiaPresetBrowser {
  void paintPresetList(SkCanvas* canvas);
  void loadPreset(const std::string& name);
};
```

#### 3.4 Persona Testing
- Test with Alex's workflow (EDM production)
- Test with Maya's workflow (learning)
- Test with Jordan's workflow (sound design)

---

## 📊 **ESTIMATED TIME**

| Phase | Task | Time | Priority |
|-------|------|------|----------|
| 1.1 | Finish Sub Oscillator | 30 min | 🔴 HIGH |
| 1.2 | Noise Level | 20 min | 🔴 HIGH |
| 1.3 | LFO Waveforms | 1-2 hrs | 🔴 HIGH |
| 1.4 | LFO Routing Fix | 30 min | ⚠️ MEDIUM |
| 2.1 | Base UI Component | 1 hr | 🔴 HIGH |
| 2.2 | Skia Knob | 1 hr | 🔴 HIGH |
| 2.3 | Simple Mode Layout | 2 hrs | 🔴 HIGH |
| 2.4 | Advanced Mode | 1 hr | ⚠️ MEDIUM |
| 2.5 | Wire Up | 1 hr | 🔴 HIGH |
| 3.1 | Tooltips | 30 min | 🟡 LOW |
| 3.2 | Visual Feedback | 1 hr | 🟡 LOW |
| 3.3 | Preset Browser | 1 hr | ⚠️ MEDIUM |
| 3.4 | Testing | 1 hr | 🔴 HIGH |

**Total:** 11-13 hours

---

## 🎯 **RECOMMENDED APPROACH**

### **Sprint 1: Core Features (3-4 hours)**
1. Finish sub oscillator
2. Add noise level
3. Add LFO waveforms
4. Fix LFO routing

**Result:** Synth engine is feature-complete

### **Sprint 2: Simple UI (4-5 hours)**
1. Create base UI component
2. Implement Skia knob
3. Layout simple mode
4. Wire up to processor

**Result:** Usable UI with essential controls

### **Sprint 3: Advanced & Polish (3-4 hours)**
1. Implement advanced mode
2. Add tooltips
3. Add preset browser
4. Persona testing

**Result:** Professional, complete synth

---

## 💬 **PERSONA EXPECTATIONS**

**Alex:**
> "I want to open it and immediately make a bass. Simple mode should have everything I need."

**Maya:**
> "Please make it pretty and easy to understand. I don't want to feel overwhelmed!"

**Jordan:**
> "Simple mode for quick sounds, advanced mode for deep editing. Perfect."

---

**Next Step:** Should I:
A) Complete the quick win features (sub, noise, LFO waveforms)
B) Start implementing the Skia UI
C) Do both in parallel (features first, then UI)

**Recommendation:** Option C - Finish features first (3-4 hrs), then build UI (4-5 hrs)
