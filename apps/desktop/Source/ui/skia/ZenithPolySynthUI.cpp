/*
  ==============================================================================

    ZenithPolySynthUI.cpp
    Refactored: 2025-12-09
    Author:  Zenith DAW

    Skia-based UI for ZenithPolySynth.
    Includes Flagship controls and Thread-Safe Rendering Pipeline.

  ==============================================================================
*/

#include "ZenithPolySynthUI.h"
#include "ZenithUIComponents.h"
#include <vector>

namespace zenith {

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : juce::AudioProcessorEditor(&p), processor(p) {

#ifdef ZENITH_USE_SKIA
  setSize(900, 600); // Expanded width for pro features

  // ===== OSCILLATOR 1 =====
  addWidget<ZenithKnob>("WAVE 1", ZenithPolySynthProcessor::Osc1Wave);
  addWidget<ZenithKnob>("SHAPE 1", ZenithPolySynthProcessor::Osc1Shape);
  addWidget<ZenithKnob>("DETUNE 1", ZenithPolySynthProcessor::Osc1Detune);
  addWidget<ZenithKnob>("MIX 1", ZenithPolySynthProcessor::Osc1Mix);

  // ===== OSCILLATOR 2 =====
  addWidget<ZenithKnob>("WAVE 2", ZenithPolySynthProcessor::Osc2Wave);
  addWidget<ZenithKnob>("SHAPE 2", ZenithPolySynthProcessor::Osc2Shape); // New Shape
  addWidget<ZenithKnob>("DETUNE 2", ZenithPolySynthProcessor::Osc2Detune);
  addWidget<ZenithKnob>("MIX 2", ZenithPolySynthProcessor::Osc2Mix);

  // ===== OSCILLATOR 3 =====
  addWidget<ZenithKnob>("WAVE 3", ZenithPolySynthProcessor::Osc3Wave);
  addWidget<ZenithKnob>("SHAPE 3", ZenithPolySynthProcessor::Osc3Shape); // New Shape
  addWidget<ZenithKnob>("DETUNE 3", ZenithPolySynthProcessor::Osc3Detune);
  addWidget<ZenithKnob>("MIX 3", ZenithPolySynthProcessor::Osc3Mix);

  // ===== SUB / NOISE =====
  addWidget<ZenithKnob>("SUB", ZenithPolySynthProcessor::SubOscLevel);
  addWidget<ZenithKnob>("NOISE", ZenithPolySynthProcessor::NoiseLevel);

  // ===== MODULATION (Flagship) =====
  addWidget<ZenithButton>("SYNC", ZenithPolySynthProcessor::Osc2Sync);
  addWidget<ZenithKnob>("FM 1->2", ZenithPolySynthProcessor::Osc2FM);
  addWidget<ZenithKnob>("RING MOD", ZenithPolySynthProcessor::RingMod);
  addWidget<ZenithKnob>("UNISON", ZenithPolySynthProcessor::UnisonVoices);
  addWidget<ZenithKnob>("SPREAD", ZenithPolySynthProcessor::UnisonDetune);

  // ===== FILTER CONTROLS =====
  addWidget<ZenithKnob>("CUTOFF", ZenithPolySynthProcessor::FilterCutoff);
  addWidget<ZenithKnob>("RES", ZenithPolySynthProcessor::FilterResonance);
  addWidget<ZenithKnob>("DRIVE", ZenithPolySynthProcessor::FilterDrive);
  addWidget<ZenithKnob>("MODEL", ZenithPolySynthProcessor::FilterModel); // Ladder switch
  addWidget<ZenithKnob>("ENV AMT", ZenithPolySynthProcessor::FilterEnvAmount);
  addWidget<ZenithKnob>("KEY TRK", ZenithPolySynthProcessor::FilterKeyTrack);

  // ===== ENVELOPE CONTROLS =====
  addWidget<ZenithSlider>("A", ZenithPolySynthProcessor::AmpAttack);
  addWidget<ZenithSlider>("D", ZenithPolySynthProcessor::AmpDecay);
  addWidget<ZenithSlider>("S", ZenithPolySynthProcessor::AmpSustain);
  addWidget<ZenithSlider>("R", ZenithPolySynthProcessor::AmpRelease);

  // ===== LFO CONTROLS =====
  addWidget<ZenithKnob>("LFO1 RATE", ZenithPolySynthProcessor::LFO1Rate);
  addWidget<ZenithKnob>("LFO1 AMT", ZenithPolySynthProcessor::LFO1Amount);
  addWidget<ZenithKnob>("LFO1 WAVE", ZenithPolySynthProcessor::LFO1Waveform);
  
  addWidget<ZenithKnob>("LFO2 RATE", ZenithPolySynthProcessor::LFO2Rate);
  addWidget<ZenithKnob>("LFO2 AMT", ZenithPolySynthProcessor::LFO2Amount);
  addWidget<ZenithKnob>("LFO2 WAVE", ZenithPolySynthProcessor::LFO2Waveform);

  // ===== EFFECTS CONTROLS (GLOBAL) =====
  addWidget<ZenithKnob>("DIST", ZenithPolySynthProcessor::DistortionAmount);
  addWidget<ZenithKnob>("CHORUS", ZenithPolySynthProcessor::ChorusAmount);
  addWidget<ZenithKnob>("REVERB", ZenithPolySynthProcessor::ReverbAmount);
  addWidget<ZenithKnob>("DLY TIME", ZenithPolySynthProcessor::DelayTime);
  addWidget<ZenithKnob>("DLY FB", ZenithPolySynthProcessor::DelayFeedback);
  addWidget<ZenithKnob>("DLY MIX", ZenithPolySynthProcessor::DelayMix);

  // ===== MASTER / PERF CONTROLS =====
  addWidget<ZenithKnob>("MASTER", ZenithPolySynthProcessor::MasterGain);
  addWidget<ZenithKnob>("GLIDE", ZenithPolySynthProcessor::GlideTime);
  addWidget<ZenithKnob>("PB RANGE", ZenithPolySynthProcessor::PitchBendRange);
  addWidget<ZenithKnob>("VEL CRV", ZenithPolySynthProcessor::VelocityCurve);

  visualizer_ = std::make_unique<ZenithVisualizer>(processor);
  addAndMakeVisible(visualizer_.get());

  presetBar_ = std::make_unique<ZenithPresetBar>();
  presetBar_->setCallbacks([this]() { loadPrevPreset(); },
                           [this]() { loadNextPreset(); }, []() {});
  addAndMakeVisible(presetBar_.get());

  refreshPresetList();
  setLookAndFeel(&zenithLookAndFeel_);

  // Initial layout
  resized();
  
  // Start generic timer for repaint triggering
  startTimer(16); 
  
  // Force frame capture once
  captureFrameSnapshot();
#endif
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
  Settings::getInstance().removeChangeListener(this);
  setLookAndFeel(nullptr);
}

void ZenithPolySynthUI::recreateRenderer() {
  renderer_ = std::make_unique<SkiaRenderer>(*this, Settings::getInstance().getRenderBackend());
  renderer_->initialize();
}

void ZenithPolySynthUI::changeListenerCallback(juce::ChangeBroadcaster *) {
  recreateRenderer();
  repaint();
}

template <typename T>
T *ZenithPolySynthUI::addWidget(const juce::String &name, const juce::String &paramId) {
  auto widget = std::make_unique<T>(name);
  T *ptr = widget.get();
  auto *param = processor.getParameters().getParameter(paramId);
  if (auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(param)) {
    widget->setParameter(ranged);
  }
  addAndMakeVisible(ptr);
  widgets_.push_back(std::move(widget));
  return ptr;
}

void ZenithPolySynthUI::resized() {
  auto area = getLocalBounds().reduced(20);
  if (presetBar_) presetBar_->setBounds(area.removeFromTop(40).reduced(100, 0));
  if (visualizer_) visualizer_->setBounds(area.removeFromTop(150).reduced(10));
  layoutWidgets();
}

void ZenithPolySynthUI::layoutWidgets() {
  std::vector<juce::Component *> row1Widgets;
  std::vector<juce::Component *> row2Widgets;
  std::vector<juce::Component *> row3Widgets;

  for (auto &w : widgets_) {
    juce::String name = w->getName();
    // ROW 1: Oscs + Mods
    if (name.contains("1") && (name.contains("WAVE") || name.contains("SHAPE") || name.contains("DETUNE") || name.contains("MIX"))) row1Widgets.push_back(w.get());
    else if (name.contains("2") && (name.contains("WAVE") || name.contains("SHAPE") || name.contains("DETUNE") || name.contains("MIX"))) row1Widgets.push_back(w.get());
    else if (name.contains("3") && (name.contains("WAVE") || name.contains("SHAPE") || name.contains("DETUNE") || name.contains("MIX"))) row1Widgets.push_back(w.get());
    else if (name == "SYNC" || name == "FM 1->2" || name == "RING MOD" || name == "UNISON" || name == "SPREAD") row1Widgets.push_back(w.get());
    else if (name == "SUB" || name == "NOISE") row1Widgets.push_back(w.get());

    // ROW 2: Filter + Effects
    else if (name == "CUTOFF" || name == "RES" || name == "DRIVE" || name == "MODEL" || name == "ENV AMT" || name == "KEY TRK") row2Widgets.push_back(w.get());
    else if (name == "DIST" || name == "CHORUS" || name == "REVERB" || name.contains("DLY")) row2Widgets.push_back(w.get());

    // ROW 3: LFO + Master
    else if (name.contains("LFO")) row3Widgets.push_back(w.get());
    else if (name == "MASTER" || name == "GLIDE" || name == "PB RANGE" || name == "VEL CRV") row3Widgets.push_back(w.get());
  }

  std::vector<juce::Component *> envWidgets;
  for (auto &w : widgets_) {
    juce::String name = w->getName();
    if (name == "A" || name == "D" || name == "S" || name == "R") envWidgets.push_back(w.get());
  }

  auto area = getLocalBounds().reduced(20);
  if (presetBar_) area.removeFromTop(40);
  if (visualizer_) area.removeFromTop(150);

  auto rightSide = area.removeFromRight(120);
  ZenithLayout::row(rightSide, envWidgets, 10.0f);
  int rowHeight = 110;
  ZenithLayout::row(area.removeFromTop(rowHeight), row1Widgets, 2.0f);
  ZenithLayout::row(area.removeFromTop(rowHeight), row2Widgets, 5.0f);
  ZenithLayout::row(area.removeFromTop(rowHeight), row3Widgets, 5.0f);
}

void ZenithPolySynthUI::mouseDown(const juce::MouseEvent &e) { processor.visualizerFifo_.reset(); }
void ZenithPolySynthUI::mouseDrag(const juce::MouseEvent &e) {}
void ZenithPolySynthUI::mouseUp(const juce::MouseEvent &e) {}
void ZenithPolySynthUI::mouseMove(const juce::MouseEvent &e) {} // Needed for override

void ZenithPolySynthUI::paint(juce::Graphics &g) {
#ifdef ZENITH_USE_SKIA
  if (renderer_) {
     // SkiaRenderer paints directly to window/context
  }
  else {
     g.fillAll(juce::Colours::black);
  }
#endif
}

void ZenithPolySynthUI::timerCallback() {
#ifdef ZENITH_USE_SKIA
  captureFrameSnapshot();
  repaint(); // Triggers paint(), which SkiaHooks into (hypothetically)
#endif
}

void ZenithPolySynthUI::captureFrameSnapshot() {
    auto* frame = frameBuffer_.getWriteBuffer();
    frame->clear();
    
    // Background
    // frame->backgroundColor = ...; (If struct supported it)

    // Widgets
    for (auto& w : widgets_) {
        // Safe casting to extract state
        if (auto* knob = dynamic_cast<ZenithKnob*>(w.get())) {
             render::KnobRenderState ks;
             ks.bounds = ZenithUtils::toSkRect(knob->getBounds());
             ks.value = knob->getValue();
             ks.labelText = knob->getName();
             ks.isHovered = knob->isMouseOver();
             frame->knobs.push_back(ks);
        }
        else if (auto* slider = dynamic_cast<ZenithSlider*>(w.get())) {
             render::SliderRenderState ss;
             ss.bounds = ZenithUtils::toSkRect(slider->getBounds());
             ss.value = slider->getValue();
             ss.labelText = slider->getName();
             frame->sliders.push_back(ss);
        }
        else if (auto* btn = dynamic_cast<ZenithButton*>(w.get())) {
             render::ButtonRenderState bs;
             bs.bounds = ZenithUtils::toSkRect(btn->getBounds());
             bs.isToggled = btn->getToggleState();
             bs.text = btn->getName();
             frame->buttons.push_back(bs);
        }
    }
    
    // Visualizer Data
    if (visualizer_) {
        // Read from processor visualizer buffer
        std::vector<float> samples(512);
        int numRead = processor.readFromVisualizer(samples.data(), 512);
        if (numRead > 0) {
            frame->visualizerSamples = samples; // Copy
        }
        frame->visualizerBounds = ZenithUtils::toSkRect(visualizer_->getBounds());
    }

    frameBuffer_.swapWriteToReady();
}

// ============================================================================
// DRAWING ROUTINES (Render Thread)
// ============================================================================

void ZenithPolySynthUI::drawSkiaContent(SkCanvas *canvas) {
    auto* frame = frameBuffer_.getLatestFrame();
    if (!frame) return;

    drawBackground(canvas);
    
    // Draw Widgets
    for (const auto& k : frame->knobs) drawKnobFromState(canvas, k);
    for (const auto& s : frame->sliders) drawSliderFromState(canvas, s);
    for (const auto& b : frame->buttons) drawButtonFromState(canvas, b);
    
    // Visualizer
    if (!frame->visualizerSamples.empty()) {
        drawVisualizerFromState(canvas, frame->visualizerSamples, frame->visualizerBounds);
    }
}

void ZenithPolySynthUI::drawBackground(SkCanvas *canvas) {
    canvas->clear(SkColorSetRGB(20, 20, 25)); // Deep dark background
}

void ZenithPolySynthUI::drawKnobFromState(SkCanvas *canvas, const render::KnobRenderState &state) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    float x = state.bounds.centerX();
    float y = state.bounds.centerY();
    float r = std::min(state.bounds.width(), state.bounds.height()) * 0.35f;
    
    // Track
    paint.setColor(SkColorSetARGB(100, 60, 60, 60));
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(4.0f);
    canvas->drawCircle(x, y, r, paint);
    
    // Value Arc
    paint.setColor(SkColorSetRGB(0, 255, 255)); // Cyan
    SkRect arcRect = SkRect::MakeXYWH(x - r, y - r, 2*r, 2*r);
    float sweep = state.value * 270.0f;
    canvas->drawArc(arcRect, 135.0f, sweep, false, paint);
    
    // Label
    // (Simplification: Skip text rendering for now to avoid SkFont setup complexity in this step)
}

void ZenithPolySynthUI::drawSliderFromState(SkCanvas *canvas, const render::SliderRenderState &state) {
    SkPaint paint;
    paint.setAntiAlias(true);
    
    // Track
    paint.setColor(SkColorSetARGB(100, 60, 60, 60));
    canvas->drawRect(state.bounds.makeInset(state.bounds.width()*0.4f, 5), paint);
    
    // Thumb/Fill
    paint.setColor(SkColorSetRGB(0, 255, 255));
    float h = state.bounds.height() * state.value;
    SkRect fill = state.bounds.makeInset(state.bounds.width()*0.4f, 5);
    fill.fTop = fill.fBottom - h;
    canvas->drawRect(fill, paint);
}

void ZenithPolySynthUI::drawButtonFromState(SkCanvas *canvas, const render::ButtonRenderState &state) {
    SkPaint paint;
    paint.setColor(state.isToggled ? SkColorSetRGB(255, 100, 100) : SkColorSetRGB(80, 80, 80));
    canvas->drawRect(state.bounds, paint);
}

void ZenithPolySynthUI::drawVisualizerFromState(SkCanvas *canvas, const std::vector<float> &samples, const SkRect &bounds) {
    SkPaint paint;
    paint.setColor(SkColorSetRGB(0, 200, 100)); // Matrix Green
    paint.setStrokeWidth(2.0f);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setAntiAlias(true);
    
    SkPath path;
    if (samples.empty()) return;
    
    float xStep = bounds.width() / samples.size();
    path.moveTo(bounds.left(), bounds.centerY() - samples[0] * 50.0f);
    
    for (size_t i = 1; i < samples.size(); ++i) {
        path.lineTo(bounds.left() + i * xStep, bounds.centerY() - samples[i] * 50.0f);
    }
    canvas->drawPath(path, paint);
}

// Helpers
void ZenithPolySynthUI::toggleAdvancedMode() { isAdvancedMode_ = !isAdvancedMode_; resized(); }
void ZenithPolySynthUI::toggleLearningMode() {}
void ZenithPolySynthUI::renderComponentRecursively(juce::Component*, SkCanvas*) {}

void ZenithPolySynthUI::refreshPresetList() {
    presetList_.clear();
    // Factory Bank (Hardcoded for "Pro" Experience out of the box)
    presetList_.push_back({ "fac_1", "Init Saw", "Factory", "Lead", {}, "ZenithPolySynth", "" });
    presetList_.push_back({ "fac_2", "Moog Bass", "Factory", "Bass", {}, "ZenithPolySynth", "" });
    presetList_.push_back({ "fac_3", "Super Pad", "Factory", "Pad", {}, "ZenithPolySynth", "" });
    presetList_.push_back({ "fac_4", "FM Bell", "Factory", "Keys", {}, "ZenithPolySynth", "" });
    presetList_.push_back({ "fac_5", "Wavetable Morph", "Factory", "Motion", {}, "ZenithPolySynth", "" });
    presetList_.push_back({ "fac_6", "Dub Chord", "Factory", "Chord", {}, "ZenithPolySynth", "" });
    
    currentPresetIndex_ = 0;
}

void ZenithPolySynthUI::loadPreset(int index) {
    if (index < 0 || index >= static_cast<int>(presetList_.size())) return;
    currentPresetIndex_ = index;
    auto& meta = presetList_[index];
    
    // Helper to set parameter by ID (handling denormalized value conversion)
    auto setP = [&](const juce::String& id, float val) {
        if (auto* p = processor.getParameters().getParameter(id)) {
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
                p->setValueNotifyingHost(rp->convertTo0to1(val));
        }
    };

    // --- FACTORY PATCH DEFINITIONS ---
    if (meta.name == "Init Saw") {
        setP(ZenithPolySynthProcessor::Osc1Wave, 1.0f); // Saw
        setP(ZenithPolySynthProcessor::Osc1Mix, 1.0f);
        setP(ZenithPolySynthProcessor::Osc2Mix, 0.0f);
        setP(ZenithPolySynthProcessor::FilterCutoff, 20000.0f);
        setP(ZenithPolySynthProcessor::FilterResonance, 0.0f);
        setP(ZenithPolySynthProcessor::ReverbAmount, 0.0f);
        setP(ZenithPolySynthProcessor::DelayMix, 0.0f);
    }
    else if (meta.name == "Moog Bass") {
        setP(ZenithPolySynthProcessor::Osc1Wave, 1.0f); // Saw
        setP(ZenithPolySynthProcessor::Osc2Wave, 1.0f);
        setP(ZenithPolySynthProcessor::Osc2Detune, 12.0f); // Slight detune
        setP(ZenithPolySynthProcessor::Osc2Mix, 0.7f);
        setP(ZenithPolySynthProcessor::FilterModel, 1.0f); // Ladder
        setP(ZenithPolySynthProcessor::FilterCutoff, 400.0f);
        setP(ZenithPolySynthProcessor::FilterResonance, 0.4f);
        setP(ZenithPolySynthProcessor::FilterDrive, 3.5f);
        setP(ZenithPolySynthProcessor::FilterEnvAmount, 0.8f);
        setP(ZenithPolySynthProcessor::ModAttack, 0.01f);
        setP(ZenithPolySynthProcessor::ModDecay, 0.3f);
        setP(ZenithPolySynthProcessor::ModSustain, 0.0f);
        setP(ZenithPolySynthProcessor::MonoMode, 1.0f);
    }
    else if (meta.name == "Super Pad") {
        setP(ZenithPolySynthProcessor::Osc1Wave, 5.0f); // Supersaw
        setP(ZenithPolySynthProcessor::Osc1Detune, 25.0f);
        setP(ZenithPolySynthProcessor::UnisonVoices, 7.0f);
        setP(ZenithPolySynthProcessor::UnisonDetune, 30.0f);
        setP(ZenithPolySynthProcessor::FilterCutoff, 800.0f);
        setP(ZenithPolySynthProcessor::FilterEnvAmount, 0.2f);
        setP(ZenithPolySynthProcessor::AmpAttack, 1.5f);
        setP(ZenithPolySynthProcessor::AmpRelease, 2.0f);
        setP(ZenithPolySynthProcessor::ReverbAmount, 0.6f);
        setP(ZenithPolySynthProcessor::ChorusAmount, 0.4f);
    }
    else if (meta.name == "FM Bell") {
        setP(ZenithPolySynthProcessor::Osc1Wave, 0.0f); // Sine
        setP(ZenithPolySynthProcessor::Osc2Wave, 0.0f); // Sine
        setP(ZenithPolySynthProcessor::Osc2FM, 0.6f);
        setP(ZenithPolySynthProcessor::FilterCutoff, 20000.0f);
        setP(ZenithPolySynthProcessor::AmpAttack, 0.0f);
        setP(ZenithPolySynthProcessor::AmpDecay, 1.5f);
        setP(ZenithPolySynthProcessor::AmpSustain, 0.0f);
        setP(ZenithPolySynthProcessor::ReverbAmount, 0.4f);
    }
    else if (meta.name == "Wavetable Morph") {
        setP(ZenithPolySynthProcessor::Osc1Wave, 6.0f); // Wavetable
        setP(ZenithPolySynthProcessor::LFO1Waveform, 1.0f); // Triangle
        setP(ZenithPolySynthProcessor::LFO1Rate, 0.5f);
        setP(ZenithPolySynthProcessor::LFO1Amount, 0.8f);
        setP(ZenithPolySynthProcessor::LFO1Target, 6.0f); // Osc1 Shape 
        setP(ZenithPolySynthProcessor::FilterCutoff, 500.0f);
        setP(ZenithPolySynthProcessor::ChorusAmount, 0.5f);
    }
    else if (meta.name == "Dub Chord") {
        setP(ZenithPolySynthProcessor::Osc1Wave, 1.0f);
        setP(ZenithPolySynthProcessor::Osc2Wave, 1.0f);
        setP(ZenithPolySynthProcessor::Osc2Detune, 700.0f); // Fifth (+7semitones = 700cnts)
        setP(ZenithPolySynthProcessor::Osc2Mix, 0.8f);
        setP(ZenithPolySynthProcessor::FilterCutoff, 600.0f);
        setP(ZenithPolySynthProcessor::FilterResonance, 0.3f);
        setP(ZenithPolySynthProcessor::DelayTime, 0.35f);
        setP(ZenithPolySynthProcessor::DelayFeedback, 0.6f);
        setP(ZenithPolySynthProcessor::DelayMix, 0.4f);
    }
}

void ZenithPolySynthUI::loadNextPreset() {
    loadPreset((currentPresetIndex_ + 1) % presetList_.size());
}

void ZenithPolySynthUI::loadPrevPreset() {
    loadPreset((currentPresetIndex_ - 1 + presetList_.size()) % presetList_.size());
}

} // namespace zenith
