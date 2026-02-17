/*
  ==============================================================================

    ZenithPolySynthUI.cpp
    Refactored: 2026-02-17
    Author: Zenith DAW

    Production Skia UI for ZenithPolySynth:
    - Full control surface with grouped sections
    - Dedicated modulation matrix panel
    - Performance macros mapped to real parameters

  ==============================================================================
*/

#include "ZenithPolySynthUI.h"
#include "ZenithDesignSystem.h"
#include <algorithm>
#include <cmath>

namespace zenith {

namespace {
constexpr int kUiWidth = 1360;
constexpr int kUiHeight = 820;
constexpr int kPadding = 14;
constexpr int kHeaderHeight = 42;
constexpr int kSectionGap = 10;
constexpr int kControlCols = 5;
constexpr int kMacroKnobSize = 92;
constexpr int kMacroGap = 22;
} // namespace

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      renderer_(std::make_unique<SkiaRenderer>(*this, SkiaRenderer::Backend::Auto)),
      visualizer_(std::make_unique<ZenithVisualizer>(processor)),
      modMatrix_(std::make_unique<ZenithModMatrix>(processor)) {
  setOpaque(false);
  setBufferedToImage(true);
  setResizable(true, true);
  setResizeLimits(1080, 700, 1920, 1200);
  setSize(kUiWidth, kUiHeight);

  addAndMakeVisible(*visualizer_);
  addAndMakeVisible(*modMatrix_);

  buildUI();
  initMacroDefinitions();
  bindMacroTargets();

  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) {
    startTimerHz(60);
  }

  syncProcessorToUI();
  design::ThemeManager::getInstance().addChangeListener(this);

  if (!renderer_->initialize()) {
    DBG("ZenithPolySynthUI: SkiaRenderer failed to initialize");
  }
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
  stopTimer();
  if (renderer_) {
    renderer_->shutdown();
  }
  design::ThemeManager::getInstance().removeChangeListener(this);
}

void ZenithPolySynthUI::paint(juce::Graphics& g) {
  juce::ignoreUnused(g);
  renderer_->render([this](SkCanvas* canvas) { drawSkiaContent(canvas); });
}

void ZenithPolySynthUI::resized() {
  layoutWidgets();
}

template <typename T>
T* ZenithPolySynthUI::addWidget(const juce::String& name, const juce::String& paramId) {
  auto widget = std::make_unique<T>(name);

  if (auto* param = processor.getParameters().getParameter(paramId)) {
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param)) {
      widget->setParameter(ranged);
    }
  }

  T* ptr = widget.get();
  addAndMakeVisible(*widget);
  ptr->addMouseListener(this, true);
  widgets_.push_back(std::move(widget));
  return ptr;
}

ZenithKnob* ZenithPolySynthUI::addKnob(const juce::String& name,
                                       const juce::String& paramId,
                                       const juce::String& helpText,
                                       std::vector<ZenithKnob*>& section) {
  auto* knob = addWidget<ZenithKnob>(name, paramId);
  knob->setTooltip(helpText);
  knob->setShowTicks(true);
  knob->setTickCount(9);
  knob->setTrackWidth(5.5f);
  section.push_back(knob);
  return knob;
}

void ZenithPolySynthUI::buildUI() {
  widgets_.clear();

  std::vector<ZenithKnob*> oscSection;
  std::vector<ZenithKnob*> filterSection;
  std::vector<ZenithKnob*> envSection;
  std::vector<ZenithKnob*> lfoSection;
  std::vector<ZenithKnob*> fxSection;

  addKnob("O1 Wave", ZenithPolySynthProcessor::Osc1Wave, "Oscillator 1 waveform", oscSection)->setTickCount(7);
  addKnob("O1 Detune", ZenithPolySynthProcessor::Osc1Detune, "Fine detune for oscillator 1", oscSection);
  addKnob("O1 Mix", ZenithPolySynthProcessor::Osc1Mix, "Oscillator 1 level", oscSection);
  addKnob("O2 Wave", ZenithPolySynthProcessor::Osc2Wave, "Oscillator 2 waveform", oscSection)->setTickCount(7);
  addKnob("O2 Detune", ZenithPolySynthProcessor::Osc2Detune, "Fine detune for oscillator 2", oscSection);
  addKnob("O2 Mix", ZenithPolySynthProcessor::Osc2Mix, "Oscillator 2 level", oscSection);
  addKnob("O3 Wave", ZenithPolySynthProcessor::Osc3Wave, "Oscillator 3 waveform", oscSection)->setTickCount(7);
  addKnob("O3 Mix", ZenithPolySynthProcessor::Osc3Mix, "Oscillator 3 level", oscSection);
  addKnob("Unison", ZenithPolySynthProcessor::UnisonVoices, "Unison voice count", oscSection)->setTickCount(8);
  addKnob("Spread", ZenithPolySynthProcessor::UnisonDetune, "Unison detune spread", oscSection);

  addKnob("Cutoff", ZenithPolySynthProcessor::FilterCutoff, "Filter cutoff", filterSection);
  addKnob("Res", ZenithPolySynthProcessor::FilterResonance, "Filter resonance", filterSection);
  addKnob("Drive", ZenithPolySynthProcessor::FilterDrive, "Filter drive", filterSection);
  addKnob("Key Trk", ZenithPolySynthProcessor::FilterKeyTrack, "Filter key tracking", filterSection);
  addKnob("Env Amt", ZenithPolySynthProcessor::FilterEnvAmount, "Filter envelope amount", filterSection);

  addKnob("A", ZenithPolySynthProcessor::AmpAttack, "Amp attack", envSection);
  addKnob("D", ZenithPolySynthProcessor::AmpDecay, "Amp decay", envSection);
  addKnob("S", ZenithPolySynthProcessor::AmpSustain, "Amp sustain", envSection);
  addKnob("R", ZenithPolySynthProcessor::AmpRelease, "Amp release", envSection);
  addKnob("M A", ZenithPolySynthProcessor::ModAttack, "Mod envelope attack", envSection);
  addKnob("M D", ZenithPolySynthProcessor::ModDecay, "Mod envelope decay", envSection);
  addKnob("M S", ZenithPolySynthProcessor::ModSustain, "Mod envelope sustain", envSection);
  addKnob("M R", ZenithPolySynthProcessor::ModRelease, "Mod envelope release", envSection);

  addKnob("LFO1 Rate", ZenithPolySynthProcessor::LFO1Rate, "LFO 1 speed", lfoSection);
  addKnob("LFO1 Amt", ZenithPolySynthProcessor::LFO1Amount, "LFO 1 amount", lfoSection);
  addKnob("LFO2 Rate", ZenithPolySynthProcessor::LFO2Rate, "LFO 2 speed", lfoSection);
  addKnob("LFO2 Amt", ZenithPolySynthProcessor::LFO2Amount, "LFO 2 amount", lfoSection);

  addKnob("Dist", ZenithPolySynthProcessor::DistortionAmount, "Distortion amount", fxSection);
  addKnob("Chorus", ZenithPolySynthProcessor::ChorusAmount, "Chorus amount", fxSection);
  addKnob("Reverb", ZenithPolySynthProcessor::ReverbAmount, "Reverb amount", fxSection);
  addKnob("Delay", ZenithPolySynthProcessor::DelayMix, "Delay wet amount", fxSection);
  addKnob("D Time", ZenithPolySynthProcessor::DelayTime, "Delay time", fxSection);
  addKnob("D FB", ZenithPolySynthProcessor::DelayFeedback, "Delay feedback", fxSection);
  addKnob("Glide", ZenithPolySynthProcessor::GlideTime, "Portamento time", fxSection);
  addKnob("Output", ZenithPolySynthProcessor::MasterGain, "Master gain", fxSection);

  for (int i = 0; i < static_cast<int>(macroKnobs_.size()); ++i) {
    macroKnobs_[i] = std::make_unique<ZenithKnob>("Macro " + juce::String(i + 1), design::colors::ACCENT_PRIMARY);
    macroKnobs_[i]->setRange(0.0f, 1.0f, 0.001f);
    macroKnobs_[i]->setValue(0.5f, false);
    macroKnobs_[i]->setShowTicks(true);
    macroKnobs_[i]->setTickCount(11);

    const int macroIndex = i;
    macroKnobs_[i]->onDragStart = [this, macroIndex]() { beginMacroGesture(macroIndex); };
    macroKnobs_[i]->onDragEnd = [this, macroIndex]() { endMacroGesture(macroIndex); };
    macroKnobs_[i]->onValueChanged = [this, macroIndex](float value) {
      applyMacroValue(macroIndex, value);
    };

    addAndMakeVisible(*macroKnobs_[i]);
    macroKnobs_[i]->addMouseListener(this, true);
  }

  layoutWidgets();
}

void ZenithPolySynthUI::layoutWidgets() {
  auto bounds = getLocalBounds().reduced(kPadding);
  bounds.removeFromTop(kHeaderHeight);

  auto macroArea = bounds.removeFromBottom(132);
  bounds.removeFromBottom(kSectionGap);

  auto visualizerArea = bounds.removeFromBottom(124);
  bounds.removeFromBottom(kSectionGap);

  auto matrixArea = bounds.removeFromRight(460);
  bounds.removeFromRight(kSectionGap);
  matrixBounds_ = matrixArea;
  visualizerBounds_ = visualizerArea;
  macroBounds_ = macroArea;

  if (modMatrix_) {
    modMatrix_->setBounds(matrixArea);
  }

  if (visualizer_) {
    visualizer_->setBounds(visualizerArea);
  }

  const int sectionCount = 5;
  const int sectionGap = 8;
  const int sectionHeight = (bounds.getHeight() - sectionGap * (sectionCount - 1)) / sectionCount;

  int widgetIndex = 0;
  int sectionIndex = 0;
  auto layoutSection = [&](int count) {
    auto section = bounds.removeFromTop(sectionHeight);
    bounds.removeFromTop(sectionGap);
    if (sectionIndex < (int)sectionBounds_.size()) {
      sectionBounds_[sectionIndex] = section;
    }
    ++sectionIndex;

    const int cols = kControlCols;
    const int rows = juce::jmax(1, static_cast<int>(std::ceil(static_cast<float>(count) / cols)));
    auto grid = section.reduced(8, 6);
    const int cellW = juce::jmax(56, grid.getWidth() / cols);
    const int cellH = juce::jmax(52, grid.getHeight() / rows);

    for (int i = 0; i < count && widgetIndex < static_cast<int>(widgets_.size()); ++i, ++widgetIndex) {
      const int row = i / cols;
      const int col = i % cols;
      widgets_[widgetIndex]->setBounds(grid.getX() + col * cellW,
                                       grid.getY() + row * cellH,
                                       cellW,
                                       cellH);
    }
  };

  layoutSection(10); // osc
  layoutSection(5);  // filter
  layoutSection(8);  // env
  layoutSection(4);  // lfo
  layoutSection(8);  // fx/global

  auto macroContent = macroArea.reduced(8, 20);
  const int reservedEditorW = juce::jlimit(280, 360, macroArea.getWidth() / 3) + 16;
  if (macroContent.getWidth() > reservedEditorW + 220) {
    macroContent.removeFromRight(reservedEditorW);
  }
  const int totalWidth = static_cast<int>(macroKnobs_.size()) * kMacroKnobSize +
                         (static_cast<int>(macroKnobs_.size()) - 1) * kMacroGap;
  int x = macroContent.getCentreX() - totalWidth / 2;
  const int y = macroContent.getY() + 8;
  for (auto& knob : macroKnobs_) {
    if (knob) {
      knob->setBounds(x, y, kMacroKnobSize, kMacroKnobSize);
      x += kMacroKnobSize + kMacroGap;
    }
  }

  updateMacroEditorLayout();
}

void ZenithPolySynthUI::initMacroDefinitions() {
  macroDefs_[0].name = "Tone";
  macroDefs_[0].color = SkColorSetRGB(0, 230, 255);
  macroDefs_[0].targets = {
      {ZenithPolySynthProcessor::FilterCutoff, 0.35f},
      {ZenithPolySynthProcessor::FilterResonance, 0.12f},
      {ZenithPolySynthProcessor::Osc1Shape, 0.20f},
      {ZenithPolySynthProcessor::Osc2Shape, 0.18f},
  };

  macroDefs_[1].name = "Motion";
  macroDefs_[1].color = SkColorSetRGB(120, 255, 180);
  macroDefs_[1].targets = {
      {ZenithPolySynthProcessor::LFO1Rate, 0.28f},
      {ZenithPolySynthProcessor::LFO2Rate, 0.24f},
      {ZenithPolySynthProcessor::LFO1Amount, 0.24f},
      {ZenithPolySynthProcessor::DelayFeedback, 0.22f},
  };

  macroDefs_[2].name = "Body";
  macroDefs_[2].color = SkColorSetRGB(255, 180, 80);
  macroDefs_[2].targets = {
      {ZenithPolySynthProcessor::SubOscLevel, 0.40f},
      {ZenithPolySynthProcessor::NoiseLevel, -0.22f},
      {ZenithPolySynthProcessor::UnisonDetune, 0.24f},
      {ZenithPolySynthProcessor::FilterDrive, 0.22f},
  };

  macroDefs_[3].name = "Space";
  macroDefs_[3].color = SkColorSetRGB(215, 145, 255);
  macroDefs_[3].targets = {
      {ZenithPolySynthProcessor::ReverbAmount, 0.48f},
      {ZenithPolySynthProcessor::DelayMix, 0.36f},
      {ZenithPolySynthProcessor::DelayTime, 0.26f},
      {ZenithPolySynthProcessor::MasterGain, -0.10f},
  };

  for (int i = 0; i < static_cast<int>(macroKnobs_.size()); ++i) {
    if (macroKnobs_[i]) {
      macroKnobs_[i]->setLabel(macroDefs_[i].name);
      macroKnobs_[i]->setAccentColor(macroDefs_[i].color);
      macroKnobs_[i]->setTooltip("Performance macro: " + macroDefs_[i].name);
    }
  }
}

void ZenithPolySynthUI::bindMacroTargets() {
  for (int macroIndex = 0; macroIndex < static_cast<int>(macroDefs_.size()); ++macroIndex) {
    auto& def = macroDefs_[macroIndex];
    auto& macroAssignments = processor.getMacroController();
    macroAssignments.clearAssignments(macroIndex);

    int slot = 0;
    for (const auto& target : def.targets) {
      MacroCurve curve = MacroCurve::Linear;
      switch (target.curve) {
        case MacroTarget::Curve::Soft:
          curve = MacroCurve::Soft;
          break;
        case MacroTarget::Curve::Hard:
          curve = MacroCurve::Hard;
          break;
        case MacroTarget::Curve::Linear:
        default:
          curve = MacroCurve::Linear;
          break;
      }
      const float assignmentDepth = target.bipolar ? target.depth : std::abs(target.depth);
      macroAssignments.assignMacroToParameter(macroIndex, slot++, target.paramId,
                                              assignmentDepth, curve);
    }

    macroParams_[macroIndex] = dynamic_cast<juce::RangedAudioParameter*>(
        processor.getParameters().getParameter(macroParamIdForIndex(macroIndex)));
    if (macroParams_[macroIndex] != nullptr) {
      macroParams_[macroIndex]->setValueNotifyingHost(macroValues_[macroIndex]);
    }
    processor.getMacroController().setMacroValue(macroIndex, macroValues_[macroIndex]);
  }
}

const juce::String& ZenithPolySynthUI::macroParamIdForIndex(int macroIndex) const {
  switch (macroIndex) {
    case 0:
      return ZenithPolySynthProcessor::Macro1;
    case 1:
      return ZenithPolySynthProcessor::Macro2;
    case 2:
      return ZenithPolySynthProcessor::Macro3;
    default:
      return ZenithPolySynthProcessor::Macro4;
  }
}

void ZenithPolySynthUI::beginMacroGesture(int macroIndex) {
  if (macroIndex < 0 || macroIndex >= static_cast<int>(macroParams_.size())) {
    return;
  }

  activeMacroGestureParam_ = macroParams_[macroIndex];
  if (activeMacroGestureParam_ != nullptr) {
    activeMacroGestureParam_->beginChangeGesture();
  }
}

void ZenithPolySynthUI::endMacroGesture(int macroIndex) {
  juce::ignoreUnused(macroIndex);
  if (activeMacroGestureParam_ != nullptr) {
    activeMacroGestureParam_->endChangeGesture();
    activeMacroGestureParam_ = nullptr;
  }
}

void ZenithPolySynthUI::applyMacroValue(int macroIndex, float value) {
  if (macroIndex < 0 || macroIndex >= static_cast<int>(macroDefs_.size())) {
    return;
  }

  macroValues_[macroIndex] = juce::jlimit(0.0f, 1.0f, value);
  processor.getMacroController().setMacroValue(macroIndex, macroValues_[macroIndex]);

  auto* macroParam = macroParams_[macroIndex];
  if (macroParam != nullptr) {
    macroParam->setValueNotifyingHost(macroValues_[macroIndex]);
  }
}

void ZenithPolySynthUI::timerCallback() {
  if (visualizer_) {
    visualizer_->updateAudioData();
  }
  repaint();
}

#ifdef ZENITH_USE_SKIA
void ZenithPolySynthUI::drawSkiaContent(SkCanvas* canvas) {
  auto bounds = getLocalBounds();

  canvas->clear(design::colors::BG_DARKEST);

  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::colors::BG_DARKER);
  canvas->drawRect(SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()), bgPaint);

  SkPaint linePaint;
  linePaint.setAntiAlias(true);
  linePaint.setColor(SkColorSetARGB(55, 80, 125, 185));
  linePaint.setStrokeWidth(1.0f);
  const float y = (float)kHeaderHeight + kPadding;
  canvas->drawLine((float)kPadding, y, (float)(bounds.getWidth() - kPadding), y, linePaint);

  SkFont titleFont = design::getSkFont(18.0f, design::FontWeight::SemiBold);
  SkPaint titlePaint;
  titlePaint.setAntiAlias(true);
  titlePaint.setColor(design::colors::TEXT_PRIMARY);
  canvas->drawString("Zenith Poly Synth", (float)kPadding, 28.0f, titleFont, titlePaint);

  SkFont subtitleFont = design::getSkFont(11.0f, design::FontWeight::Regular);
  SkPaint subtitlePaint;
  subtitlePaint.setAntiAlias(true);
  subtitlePaint.setColor(design::colors::TEXT_SECONDARY);
  canvas->drawString("Skia performance surface with live modulation routing", (float)kPadding,
                     42.0f, subtitleFont, subtitlePaint);

  static const std::array<const char*, 5> sectionTitles = {
      "Oscillators", "Filter", "Envelopes", "LFOs", "FX and Output"};
  SkFont sectionTitleFont = design::getSkFont(10.5f, design::FontWeight::SemiBold);
  for (int i = 0; i < (int)sectionBounds_.size(); ++i) {
    const auto& section = sectionBounds_[i];
    if (section.isEmpty()) {
      continue;
    }

    SkPaint sectionPaint;
    sectionPaint.setAntiAlias(true);
    sectionPaint.setColor(SkColorSetARGB(50, 100, 145, 215));
    canvas->drawRoundRect(
        SkRect::MakeXYWH((float)section.getX(), (float)section.getY(),
                         (float)section.getWidth(), (float)section.getHeight()),
        8.0f, 8.0f, sectionPaint);

    SkPaint sectionStrokePaint;
    sectionStrokePaint.setAntiAlias(true);
    sectionStrokePaint.setStyle(SkPaint::kStroke_Style);
    sectionStrokePaint.setStrokeWidth(1.0f);
    sectionStrokePaint.setColor(SkColorSetARGB(70, 150, 190, 250));
    canvas->drawRoundRect(
        SkRect::MakeXYWH((float)section.getX(), (float)section.getY(),
                         (float)section.getWidth(), (float)section.getHeight()),
        8.0f, 8.0f, sectionStrokePaint);

    SkPaint sectionTextPaint;
    sectionTextPaint.setAntiAlias(true);
    sectionTextPaint.setColor(design::withAlpha(design::colors::TEXT_PRIMARY, 0.86f));
    canvas->drawString(sectionTitles[(size_t)i], (float)section.getX() + 10.0f,
                       (float)section.getY() + 14.0f, sectionTitleFont,
                       sectionTextPaint);
  }

  auto drawPanelLabel = [&](const juce::Rectangle<int>& r, const char* text) {
    if (r.isEmpty()) {
      return;
    }
    SkPaint p;
    p.setAntiAlias(true);
    p.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.9f));
    canvas->drawString(text, (float)r.getX() + 8.0f, (float)r.getY() + 15.0f,
                       sectionTitleFont, p);
  };
  drawPanelLabel(matrixBounds_, "Modulation Matrix");
  drawPanelLabel(visualizerBounds_, "Output Visualizer");
  drawPanelLabel(macroBounds_, "Performance Macros");

  if (!macroBounds_.isEmpty()) {
    SkPaint macroCardPaint;
    macroCardPaint.setAntiAlias(true);
    macroCardPaint.setColor(SkColorSetARGB(42, 110, 155, 220));
    canvas->drawRoundRect(
        SkRect::MakeXYWH((float)macroBounds_.getX(), (float)macroBounds_.getY(),
                         (float)macroBounds_.getWidth(), (float)macroBounds_.getHeight()),
        10.0f, 10.0f, macroCardPaint);
  }

  for (const auto& widget : widgets_) {
    if (widget && widget->isVisible()) {
      canvas->save();
      canvas->translate((SkScalar)widget->getX(), (SkScalar)widget->getY());
      widget->drawSkia(canvas);
      canvas->restore();
    }
  }

  for (const auto& macro : macroKnobs_) {
    if (macro && macro->isVisible()) {
      canvas->save();
      canvas->translate((SkScalar)macro->getX(), (SkScalar)macro->getY());
      macro->drawSkia(canvas);
      canvas->restore();
    }
  }

  SkFont macroValueFont = design::getSkFont(10.0f, design::FontWeight::SemiBold);
  for (int i = 0; i < static_cast<int>(macroKnobs_.size()); ++i) {
    if (!macroKnobs_[i] || !macroKnobs_[i]->isVisible()) {
      continue;
    }
    const float centered = (macroValues_[i] - 0.5f) * 2.0f;
    const juce::String valueText = (centered >= 0.0f ? "+" : "") + juce::String(centered * 100.0f, 0) + "%";
    SkPaint valuePaint;
    valuePaint.setAntiAlias(true);
    valuePaint.setColor(design::withAlpha(macroDefs_[i].color, 0.95f));

    const float tx = (float)macroKnobs_[i]->getX() + 30.0f;
    const float ty = (float)macroKnobs_[i]->getBottom() + 14.0f;
    canvas->drawString(valueText.toRawUTF8(), tx, ty, macroValueFont, valuePaint);
  }

  if (!macroEditorBounds_.isEmpty()) {
    const auto macroIndex = juce::jlimit(0, (int)macroDefs_.size() - 1, activeMacroEditorIndex_);
    auto& def = macroDefs_[(size_t)macroIndex];

    SkPaint editorBg;
    editorBg.setAntiAlias(true);
    editorBg.setColor(SkColorSetARGB(170, 12, 18, 28));
    canvas->drawRoundRect(
        SkRect::MakeXYWH((float)macroEditorBounds_.getX(), (float)macroEditorBounds_.getY(),
                         (float)macroEditorBounds_.getWidth(), (float)macroEditorBounds_.getHeight()),
        9.0f, 9.0f, editorBg);

    SkPaint editorStroke;
    editorStroke.setAntiAlias(true);
    editorStroke.setStyle(SkPaint::kStroke_Style);
    editorStroke.setStrokeWidth(1.0f);
    editorStroke.setColor(SkColorSetARGB(96, 120, 165, 220));
    canvas->drawRoundRect(
        SkRect::MakeXYWH((float)macroEditorBounds_.getX(), (float)macroEditorBounds_.getY(),
                         (float)macroEditorBounds_.getWidth(), (float)macroEditorBounds_.getHeight()),
        9.0f, 9.0f, editorStroke);

    SkFont titleFont = design::getSkFont(10.5f, design::FontWeight::SemiBold);
    SkPaint titlePaint;
    titlePaint.setAntiAlias(true);
    titlePaint.setColor(design::withAlpha(design::colors::TEXT_PRIMARY, 0.92f));
    canvas->drawString("Macro Assignments", (float)macroEditorBounds_.getX() + 8.0f,
                       (float)macroEditorBounds_.getY() + 14.0f, titleFont, titlePaint);

    for (int i = 0; i < static_cast<int>(macroTabBounds_.size()); ++i) {
      const auto& tab = macroTabBounds_[(size_t)i];
      SkPaint tabPaint;
      tabPaint.setAntiAlias(true);
      const bool active = i == macroIndex;
      tabPaint.setColor(active ? SkColorSetARGB(128, 90, 145, 225)
                               : SkColorSetARGB(58, 70, 92, 122));
      canvas->drawRoundRect(
          SkRect::MakeXYWH((float)tab.getX(), (float)tab.getY(), (float)tab.getWidth(),
                           (float)tab.getHeight()),
          5.0f, 5.0f, tabPaint);

      SkPaint tabTextPaint;
      tabTextPaint.setAntiAlias(true);
      tabTextPaint.setColor(active ? SkColorSetRGB(234, 244, 255)
                                   : SkColorSetARGB(180, 190, 210, 235));
      const auto tabText = macroDefs_[(size_t)i].name;
      canvas->drawString(tabText.toRawUTF8(), (float)tab.getX() + 8.0f,
                         (float)tab.getY() + 12.0f, titleFont, tabTextPaint);
    }

    SkFont rowFont = design::getSkFont(10.0f, design::FontWeight::Regular);
    for (int i = 0; i < (int)macroTargetRowBounds_.size() && i < (int)def.targets.size(); ++i) {
      const auto& row = macroTargetRowBounds_[(size_t)i];
      const auto& target = def.targets[(size_t)i];
      const bool selected = i == activeMacroTargetIndex_;

      SkPaint rowPaint;
      rowPaint.setAntiAlias(true);
      rowPaint.setColor(selected ? SkColorSetARGB(118, 95, 142, 220)
                                 : SkColorSetARGB(42, 64, 84, 112));
      canvas->drawRoundRect(
          SkRect::MakeXYWH((float)row.getX(), (float)row.getY(), (float)row.getWidth(),
                           (float)row.getHeight()),
          4.0f, 4.0f, rowPaint);

      SkPaint rowText;
      rowText.setAntiAlias(true);
      rowText.setColor(design::withAlpha(design::colors::TEXT_PRIMARY, 0.9f));
      canvas->drawString(target.paramId.toRawUTF8(), (float)row.getX() + 7.0f,
                         (float)row.getY() + 12.0f, rowFont, rowText);

      const juce::String depthText = juce::String(target.depth, 2);
      SkPaint depthPaint;
      depthPaint.setAntiAlias(true);
      depthPaint.setColor(design::withAlpha(def.color, 0.92f));
      canvas->drawString(depthText.toRawUTF8(), (float)row.getRight() - 46.0f,
                         (float)row.getY() + 12.0f, rowFont, depthPaint);

      const char* curveLabel = "LIN";
      if (target.curve == MacroTarget::Curve::Soft) {
        curveLabel = "SFT";
      } else if (target.curve == MacroTarget::Curve::Hard) {
        curveLabel = "HRD";
      }
      SkPaint curvePaint;
      curvePaint.setAntiAlias(true);
      curvePaint.setColor(design::withAlpha(design::colors::TEXT_SECONDARY, 0.86f));
      canvas->drawString(curveLabel, (float)row.getRight() - 72.0f,
                         (float)row.getY() + 12.0f, rowFont, curvePaint);
    }

    if (activeMacroTargetIndex_ >= 0 &&
        activeMacroTargetIndex_ < static_cast<int>(def.targets.size())) {
      const auto& target = def.targets[(size_t)activeMacroTargetIndex_];

      SkPaint sliderTrack;
      sliderTrack.setAntiAlias(true);
      sliderTrack.setColor(SkColorSetARGB(86, 70, 98, 130));
      canvas->drawRoundRect(
          SkRect::MakeXYWH((float)macroDepthSliderBounds_.getX(),
                           (float)macroDepthSliderBounds_.getY(),
                           (float)macroDepthSliderBounds_.getWidth(),
                           (float)macroDepthSliderBounds_.getHeight()),
          4.0f, 4.0f, sliderTrack);

      const float depthRangeStart = target.bipolar ? -1.0f : 0.0f;
      const float depthNorm =
          juce::jlimit(0.0f, 1.0f, (target.depth - depthRangeStart) / (1.0f - depthRangeStart));
      const float fillW = depthNorm * (float)macroDepthSliderBounds_.getWidth();
      SkPaint sliderFill;
      sliderFill.setAntiAlias(true);
      sliderFill.setColor(design::withAlpha(def.color, 0.95f));
      canvas->drawRoundRect(
          SkRect::MakeXYWH((float)macroDepthSliderBounds_.getX(),
                           (float)macroDepthSliderBounds_.getY(), fillW,
                           (float)macroDepthSliderBounds_.getHeight()),
          4.0f, 4.0f, sliderFill);
    }

    auto drawSmallButton = [&](const juce::Rectangle<int>& b, const char* text, bool active) {
      SkPaint bp;
      bp.setAntiAlias(true);
      bp.setColor(active ? SkColorSetARGB(132, 95, 154, 238)
                         : SkColorSetARGB(65, 68, 88, 114));
      canvas->drawRoundRect(
          SkRect::MakeXYWH((float)b.getX(), (float)b.getY(), (float)b.getWidth(),
                           (float)b.getHeight()),
          4.0f, 4.0f, bp);

      SkPaint tp;
      tp.setAntiAlias(true);
      tp.setColor(active ? SK_ColorWHITE : SkColorSetARGB(205, 210, 224, 246));
      SkFont bf = design::getSkFont(9.5f, design::FontWeight::SemiBold);
      canvas->drawString(text, (float)b.getX() + 6.0f, (float)b.getY() + 12.0f, bf, tp);
    };

    drawSmallButton(macroAssignButtonBounds_, macroAssignArmed_ ? "Click a control..." : "Assign by Click",
                    macroAssignArmed_);
    const bool bipolar = (activeMacroTargetIndex_ >= 0 &&
                          activeMacroTargetIndex_ < static_cast<int>(def.targets.size()))
                             ? def.targets[(size_t)activeMacroTargetIndex_].bipolar
                             : true;
    drawSmallButton(macroAddTargetBounds_, "+ Target", false);
    drawSmallButton(macroRemoveTargetBounds_, "- Target", false);
    const char* curveButtonText = "Curve: Linear";
    if (activeMacroTargetIndex_ >= 0 &&
        activeMacroTargetIndex_ < static_cast<int>(def.targets.size())) {
      const auto curve = def.targets[(size_t)activeMacroTargetIndex_].curve;
      curveButtonText = curve == MacroTarget::Curve::Soft
                            ? "Curve: Soft"
                            : (curve == MacroTarget::Curve::Hard ? "Curve: Hard"
                                                                 : "Curve: Linear");
    }
    drawSmallButton(macroCurveButtonBounds_, curveButtonText, true);
    drawSmallButton(macroBipolarToggleBounds_, bipolar ? "Bipolar" : "Unipolar", bipolar);
  }

  if (modMatrix_ && modMatrix_->isVisible()) {
    canvas->save();
    canvas->translate((SkScalar)modMatrix_->getX(), (SkScalar)modMatrix_->getY());
    modMatrix_->drawSkia(canvas);
    canvas->restore();
  }

  if (visualizer_ && visualizer_->isVisible()) {
    canvas->save();
    canvas->translate((SkScalar)visualizer_->getX(), (SkScalar)visualizer_->getY());
    visualizer_->drawSkia(canvas);
    canvas->restore();
  }
}
#endif

void ZenithPolySynthUI::syncProcessorToUI() {
  for (const auto& widget : widgets_) {
    if (auto* knob = dynamic_cast<ZenithKnob*>(widget.get())) {
      knob->updateFromParameter();
    }
  }

  for (int macroIndex = 0; macroIndex < static_cast<int>(macroKnobs_.size()); ++macroIndex) {
    if (macroKnobs_[macroIndex]) {
      float macroValue = 0.5f;
      if (macroParams_[macroIndex] != nullptr) {
        macroValue = macroParams_[macroIndex]->getValue();
      }
      macroKnobs_[macroIndex]->setValue(macroValue, false);
      macroValues_[macroIndex] = macroValue;
      processor.getMacroController().setMacroValue(macroIndex, macroValue);
    }
  }
}

void ZenithPolySynthUI::updateMacroEditorLayout() {
  macroEditorBounds_ = {};
  macroTargetRowBounds_.clear();
  if (macroBounds_.isEmpty()) {
    return;
  }

  auto editorArea = macroBounds_.reduced(0, 2);
  const int editorW = juce::jlimit(280, 360, macroBounds_.getWidth() / 3);
  macroEditorBounds_ = editorArea.removeFromRight(editorW).reduced(8, 8);

  const int tabsY = macroEditorBounds_.getY() + 18;
  const int tabH = 16;
  const int tabW = juce::jmax(54, (macroEditorBounds_.getWidth() - 10) / 4);
  for (int i = 0; i < (int)macroTabBounds_.size(); ++i) {
    macroTabBounds_[(size_t)i] =
        juce::Rectangle<int>(macroEditorBounds_.getX() + 5 + i * tabW, tabsY, tabW - 3, tabH);
  }

  int rowY = tabsY + tabH + 6;
  const int rowH = 14;
  const int rows = 6;
  for (int i = 0; i < rows; ++i) {
    macroTargetRowBounds_.push_back(juce::Rectangle<int>(
        macroEditorBounds_.getX() + 6, rowY, macroEditorBounds_.getWidth() - 12, rowH));
    rowY += rowH + 3;
  }

  macroDepthSliderBounds_ = juce::Rectangle<int>(
      macroEditorBounds_.getX() + 8, macroEditorBounds_.getBottom() - 30,
      macroEditorBounds_.getWidth() - 16, 8);
  macroAddTargetBounds_ = juce::Rectangle<int>(
      macroEditorBounds_.getX() + 8, macroDepthSliderBounds_.getY() - 33, 74, 14);
  macroRemoveTargetBounds_ = juce::Rectangle<int>(
      macroAddTargetBounds_.getRight() + 4, macroDepthSliderBounds_.getY() - 33, 74, 14);
  macroCurveButtonBounds_ = juce::Rectangle<int>(
      macroRemoveTargetBounds_.getRight() + 4, macroDepthSliderBounds_.getY() - 33,
      macroEditorBounds_.getWidth() - 8 - (macroRemoveTargetBounds_.getRight() + 4 - macroEditorBounds_.getX()), 14);
  macroAssignButtonBounds_ = juce::Rectangle<int>(
      macroEditorBounds_.getX() + 8, macroDepthSliderBounds_.getY() - 16,
      macroEditorBounds_.getWidth() - 16, 14);
  macroBipolarToggleBounds_ = juce::Rectangle<int>(
      macroEditorBounds_.getRight() - 82, macroDepthSliderBounds_.getY() - 33, 74, 14);
}

void ZenithPolySynthUI::setSelectedMacroTargetDepth(float depth, bool notify) {
  const int macroIndex = juce::jlimit(0, (int)macroDefs_.size() - 1, activeMacroEditorIndex_);
  auto& def = macroDefs_[(size_t)macroIndex];
  if (activeMacroTargetIndex_ < 0 || activeMacroTargetIndex_ >= (int)def.targets.size()) {
    return;
  }

  auto& target = def.targets[(size_t)activeMacroTargetIndex_];
  const float minDepth = target.bipolar ? -1.0f : 0.0f;
  target.depth = juce::jlimit(minDepth, 1.0f, depth);
  if (!target.bipolar) {
    target.depth = std::abs(target.depth);
  }
  if (notify) {
    bindMacroTargets();
  }
  repaint();
}

bool ZenithPolySynthUI::assignParameterToSelectedMacroTarget(juce::Component* sourceComp) {
  auto* control = dynamic_cast<ZenithControl*>(sourceComp);
  if (control == nullptr || control->getParameter() == nullptr) {
    return false;
  }

  assignSelectedTargetToParamId(control->getParameter()->getParameterID());
  return true;
}

void ZenithPolySynthUI::assignSelectedTargetToParamId(const juce::String& paramId) {
  if (paramId.isEmpty()) {
    return;
  }

  const int macroIndex = juce::jlimit(0, (int)macroDefs_.size() - 1, activeMacroEditorIndex_);
  auto& def = macroDefs_[(size_t)macroIndex];
  if (activeMacroTargetIndex_ < 0 || activeMacroTargetIndex_ >= (int)def.targets.size()) {
    return;
  }

  def.targets[(size_t)activeMacroTargetIndex_].paramId = paramId;
  bindMacroTargets();
  repaint();
}

juce::String ZenithPolySynthUI::getDefaultAssignableParamId() const {
  for (const auto& widget : widgets_) {
    if (auto* control = dynamic_cast<ZenithControl*>(widget.get())) {
      if (auto* param = control->getParameter()) {
        return param->getParameterID();
      }
    }
  }
  return ZenithPolySynthProcessor::FilterCutoff;
}

void ZenithPolySynthUI::changeListenerCallback(juce::ChangeBroadcaster* source) {
  if (source == &design::ThemeManager::getInstance()) {
    repaint();
  }
}

void ZenithPolySynthUI::mouseDown(const juce::MouseEvent& e) {
  if (macroAssignArmed_) {
    auto* control = dynamic_cast<ZenithControl*>(e.originalComponent);
    if (control != nullptr && control->getParameter() != nullptr) {
      pendingAssignParamId_ = control->getParameter()->getParameterID();
      macroAssignDragActive_ = true;
      return;
    }
  }

  const auto pos = e.getPosition();
  if (!macroEditorBounds_.contains(pos)) {
    return;
  }

  for (int i = 0; i < (int)macroTabBounds_.size(); ++i) {
    if (macroTabBounds_[(size_t)i].contains(pos)) {
      activeMacroEditorIndex_ = i;
      activeMacroTargetIndex_ = juce::jlimit(
          0, juce::jmax(0, (int)macroDefs_[(size_t)i].targets.size() - 1), activeMacroTargetIndex_);
      repaint();
      return;
    }
  }

  for (int i = 0; i < (int)macroTargetRowBounds_.size(); ++i) {
    if (macroTargetRowBounds_[(size_t)i].contains(pos) &&
        i < (int)macroDefs_[(size_t)activeMacroEditorIndex_].targets.size()) {
      activeMacroTargetIndex_ = i;
      repaint();
      return;
    }
  }

  if (macroAssignButtonBounds_.contains(pos)) {
    macroAssignArmed_ = !macroAssignArmed_;
    repaint();
    return;
  }

  if (macroBipolarToggleBounds_.contains(pos)) {
    auto& target = macroDefs_[(size_t)activeMacroEditorIndex_]
                       .targets[(size_t)juce::jlimit(
                           0, juce::jmax(0, (int)macroDefs_[(size_t)activeMacroEditorIndex_].targets.size() - 1),
                           activeMacroTargetIndex_)];
    target.bipolar = !target.bipolar;
    if (!target.bipolar) {
      target.depth = std::abs(target.depth);
    }
    bindMacroTargets();
    repaint();
    return;
  }

  if (macroAddTargetBounds_.contains(pos)) {
    auto& def = macroDefs_[(size_t)activeMacroEditorIndex_];
    constexpr int kUiMaxTargets = 6;
    if ((int)def.targets.size() < kUiMaxTargets) {
      MacroTarget target;
      target.paramId = getDefaultAssignableParamId();
      target.depth = 0.0f;
      target.bipolar = true;
      target.curve = MacroTarget::Curve::Linear;
      def.targets.push_back(target);
      activeMacroTargetIndex_ = (int)def.targets.size() - 1;
      bindMacroTargets();
      repaint();
    }
    return;
  }

  if (macroRemoveTargetBounds_.contains(pos)) {
    auto& def = macroDefs_[(size_t)activeMacroEditorIndex_];
    if (def.targets.size() > 1 && activeMacroTargetIndex_ >= 0 &&
        activeMacroTargetIndex_ < (int)def.targets.size()) {
      def.targets.erase(def.targets.begin() + activeMacroTargetIndex_);
      activeMacroTargetIndex_ =
          juce::jlimit(0, juce::jmax(0, (int)def.targets.size() - 1), activeMacroTargetIndex_);
      bindMacroTargets();
      repaint();
    }
    return;
  }

  if (macroCurveButtonBounds_.contains(pos)) {
    auto& def = macroDefs_[(size_t)activeMacroEditorIndex_];
    if (activeMacroTargetIndex_ >= 0 && activeMacroTargetIndex_ < (int)def.targets.size()) {
      auto& curve = def.targets[(size_t)activeMacroTargetIndex_].curve;
      if (curve == MacroTarget::Curve::Linear) {
        curve = MacroTarget::Curve::Soft;
      } else if (curve == MacroTarget::Curve::Soft) {
        curve = MacroTarget::Curve::Hard;
      } else {
        curve = MacroTarget::Curve::Linear;
      }
      bindMacroTargets();
      repaint();
    }
    return;
  }

  if (macroDepthSliderBounds_.contains(pos)) {
    macroDepthDragging_ = true;
    macroDepthDragStartY_ = (float)e.position.y;
    auto& target = macroDefs_[(size_t)activeMacroEditorIndex_]
                       .targets[(size_t)juce::jlimit(
                           0, juce::jmax(0, (int)macroDefs_[(size_t)activeMacroEditorIndex_].targets.size() - 1),
                           activeMacroTargetIndex_)];
    macroDepthDragStartValue_ = target.depth;
    const float depthRangeStart = target.bipolar ? -1.0f : 0.0f;
    const float norm = juce::jlimit(
        0.0f, 1.0f,
        ((float)e.position.x - (float)macroDepthSliderBounds_.getX()) /
            (float)macroDepthSliderBounds_.getWidth());
    setSelectedMacroTargetDepth(depthRangeStart + norm * (1.0f - depthRangeStart), true);
    return;
  }
}

void ZenithPolySynthUI::mouseDrag(const juce::MouseEvent& e) {
  if (macroAssignDragActive_) {
    juce::ignoreUnused(e);
    return;
  }

  if (!macroDepthDragging_) {
    return;
  }
  const auto& target = macroDefs_[(size_t)activeMacroEditorIndex_]
                           .targets[(size_t)juce::jlimit(
                               0, juce::jmax(0, (int)macroDefs_[(size_t)activeMacroEditorIndex_].targets.size() - 1),
                               activeMacroTargetIndex_)];
  const float range = target.bipolar ? 2.0f : 1.0f;
  const float next = macroDepthDragStartValue_ +
                     (macroDepthDragStartY_ - (float)e.position.y) * (range / 120.0f);
  setSelectedMacroTargetDepth(next, true);
}

void ZenithPolySynthUI::mouseUp(const juce::MouseEvent& e) {
  if (macroAssignDragActive_) {
    for (int i = 0; i < (int)macroTargetRowBounds_.size(); ++i) {
      if (macroTargetRowBounds_[(size_t)i].contains(e.getPosition()) &&
          i < (int)macroDefs_[(size_t)activeMacroEditorIndex_].targets.size()) {
        activeMacroTargetIndex_ = i;
        break;
      }
    }

    if (pendingAssignParamId_.isNotEmpty()) {
      assignSelectedTargetToParamId(pendingAssignParamId_);
    }
    pendingAssignParamId_.clear();
    macroAssignDragActive_ = false;
    macroAssignArmed_ = false;
    repaint();
    return;
  }

  juce::ignoreUnused(e);
  macroDepthDragging_ = false;
}

void ZenithPolySynthUI::mouseMove(const juce::MouseEvent& e) {
  juce::ignoreUnused(e);
}

} // namespace zenith
