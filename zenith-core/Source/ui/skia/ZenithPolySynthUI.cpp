/*
  ==============================================================================

    ZenithPolySynthUI.cpp
    Created: 2025-11-27
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithPolySynthUI.h"



// Include all Skia headers BEFORE entering namespace zenith
#ifdef ZENITH_USE_SKIA
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkSurface.h>
#include <skia/include/core/SkRefCnt.h>
#include <skia/include/core/SkPoint.h>
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), 
      SkiaRenderer(this), // Initialize SkiaRenderer
      processor(p) {
  
  // Set initial size
  setSize(kSimpleWidth, kSimpleHeight);

  // Note: OpenGL setup is now handled by SkiaRenderer base class
  
#ifdef ZENITH_USE_SKIA
  // Colors (uint32_t 0xAARRGGBB)
  uint32_t pink = 0xFFFF0096; // SkColorSetRGB(255, 0, 150)
  uint32_t purple = 0xFFC864FF; // SkColorSetRGB(200, 100, 255)
  uint32_t amber = 0xFFFFC800; // SkColorSetRGB(255, 200, 0)

  // 1. Cutoff Knob (Big - Cyan default)
  cutoffKnob_ = std::make_unique<ZenithKnob>("CUTOFF");
  cutoffKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::FilterCutoff));
  addAndMakeVisible(cutoffKnob_.get());
  setupControl(cutoffKnob_.get(), "Filter Cutoff. Controls the brightness of the sound.");

  // 2. Resonance Knob
  resKnob_ = std::make_unique<ZenithKnob>("RES");
  resKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::FilterResonance));
  addAndMakeVisible(resKnob_.get());
  setupControl(resKnob_.get(), "Filter Resonance. Emphasizes the cutoff frequency.");

  // Env Amount Knob
  envAmtKnob_ = std::make_unique<ZenithKnob>("ENV AMT");
  envAmtKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::FilterEnvAmount));
  addAndMakeVisible(envAmtKnob_.get());
  setupControl(envAmtKnob_.get(), "Filter Envelope Amount. How much the envelope affects cutoff.");

  // Sub Level
  subLevelKnob_ = std::make_unique<ZenithKnob>("SUB");
  subLevelKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::SubOscLevel));
  addAndMakeVisible(subLevelKnob_.get());
  setupControl(subLevelKnob_.get(), "Sub Oscillator Level. Adds low-end weight.");

  // Noise Level
  noiseLevelKnob_ = std::make_unique<ZenithKnob>("NOISE");
  noiseLevelKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::NoiseLevel));
  addAndMakeVisible(noiseLevelKnob_.get());
  setupControl(noiseLevelKnob_.get(), "Noise Level. Adds texture and grit.");

  // 3. Amp Envelope (Pink)
  ampAttackSlider_ = std::make_unique<ZenithSlider>("A", pink);
  ampAttackSlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::AmpAttack));
  addAndMakeVisible(ampAttackSlider_.get());
  setupControl(ampAttackSlider_.get(), "Amp Attack. Time to reach full volume.");

  ampDecaySlider_ = std::make_unique<ZenithSlider>("D", pink);
  ampDecaySlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::AmpDecay));
  addAndMakeVisible(ampDecaySlider_.get());
  setupControl(ampDecaySlider_.get(), "Amp Decay. Time to fall to sustain level.");

  ampSustainSlider_ = std::make_unique<ZenithSlider>("S", pink);
  ampSustainSlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::AmpSustain));
  addAndMakeVisible(ampSustainSlider_.get());
  setupControl(ampSustainSlider_.get(), "Amp Sustain. Level while holding the key.");

  ampReleaseSlider_ = std::make_unique<ZenithSlider>("R", pink);
  ampReleaseSlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::AmpRelease));
  addAndMakeVisible(ampReleaseSlider_.get());
  setupControl(ampReleaseSlider_.get(), "Amp Release. Time to fade out after releasing key.");

  // 5. Visualizer
  visualizer_ = std::make_unique<ZenithVisualizer>(processor);
  addAndMakeVisible(visualizer_.get());

  // 6. Advanced Controls (LFOs - Purple)
  lfo1RateKnob_ = std::make_unique<ZenithKnob>("LFO1 RATE", purple);
  lfo1RateKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::LFO1Rate));
  addChildComponent(lfo1RateKnob_.get());
  setupControl(lfo1RateKnob_.get(), "LFO 1 Rate. Speed of the first Low Frequency Oscillator.");

  lfo1AmountKnob_ = std::make_unique<ZenithKnob>("LFO1 AMT", purple);
  lfo1AmountKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::LFO1Amount));
  addChildComponent(lfo1AmountKnob_.get());
  setupControl(lfo1AmountKnob_.get(), "LFO 1 Amount. Depth of modulation.");

  lfo2RateKnob_ = std::make_unique<ZenithKnob>("LFO2 RATE", purple);
  lfo2RateKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::LFO2Rate));
  addChildComponent(lfo2RateKnob_.get());
  setupControl(lfo2RateKnob_.get(), "LFO 2 Rate. Speed of the second Low Frequency Oscillator.");

  lfo2AmountKnob_ = std::make_unique<ZenithKnob>("LFO2 AMT", purple);
  lfo2AmountKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::LFO2Amount));
  addChildComponent(lfo2AmountKnob_.get());
  setupControl(lfo2AmountKnob_.get(), "LFO 2 Amount. Depth of modulation.");

  // Filter 2 (Amber)
  filter2CutoffKnob_ = std::make_unique<ZenithKnob>("F2 CUTOFF", amber);
  filter2CutoffKnob_->setParameter(processor.getParameters().getParameter("filter2_cutoff"));
  addChildComponent(filter2CutoffKnob_.get());
  setupControl(filter2CutoffKnob_.get(), "Filter 2 Cutoff. Controls the brightness of the second filter.");

  filter2ResKnob_ = std::make_unique<ZenithKnob>("F2 RES", amber);
  filter2ResKnob_->setParameter(processor.getParameters().getParameter("filter2_resonance"));
  addChildComponent(filter2ResKnob_.get());
  setupControl(filter2ResKnob_.get(), "Filter 2 Resonance. Emphasizes the cutoff frequency.");

  filter2DriveKnob_ = std::make_unique<ZenithKnob>("F2 DRIVE", amber);
  filter2DriveKnob_->setParameter(processor.getParameters().getParameter("filter2_drive"));
  addChildComponent(filter2DriveKnob_.get());
  setupControl(filter2DriveKnob_.get(), "Filter 2 Drive. Adds saturation and warmth.");

  // 7. Modulation Matrix
  modMatrix_ = std::make_unique<ZenithModMatrix>(processor);
  addChildComponent(modMatrix_.get());

  // 4. Expand Button
  expandButton_ = std::make_unique<ZenithButton>("EXPAND", [this]() { toggleAdvancedMode(); });
  addAndMakeVisible(expandButton_.get());
  
  // Learning Mode Button
  learningModeButton_ = std::make_unique<ZenithButton>("LEARN", [this]() { toggleLearningMode(); });
  addAndMakeVisible(learningModeButton_.get());

  // Overlay
  tooltipOverlay_ = std::make_unique<ZenithTooltipOverlay>();
  addChildComponent(tooltipOverlay_.get());
  tooltipOverlay_->setVisible(false);

  // Preset Bar
  presetBar_ = std::make_unique<ZenithPresetBar>();
  presetBar_->setCallbacks(
      [this]() { loadPrevPreset(); },
      [this]() { loadNextPreset(); },
      []() {} // Menu not implemented yet
  );
  addAndMakeVisible(presetBar_.get());

  // Filter Envelope (Advanced - Pink)
  modAttackSlider_ = std::make_unique<ZenithSlider>("F.A", pink);
  modAttackSlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::ModAttack));
  addChildComponent(modAttackSlider_.get());
  setupControl(modAttackSlider_.get(), "Filter Env Attack. Time to reach full modulation.");

  modDecaySlider_ = std::make_unique<ZenithSlider>("F.D", pink);
  modDecaySlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::ModDecay));
  addChildComponent(modDecaySlider_.get());
  setupControl(modDecaySlider_.get(), "Filter Env Decay. Time to fall to sustain.");

  modSustainSlider_ = std::make_unique<ZenithSlider>("F.S", pink);
  modSustainSlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::ModSustain));
  addChildComponent(modSustainSlider_.get());
  setupControl(modSustainSlider_.get(), "Filter Env Sustain. Level while holding key.");

  modReleaseSlider_ = std::make_unique<ZenithSlider>("F.R", pink);
  modReleaseSlider_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::ModRelease));
  addChildComponent(modReleaseSlider_.get());
  setupControl(modReleaseSlider_.get(), "Filter Env Release. Time to fade out.");

  // Load Presets
  refreshPresetList();
  
  // Set LookAndFeel
  setLookAndFeel(&zenithLookAndFeel_);
#endif
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
  setLookAndFeel(nullptr);
}

void ZenithPolySynthUI::drawSkiaContent(SkCanvas* canvas) {
#ifdef ZENITH_USE_SKIA
  if (!canvas) return;

  // Scale canvas to match component coordinates
  // Note: SkiaRenderer handles the surface creation, but we might need to handle scaling
  // if the surface was created with physical pixels and we want logical coordinates.
  // JUCE's getRenderingScale() is useful here.
  
  auto scale = static_cast<float>(openGLContext_.getRenderingScale());
  canvas->save();
  canvas->scale(scale, scale);

  // Clear background (optional, SkiaRenderer does it, but we can do it again with our color)
  // canvas->clear(0xFF141419); 

  // Draw UI
  drawBackground(canvas);
  
  // Render children
  for (auto *child : getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();
#endif
}

void ZenithPolySynthUI::renderComponentRecursively(juce::Component *comp, SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (!comp->isVisible()) return;

  canvas->save();
  
  auto bounds = comp->getBounds();
  canvas->translate((SkScalar)bounds.getX(), (SkScalar)bounds.getY());
  
  // Clip to bounds
  canvas->clipRect(SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()));

  // Draw if it's a Skia component
  if (auto *skiaComp = dynamic_cast<SkiaComponent *>(comp)) {
    skiaComp->drawSkia(canvas);
  }

  // Recursively draw children
  for (auto *child : comp->getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();
#endif
}

void ZenithPolySynthUI::drawBackground(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  auto bounds = getLocalBounds().toFloat();
  SkPaint paint;
  paint.setAntiAlias(true);

  // Premium Radial Gradient Background
  SkPoint center = { bounds.centerX(), bounds.centerY() };
  SkColor colors[2] = { SkColorSetRGB(30, 30, 40), SkColorSetRGB(10, 10, 15) }; // Lighter center -> Dark edges
  float radius = std::max(bounds.width(), bounds.height()) * 0.8f;
  
  paint.setShader(SkGradientShader::MakeRadial(center, radius, colors, nullptr, 2, SkTileMode::kClamp));
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRect(SkRect::MakeWH(bounds.width(), bounds.height()), paint);
  paint.setShader(nullptr);

  // Subtle Noise/Texture (Optional, simulated with dots)
  // For now, just the gradient is a huge improvement over flat grey.
#else
  juce::ignoreUnused(canvas);
#endif
}

void ZenithPolySynthUI::drawGlassPanel(SkCanvas *canvas, const juce::Rectangle<int> &bounds) {
#ifdef ZENITH_USE_SKIA
  SkRect rect = SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(), (float)bounds.getWidth(), (float)bounds.getHeight());
  SkRRect rrect = SkRRect::MakeRectXY(rect, 16.0f, 16.0f); // 16px rounded corners

  SkPaint paint;
  
  // 1. Semi-transparent fill
  paint.setColor(0x1EFFFFFF); // SkColorSetARGB(30, 255, 255, 255)
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRRect(rrect, paint);

  // 2. Border/Stroke (Glow effect)
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(0x32FFFFFF); // SkColorSetARGB(50, 255, 255, 255)
  canvas->drawRRect(rrect, paint);
#else
  juce::ignoreUnused(canvas, bounds);
#endif
}

void ZenithPolySynthUI::paint(juce::Graphics &g) {
  // Fallback
  g.fillAll(juce::Colours::black);
  g.setColour(juce::Colours::white);
  g.drawText("Initializing OpenGL...", getLocalBounds(), juce::Justification::centred);
}

void ZenithPolySynthUI::resized() {
  auto area = getLocalBounds().reduced(40); // Inside glass panel
  
  // Top Bar: Presets
  auto topBar = area.removeFromTop(40);
  if (presetBar_) presetBar_->setBounds(topBar.reduced(100, 0)); // Centered with margin

  // Top: Visualizer
  auto topArea = area.removeFromTop(static_cast<int>(area.getHeight() * 0.4f));
  if (visualizer_) visualizer_->setBounds(topArea.reduced(10));
  
  // Bottom: Controls
  auto bottomArea = area;
  
  if (isAdvancedMode_) {
      // Reserve bottom 200px for Advanced Controls
      auto advancedArea = bottomArea.removeFromBottom(200);
      
      // LFOs (Left)
      int knobSize = 60;
      int x = advancedArea.getX() + 20;
      int y = advancedArea.getY() + 20;
      
      lfo1RateKnob_->setBounds(x, y, knobSize, knobSize);
      lfo1AmountKnob_->setBounds(x + 80, y, knobSize, knobSize);
      
      lfo2RateKnob_->setBounds(x, y + 80, knobSize, knobSize);
      lfo2AmountKnob_->setBounds(x + 80, y + 80, knobSize, knobSize);
      
      // Filter 2 (Middle-Left)
      x += 180;
      filter2CutoffKnob_->setBounds(x, y, knobSize, knobSize);
      filter2ResKnob_->setBounds(x + 80, y, knobSize, knobSize);
      filter2DriveKnob_->setBounds(x, y + 80, knobSize, knobSize);

      // Matrix (Right)
      int matrixX = x + 180;
      int matrixW = advancedArea.getRight() - matrixX - 20;
      int matrixH = advancedArea.getHeight() - 40;
      if (modMatrix_) modMatrix_->setBounds(matrixX, y, matrixW, matrixH);

      // Filter Envelope (Between Filter 2 and Matrix?)
      int envX = x + 80 + knobSize + 20; 
      int envY = y;
      int envW = 100;
      int envH = 160;
      
      // Adjust Matrix to start after Env
      matrixX = envX + envW + 20;
      matrixW = advancedArea.getRight() - matrixX - 20;
      if (modMatrix_) modMatrix_->setBounds(matrixX, y, matrixW, matrixH);

      int sliderW = envW / 4;
      modAttackSlider_->setBounds(envX, envY, sliderW, envH);
      modDecaySlider_->setBounds(envX + sliderW, envY, sliderW, envH);
      modSustainSlider_->setBounds(envX + sliderW*2, envY, sliderW, envH);
      modReleaseSlider_->setBounds(envX + sliderW*3, envY, sliderW, envH);
  }

  // 1. Left: Source (20%)
  auto sourceArea = bottomArea.removeFromLeft(static_cast<int>(bottomArea.getWidth() * 0.25f));
  subLevelKnob_->setBounds(sourceArea.removeFromTop(sourceArea.getHeight() / 2).reduced(10));
  noiseLevelKnob_->setBounds(sourceArea.reduced(10));

  // 3. Right: Envelopes (25%)
  auto envArea = bottomArea.removeFromRight(static_cast<int>(bottomArea.getWidth() * 0.33f));
  int sliderWidth = envArea.getWidth() / 4;
  ampAttackSlider_->setBounds(envArea.removeFromLeft(sliderWidth).reduced(5));
  ampDecaySlider_->setBounds(envArea.removeFromLeft(sliderWidth).reduced(5));
  ampSustainSlider_->setBounds(envArea.removeFromLeft(sliderWidth).reduced(5));
  ampReleaseSlider_->setBounds(envArea.reduced(5));

  // 2. Center: Filter (Remaining)
  auto filterArea = bottomArea;
  // Big Cutoff in center
  int cutoffSize = std::min(filterArea.getWidth(), filterArea.getHeight()) - 40;
  cutoffKnob_->setBounds(filterArea.getCentreX() - cutoffSize/2, 
                         filterArea.getCentreY() - cutoffSize/2, 
                         cutoffSize, cutoffSize);
  
  resKnob_->setBounds(filterArea.getX(), filterArea.getBottom() - 60, 60, 60);
  envAmtKnob_->setBounds(filterArea.getRight() - 60, filterArea.getBottom() - 60, 60, 60);

  // Buttons
  int btnWidth = 120;
  int btnHeight = 40;
  expandButton_->setBounds(getWidth() - btnWidth - 20, getHeight() - (isAdvancedMode_ ? 240 : 60), btnWidth, btnHeight);
  
  learningModeButton_->setBounds(getWidth() - 100, 10, 80, 30);
  
  // Overlay
  if (tooltipOverlay_) tooltipOverlay_->setBounds(getLocalBounds());
}

void ZenithPolySynthUI::setupControl(ZenithControl* control, const juce::String& tooltip) {
    control->setTooltip(tooltip);
    control->onHoverStateChanged = [this](ZenithControl* c) {
        if (isLearningMode_ && tooltipOverlay_) {
            tooltipOverlay_->setTarget(c);
            tooltipOverlay_->setVisible(c != nullptr);
        }
    };
}

void ZenithPolySynthUI::toggleAdvancedMode() {
    isAdvancedMode_ = !isAdvancedMode_;
    setSize(isAdvancedMode_ ? kAdvancedWidth : kSimpleWidth,
            isAdvancedMode_ ? kAdvancedHeight : kSimpleHeight);
    
    // Toggle visibility
    if (lfo1RateKnob_) lfo1RateKnob_->setVisible(isAdvancedMode_);
    if (lfo1AmountKnob_) lfo1AmountKnob_->setVisible(isAdvancedMode_);
    if (lfo2RateKnob_) lfo2RateKnob_->setVisible(isAdvancedMode_);
    if (lfo2AmountKnob_) lfo2AmountKnob_->setVisible(isAdvancedMode_);
    if (filter2CutoffKnob_) filter2CutoffKnob_->setVisible(isAdvancedMode_);
    if (filter2ResKnob_) filter2ResKnob_->setVisible(isAdvancedMode_);
    if (filter2DriveKnob_) filter2DriveKnob_->setVisible(isAdvancedMode_);
    if (modMatrix_) modMatrix_->setVisible(isAdvancedMode_);
    
    if (modAttackSlider_) modAttackSlider_->setVisible(isAdvancedMode_);
    if (modDecaySlider_) modDecaySlider_->setVisible(isAdvancedMode_);
    if (modSustainSlider_) modSustainSlider_->setVisible(isAdvancedMode_);
    if (modReleaseSlider_) modReleaseSlider_->setVisible(isAdvancedMode_);
}

void ZenithPolySynthUI::toggleLearningMode() {
    isLearningMode_ = !isLearningMode_;
    if (tooltipOverlay_) {
        tooltipOverlay_->setVisible(isLearningMode_);
        if (!isLearningMode_) tooltipOverlay_->setTarget(nullptr);
    }
}

void ZenithPolySynthUI::refreshPresetList() {
    presetList_ = ZenithPresetManager::getInstance().getPresetList("ZenithPolySynth");
    if (presetList_.empty()) {
        if (presetBar_) presetBar_->setPresetName("No Presets Found");
    } else {
        currentPresetIndex_ = 0;
        loadPreset(0);
    }
}

void ZenithPolySynthUI::loadPreset(int index) {
    if (index < 0 || index >= static_cast<int>(presetList_.size())) return;
    
    currentPresetIndex_ = index;
    auto& meta = presetList_[index];
    
    // Update UI
    if (presetBar_) presetBar_->setPresetName(meta.name);
    
    // Load Data
    auto preset = ZenithPresetManager::getInstance().loadPreset("ZenithPolySynth", meta.id);
    
    // Apply to Processor Parameters
    auto& params = processor.getParameters();
    for (auto const& pair : preset.parameters) {
        auto paramId = pair.first;
        auto paramValue = pair.second;
        if (auto* param = params.getParameter(paramId)) {
            // Value is normalized [0-1]
            auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param);
            if (rangedParam) {
                rangedParam->beginChangeGesture();
                rangedParam->setValueNotifyingHost(paramValue);
                rangedParam->endChangeGesture();
            }
        }
    }
}

void ZenithPolySynthUI::loadNextPreset() {
    if (presetList_.empty()) return;
    int next = (currentPresetIndex_ + 1) % static_cast<int>(presetList_.size());
    loadPreset(next);
}

void ZenithPolySynthUI::loadPrevPreset() {
    if (presetList_.empty()) return;
    int size = static_cast<int>(presetList_.size());
    int prev = (currentPresetIndex_ - 1 + size) % size;
    loadPreset(prev);
}

} // namespace zenith
