# ZenithPolySynth - P0 Implementation Sprint

**Goal:** Implement critical P0 features to make ZenithPolySynth competitive with Serum/Vital/Pigments

**Duration:** 4 weeks

---

## Week 1: Wavetable Oscillator Core

### Tasks:

**1.1 Wavetable Data Structure**
```cpp
// Add to ZenithPolySynthDefs.h
struct Wavetable {
    static constexpr int TABLE_SIZE = 2048;
    static constexpr int NUM_FRAMES = 256;

    std::vector<float> frames;  // 256 frames × 2048 samples
    juce::String name;
    juce::String category;

    // Get interpolated sample at position
    float getSample(float position) const;
    void loadFromAudioFile(const juce::File& file);
    void loadFromWTF(const juce::File& file);  // Serum format
    void loadFromVitalWt(const juce::File& file);  // Vital format
};

enum class WavetableInterpolation {
    Linear = 0,
    Cubic,
    Sinc
};
```

**1.2 WavetableOscillator Class**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/oscillators/WavetableOscillator.h
class WavetableOscillator {
public:
    WavetableOscillator();
    void setWavetable(const Wavetable* wt);
    void setPhase(float phase) { phase_ = phase; }
    void setMorph(float morph);  // 0-1, interpolates between frames
    void setPhaseOffset(float offset);

    float processSample();
    void processBlock(float* buffer, int numSamples);

private:
    const Wavetable* wavetable_ = nullptr;
    float phase_ = 0.0f;
    float morph_ = 0.0f;
    float phaseOffset_ = 0.0f;
    float phaseIncrement_ = 0.0f;

    float interpolateLinear(float position) const;
    float interpolateCubic(float position) const;
};
```

**1.3 Update Voice to Use Wavetable**
```cpp
// In Voice.h
class Voice : public juce::MPESynthesiserVoice {
private:
    std::unique_ptr<WavetableOscillator> osc1_;
    std::unique_ptr<WavetableOscillator> osc2_;
    std::unique_ptr<WavetableOscillator> osc3_;

    // Replace existing oscillator instances
};
```

**1.4 Wavetable Library**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/wavetables/WavetableLibrary.h
class WavetableLibrary {
public:
    static WavetableLibrary& getInstance();

    void loadAllWavetables();
    const std::vector<Wavetable>& getAllWavetables() const;
    const Wavetable* getRandomWavetable() const;
    const Wavetable* getWavetable(const juce::String& name) const;

private:
    std::vector<Wavetable> wavetables_;
    void loadFromDirectory(const juce::File& dir);
};
```

**1.5 Import Formats**
```cpp
// Serum .wtf format
class SerumWavetableLoader {
public:
    static Wavetable loadFromWTF(const juce::File& file);
private:
    static void decodeSSTV24(const std::vector<uint8_t>& in, std::vector<float>& out);
};

// Vital .wt format
class VitalWavetableLoader {
public:
    static Wavetable loadFromWTF(const juce::File& file);
};
```

**Acceptance Criteria:**
- ✅ Load and play wavetable oscillator
- ✅ Morph between wavetable frames smoothly
- ✅ Import .wt files (Serum/Vital format)
- ✅ Import audio samples as single-cycle wavetables
- ✅ 200+ factory wavetables bundled
- ✅ CPU < 2% per voice

---

## Week 2: Advanced Filters

### Tasks:

**2.1 Filter Slopes**
```cpp
// Update ZenithPolySynthDefs.h
enum class FilterSlope {
    _12dB = 0,
    _24dB,
    _36dB,
    _48dB
};

// Add to FilterType enum
class FilterBase {
public:
    virtual void setSlope(FilterSlope slope) = 0;
};
```

**2.2 Diode Ladder Filter**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/filters/DiodeLadderFilter.h
class DiodeLadderFilter : public FilterBase {
public:
    DiodeLadderFilter();

    void setCutoff(float freq) override;
    void setResonance(float res) override;
    void setSlope(FilterSlope slope) override;
    void setDrive(float drive);

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    // Diode ladder state variables
    std::array<float, 4> stages_;
    float drive_;
    float tanhDrive(float x) const;
};
```

**2.3 MS-20 Filter**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/filters/MS20Filter.h
class MS20Filter : public FilterBase {
public:
    MS20Filter();

    void setCutoff(float freq) override;
    void setResonance(float res) override;
    void setSlope(FilterSlope slope) override;
    void setPeak(float peak);  // MS-20 has separate peak control

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    std::array<float, 2> stages_;
    float peak_;
};
```

**2.4 Moog Ladder Filter**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/filters/MoogLadderFilter.h
class MoogLadderFilter : public FilterBase {
public:
    MoogLadderFilter();

    void setCutoff(float freq) override;
    void setResonance(float res) override;
    void setSlope(FilterSlope slope) override;

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    std::array<float, 4> stages_;
    float feedback_;
};
```

**2.5 Filter Drive & Saturation**
```cpp
enum class SaturationType {
    None = 0,
    SoftClip,
    HardClip,
    Tanh,
    Sigmoid,
    Bitcrush
};

class Saturation {
public:
    void setType(SaturationType type);
    void setAmount(float amount);  // 0-1
    float processSample(float input);
private:
    SaturationType type_;
    float amount_;
};
```

**2.6 Comb Filter**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/filters/CombFilter.h
class CombFilter : public FilterBase {
public:
    CombFilter();

    void setCutoff(float freq) override;  // For comb, this is delay time
    void setResonance(float res) override;  // For comb, this is feedback
    void setSlope(FilterSlope slope) override;  // Not used for comb

    void setDelayTime(float seconds);
    void setDamping(float damping);

    float processSample(float input);
    void processBlock(float* buffer, int numSamples);

private:
    juce::AudioBuffer<float> delayBuffer_;
    int writePos_;
    float feedback_;
    float damping_;
};
```

**Acceptance Criteria:**
- ✅ 12/24/36/48 dB slopes
- ✅ 5 filter models (SVF, ladder, diode, MS-20, Moog)
- ✅ Filter drive with 5 saturation algorithms
- ✅ Comb filter for phaser/flanger
- ✅ CPU < 1% per filter

---

## Week 3: Arpeggiator

### Tasks:

**3.1 Arpeggiator Class**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/sequencer/Arpeggiator.h
enum class ArpMode {
    Up = 0,
    Down,
    UpDown,
    Random,
    Chord,
    Order,
    AsPlayed
};

class Arpeggiator {
public:
    Arpeggiator();

    void setMode(ArpMode mode);
    void setRate(float rate);  // In Hz or sync
    void setGate(float gate);  // Note length (0-1)
    void setOctaveRange(int octaves);
    void setSwing(float swing);  // 0-0.5

    void setPattern(const std::vector<int>& pattern);  // 16 steps
    void setVelocityPattern(const std::vector<float>& vel);

    void noteOn(int note, float velocity);
    void noteOff(int note);
    void reset();
    void process(juce::MidiBuffer& buffer, double sampleRate);

private:
    ArpMode mode_;
    float rate_;
    float gate_;
    int octaveRange_;
    float swing_;
    std::vector<int> pattern_;
    std::vector<float> velocityPattern_;

    std::vector<int> activeNotes_;
    int currentStep_;
    int direction_;  // +1 for up, -1 for down
    juce::uint32 lastNoteTime_;
};
```

**3.2 BPM Sync**
```cpp
enum class ArpSyncRate {
    _1_64 = 0,
    _1_32,
    _1_16,
    _1_8,
    _1_4,
    _1_2,
    _1_1,
    _2_1,
    _4_1,
    _8_1,
    _16_1
};

class Arpeggiator {
    void setSyncRate(ArpSyncRate rate);
    void setBPM(double bpm);

private:
    double calculateNoteDuration() const;
};
```

**3.3 Pattern Editor (UI)**
```cpp
// New UI component: ArpPatternEditor.h
class ArpPatternEditor : public juce::Component {
public:
    ArpPatternEditor(Arpeggiator& arp);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    Arpeggiator& arp_;
    std::array<int, 16> pattern_;  // 0 = rest, 1-16 = note offset
};
```

**3.4 Integrate into Voice Manager**
```cpp
// In ZenithPolySynthProcessor
class ZenithPolySynthProcessor {
private:
    std::unique_ptr<Arpeggiator> arpeggiator_;

    void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midiMessages) override {
        // Pass MIDI through arpeggiator
        juce::MidiBuffer arpBuffer;
        arpeggiator_->process(midiMessages, getSampleRate());
        // Process generated notes
    }
};
```

**Acceptance Criteria:**
- ✅ 7 arp modes (Up, Down, Up-Down, Random, Chord, Order, As Played)
- ✅ BPM sync (1/64 to 16 bars)
- ✅ Gate control (note length)
- ✅ Octave range (1-4)
- ✅ Swing (0-50%)
- ✅ 16-step pattern editor
- ✅ Velocity pattern
- ✅ Hold mode (sustain while holding notes)

---

## Week 4: Step LFO & Unison

### Tasks:

**4.1 Step LFO Class**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/lfos/StepLFO.h
class StepLFO {
public:
    StepLFO();

    void setSteps(int numSteps);  // 16, 32, 64
    void setStep(int index, float value);  // -1 to +1
    void setSmoothing(float smooth);  // 0-1
    void setRate(float rate);  // Hz or sync
    void reset();

    float processSample();
    void processBlock(float* buffer, int numSamples);

    const std::vector<float>& getSteps() const { return steps_; }

private:
    std::vector<float> steps_;
    int currentStep_;
    float phase_;
    float smoothing_;
    float currentOutput_;
    float targetOutput_;
};

enum class LFOShape {
    Step = 0,
    Smooth,
    Linear,
    Cubic
};
```

**4.2 Step LFO UI**
```cpp
// New UI component: StepLFOEditor.h
class StepLFOEditor : public juce::Component {
public:
    StepLFOEditor(StepLFO& lfo);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    StepLFO& lfo_;
    int numSteps_;
    void drawStep(juce::Graphics& g, int stepIndex, float value,
                juce::Rectangle<float> rect);
};
```

**4.3 Unison Class**
```cpp
// New file: apps/desktop/Source/instruments/ZenithPolySynth/oscillators/UnisonVoice.h
class UnisonVoice {
public:
    UnisonVoice();
    void setDetune(float cents);
    void setSpread(float spread);  // 0-1 (stereo width)
    void setPan(float pan);  // 0-1 (left to right)
    void setPhaseOffset(float phase);

    float getSample(float input, int voiceIndex);
    void reset();

private:
    float detune_;
    float spread_;
    float pan_;
    float phaseOffset_;
    float leftGain_;
    float rightGain_;
};

class UnisonManager {
public:
    UnisonManager();
    void setNumVoices(int voices);  // 1-16
    void setDetune(float cents);
    void setSpread(float spread);
    void setPanRandom(bool randomize);

    void process(float* left, float* right, int numSamples);
    void reset();

private:
    std::vector<UnisonVoice> voices_;
    int numVoices_;
    juce::Random rng_;
};
```

**4.4 Integrate Unison into Voice**
```cpp
// In Voice.h
class Voice : public juce::MPESynthesiserVoice {
private:
    std::unique_ptr<UnisonManager> unison_;

    float calculateUnisonDetune(int voiceIndex) const;
};
```

**4.5 Update Modulation Matrix**
```cpp
// Add LFO1RateMod, LFO2RateMod as destinations
// Add ArpRateMod, ArpGateMod as destinations
// Add UnisonDetuneMod, UnisonSpreadMod as destinations
```

**Acceptance Criteria:**
- ✅ Step LFO with 16/32/64 steps
- ✅ Per-step smoothing (linear, cubic)
- ✅ Pattern editor (draw)
- ✅ Randomize, shift, reverse patterns
- ✅ BPM sync
- ✅ 16 unison voices
- ✅ Detune (cents), spread (stereo), pan randomization
- ✅ Volume compensation (prevent clipping when unison enabled)

---

## Integration Tasks (Throughout Sprint)

### Update UI Components

**1. Wavetable Editor**
```cpp
// New component: WavetableEditor.h
class WavetableEditor : public juce::Component {
public:
    WavetableEditor(Wavetable& wavetable);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setTool(Tool tool);  // Draw, Smooth, Generate, Import

private:
    Wavetable& wavetable_;
    int currentFrame_;  // Which frame is being edited
    enum class Tool { Draw, Smooth, Generate, Import };

    void generateFromHarmonics();  // Sine add
    void smoothWaveform();
    void drawWaveform(juce::Graphics& g, juce::Rectangle<float> rect);
};
```

**2. Filter Model Selector**
```cpp
// Update existing UI: ZenithPolySynthEditor
void addFilterControls() {
    filterModelSelector_ = std::make_unique<juce::ComboBox>();
    filterModelSelector_->addItem("SVF", 1);
    filterModelSelector_->addItem("Ladder", 2);
    filterModelSelector_->addItem("Diode Ladder", 3);
    filterModelSelector_->addItem("MS-20", 4);
    filterModelSelector_->addItem("Moog", 5);

    filterSlopeSelector_ = std::make_unique<juce::ComboBox>();
    filterSlopeSelector_->addItem("12 dB", 1);
    filterSlopeSelector_->addItem("24 dB", 2);
    filterSlopeSelector_->addItem("36 dB", 3);
    filterSlopeSelector_->addItem("48 dB", 4);

    // Add to UI layout
}
```

**3. Arpeggiator UI**
```cpp
// New component: ArpeggiatorPanel.h
class ArpeggiatorPanel : public juce::Component {
public:
    ArpeggiatorPanel(Arpeggiator& arp);

private:
    Arpeggiator& arp_;
    juce::ComboBox modeSelect_;
    juce::ComboBox rateSelect_;
    juce::Slider gateSlider_;
    juce::Slider octaveSlider_;
    juce::Slider swingSlider_;
    juce::ToggleButton holdButton_;
    std::unique_ptr<ArpPatternEditor> patternEditor_;
};
```

**4. LFO Panel Updates**
```cpp
// Update existing LFO panel to include step mode
void addLFOControls(int lfoIndex) {
    lfoShapeSelect_ = std::make_unique<juce::ComboBox>();
    lfoShapeSelect_->addItem("Sine", 1);
    lfoShapeSelect_->addItem("Triangle", 2);
    lfoShapeSelect_->addItem("Saw", 3);
    lfoShapeSelect_->addItem("Square", 4);
    lfoShapeSelect_->addItem("Step", 5);  // NEW
    lfoShapeSelect_->addItem("Random", 6);  // NEW

    if (lfoShapeSelect_->getSelectedId() == 5) {
        // Show step LFO editor
        stepLFOEditor_ = std::make_unique<StepLFOEditor>(lfo_);
        addAndMakeVisible(*stepLFOEditor_);
    }
}
```

### Update Parameter Manager

```cpp
// In ZenithPolySynthParameterManager.cpp
void addWavetableParameters() {
    addOscParameter("Osc1Wavetable", "Wave", 0.0f, 255.0f, 0.0f);
    addOscParameter("Osc1Morph", "Morph", 0.0f, 1.0f, 0.0f);
    addOscParameter("Osc1Phase", "Phase", 0.0f, 1.0f, 0.0f);
}

void addFilterParameters() {
    addParameter("FilterModel", "Model", 0.0f, 4.0f, 0.0f);  // 5 models
    addParameter("FilterSlope", "Slope", 0.0f, 3.0f, 1.0f);  // 12-48 dB
    addParameter("FilterDrive", "Drive", 0.0f, 1.0f, 0.0f);
}

void addArpParameters() {
    addParameter("ArpMode", "Mode", 0.0f, 6.0f, 0.0f);  // 7 modes
    addParameter("ArpRate", "Rate", 0.0f, 10.0f, 4.0f);  // 1/64 to 16/1
    addParameter("ArpGate", "Gate", 0.1f, 1.0f, 0.8f);
    addParameter("ArpOctave", "Octave", 1.0f, 4.0f, 1.0f);
    addParameter("ArpSwing", "Swing", 0.0f, 0.5f, 0.0f);
}

void addUnisonParameters() {
    addParameter("UnisonVoices", "Voices", 1.0f, 16.0f, 1.0f);
    addParameter("UnisonDetune", "Detune", 0.0f, 50.0f, 5.0f);
    addParameter("UnisonSpread", "Spread", 0.0f, 1.0f, 0.3f);
    addParameter("UnisonPanRandom", "Pan Random", 0.0f, 1.0f, 0.0f);
}
```

---

## Testing Strategy

**Unit Tests:**
```
WavetableTests.cpp:
- testLoadWavetable()
- testMorphInterpolation()
- testImportFromAudioFile()

FilterTests.cpp:
- testFilterSlopes()
- testFilterModels()
- testFilterDrive()

ArpeggiatorTests.cpp:
- testArpModes()
- testBpmSync()
- testPatternPlayback()

StepLFOTests.cpp:
- testStepSequencing()
- testSmoothing()
- testRandomization()

UnisonTests.cpp:
- testUnisonVoices()
- testDetune()
- testSpread()
```

**Integration Tests:**
```
ZenithPolySynthTests.cpp:
- testFullChainWithWavetable()
- testArpeggiatorOutput()
- testUnisonPerformance()
- testCPUUsage()
```

**Performance Targets:**
- Wavetable oscillator: < 2% CPU per voice
- Filter (high quality): < 1% CPU per voice
- Arpeggiator: < 0.5% CPU
- Step LFO: < 0.5% CPU
- Unison (16 voices): < 5% CPU per note

---

## Deliverables

**End of Week 1:**
- ✅ Wavetable oscillator implementation
- ✅ Wavetable loader (audio file, .wt files)
- ✅ 200+ factory wavetables

**End of Week 2:**
- ✅ 5 filter models implemented
- ✅ 4 filter slopes (12-48 dB)
- ✅ Filter drive + saturation

**End of Week 3:**
- ✅ Arpeggiator with 7 modes
- ✅ BPM sync
- ✅ Pattern editor UI

**End of Week 4:**
- ✅ Step LFO with editor
- ✅ 16-voice unison
- ✅ All UI components integrated

**Final Deliverable:**
- ✅ ZenithPolySynth now competitive with Serum/Vital/Pigments on core features
- ✅ Performance tests passing
- ✅ 500+ presets (new factory bank)
- ✅ User documentation updated

---

## Success Metrics

**Feature Completeness:**
- ✅ Match Serum on wavetable synthesis
- ✅ Match Vital on advanced filters
- ✅ Match Pigments on arpeggiator
- ✅ Exceed competitors on unison voices (16 vs. Vital's 8)

**Performance:**
- ✅ CPU < 5% per voice at 48kHz
- ✅ 32 polyphony at < 20% CPU
- ✅ Latency < 5ms

**User Experience:**
- ✅ Intuitive wavetable editor
- ✅ Smooth morphing
- ✅ Fast preset loading
- ✅ Responsive UI (60 FPS)

**Differentiation:**
- ✅ Most unison voices (16)
- ✅ Most filter models (5)
- ✅ Most modulation slots (expanded from 64 to 128)
- ✅ Step LFO with 64 steps (most have 32)

---

This sprint will transform ZenithPolySynth from basic subtractive synth to competitive wavetable synthesizer, establishing foundation for P1 features (FM, granular, etc.) in following months.
