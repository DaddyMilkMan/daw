/*
  ==============================================================================

    ZenithPolySynthUI.cpp
    Created: 2025-11-27
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithPolySynthUI.h"

namespace zenith {

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), processor(p) {
  
  // Set initial size
  setSize(kSimpleWidth, kSimpleHeight);

  // Setup OpenGL
  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(true);
  openGLContext.setComponentPaintingEnabled(false);
  openGLContext.setMultisamplingEnabled(true);
  
  // Helper for setup
  auto setupControl = [&](ZenithControl* c, const juce::String& tip) {
      c->setTooltip(tip);
      c->onHoverStateChanged = [this](ZenithControl* ctrl) {
          if (isLearningMode_ && tooltipOverlay_) {
              tooltipOverlay_->setTarget(ctrl);
          }
      };
  };

  // 1. Source Controls (Blue)
  SkColor blue = SkColorSetRGB(0, 200, 255);
  subLevelKnob_ = std::make_unique<ZenithKnob>("SUB", blue);
  subLevelKnob_->setParameter(processor.parameters.getParameter("sub_level"));
  addChildComponent(subLevelKnob_.get());
  setupControl(subLevelKnob_.get(), "Sub Oscillator Level. Adds low-end weight.");

  noiseLevelKnob_ = std::make_unique<ZenithKnob>("NOISE", blue);
  noiseLevelKnob_->setParameter(processor.parameters.getParameter("noise_level"));
  addChildComponent(noiseLevelKnob_.get());
  setupControl(noiseLevelKnob_.get(), "Noise Level. Adds texture and grit.");

  // 2. Filter Controls (Amber)
  SkColor amber = SkColorSetRGB(255, 150, 0);
  cutoffKnob_ = std::make_unique<ZenithKnob>("CUTOFF", amber);
  cutoffKnob_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::FilterCutoff));
  addChildComponent(cutoffKnob_.get());
  setupControl(cutoffKnob_.get(), "Filter Cutoff. The most important knob. Controls brightness.");

  resKnob_ = std::make_unique<ZenithKnob>("RES", amber);
  resKnob_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::FilterResonance));
  addChildComponent(resKnob_.get());
  setupControl(resKnob_.get(), "Filter Resonance. Emphasizes the cutoff frequency.");

  envAmtKnob_ = std::make_unique<ZenithKnob>("ENV AMT", amber);
  envAmtKnob_->setParameter(processor.parameters.getParameter("filter_env_amount"));
  addChildComponent(envAmtKnob_.get());
  setupControl(envAmtKnob_.get(), "Filter Envelope Amount. How much the envelope affects the cutoff.");

  // 3. Amp Envelope (Pink)
  SkColor pink = SkColorSetRGB(255, 0, 150);
  ampAttackSlider_ = std::make_unique<ZenithSlider>("A", pink);
  ampAttackSlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpAttack));
  addAndMakeVisible(ampAttackSlider_.get());
  setupControl(ampAttackSlider_.get(), "Amp Attack. Time to reach full volume.");

  ampDecaySlider_ = std::make_unique<ZenithSlider>("D", pink);
  ampDecaySlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpDecay));
  addAndMakeVisible(ampDecaySlider_.get());
  setupControl(ampDecaySlider_.get(), "Amp Decay. Time to fall to sustain level.");

  ampSustainSlider_ = std::make_unique<ZenithSlider>("S", pink);
  ampSustainSlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpSustain));
  addAndMakeVisible(ampSustainSlider_.get());
  setupControl(ampSustainSlider_.get(), "Amp Sustain. Level while holding the key.");

  ampReleaseSlider_ = std::make_unique<ZenithSlider>("R", pink);
  ampReleaseSlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpRelease));
  addAndMakeVisible(ampReleaseSlider_.get());
  setupControl(ampReleaseSlider_.get(), "Amp Release. Time to fade out after releasing key.");

  // 5. Visualizer
  visualizer_ = std::make_unique<ZenithVisualizer>(processor);
  addAndMakeVisible(visualizer_.get());

  // 6. Advanced Controls (LFOs - Purple)
  SkColor purple = SkColorSetRGB(200, 100, 255);
  lfo1RateKnob_ = std::make_unique<ZenithKnob>("LFO1 RATE", purple);
  lfo1RateKnob_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::LFO1Rate));
  addChildComponent(lfo1RateKnob_.get());
  setupControl(lfo1RateKnob_.get(), "LFO 1 Rate. Speed of the first Low Frequency Oscillator.");

  lfo1AmountKnob_ = std::make_unique<ZenithKnob>("LFO1 AMT", purple);
  lfo1AmountKnob_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::LFO1Amount));
  addChildComponent(lfo1AmountKnob_.get());
  setupControl(lfo1AmountKnob_.get(), "LFO 1 Amount. Depth of modulation.");

  lfo2RateKnob_ = std::make_unique<ZenithKnob>("LFO2 RATE", purple);
  lfo2RateKnob_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::LFO2Rate));
  addChildComponent(lfo2RateKnob_.get());
  setupControl(lfo2RateKnob_.get(), "LFO 2 Rate. Speed of the second Low Frequency Oscillator.");

  lfo2AmountKnob_ = std::make_unique<ZenithKnob>("LFO2 AMT", purple);
  lfo2AmountKnob_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::LFO2Amount));
  addChildComponent(lfo2AmountKnob_.get());
  setupControl(lfo2AmountKnob_.get(), "LFO 2 Amount. Depth of modulation.");

  // Filter 2 (Amber)
  filter2CutoffKnob_ = std::make_unique<ZenithKnob>("F2 CUTOFF", amber);
  filter2CutoffKnob_->setParameter(processor.parameters.getParameter("filter2_cutoff"));
  addChildComponent(filter2CutoffKnob_.get());
  setupControl(filter2CutoffKnob_.get(), "Filter 2 Cutoff. Controls the brightness of the second filter.");

  filter2ResKnob_ = std::make_unique<ZenithKnob>("F2 RES", amber);
  filter2ResKnob_->setParameter(processor.parameters.getParameter("filter2_resonance"));
  addChildComponent(filter2ResKnob_.get());
  setupControl(filter2ResKnob_.get(), "Filter 2 Resonance. Emphasizes the cutoff frequency.");

  filter2DriveKnob_ = std::make_unique<ZenithKnob>("F2 DRIVE", amber);
  filter2DriveKnob_->setParameter(processor.parameters.getParameter("filter2_drive"));
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
}

void ZenithPolySynthUI::renderOpenGL() {
#ifdef ZENITH_USE_SKIA
  // 1. Initialize Skia if needed
  if (!rendererInitialized_) {
    auto glInterface = GrGLMakeNativeInterface();
    if (glInterface) {
      grContext_ = GrDirectContexts::MakeGL(glInterface);
      if (grContext_) {
        rendererInitialized_ = true;
      }
    }
  }

  if (!grContext_) return;

  // 2. Create Surface
  int fbWidth = getWidth(); // Note: might need scaling for HiDPI
  int fbHeight = getHeight();
  
  // Handle HiDPI
  double scale = openGLContext.getRenderingScale();
  fbWidth = juce::roundToInt(fbWidth * scale);
  fbHeight = juce::roundToInt(fbHeight * scale);

  if (fbWidth <= 0 || fbHeight <= 0) return;

  GrGLFramebufferInfo fbInfo;
  fbInfo.fFBOID = 0;
  fbInfo.fFormat = 0x8058; // GL_RGBA8

  GrBackendRenderTarget backendRT =
      GrBackendRenderTargets::MakeGL(fbWidth, fbHeight, 1, 8, fbInfo);

  SkSurfaceProps props(0, kRGB_H_SkPixelGeometry);
  sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(
      grContext_.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, nullptr, &props);

  if (!surface) return;

  SkCanvas *canvas = surface->getCanvas();
  if (!canvas) return;

  // 3. Draw
  // Scale canvas to match component coordinates
  canvas->save();
  canvas->scale((float)scale, (float)scale);

  // Clear background
  canvas->clear(SkColorSetRGB(20, 20, 25)); // Dark background

  // Draw UI
  drawBackground(canvas);
  
  // Render children
  for (auto *child : getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();

  // 4. Flush
  grContext_->flush();
#else
  // Fallback if Skia not enabled
  juce::OpenGLHelpers::clear(juce::Colours::black);
#endif
}

void ZenithPolySynthUI::renderComponentRecursively(juce::Component *comp, SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (!comp->isVisible()) return;

  canvas->save();
  
  auto bounds = comp->getBounds();
  canvas->translate((SkScalar)bounds.getX(), (SkScalar)bounds.getY());
  canvas->clipRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));

  if (auto *skiaComp = dynamic_cast<SkiaComponent *>(comp)) {
    skiaComp->drawSkia(canvas);
  }

  for (auto *child : comp->getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();
#endif
}

void ZenithPolySynthUI::drawBackground(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  // Draw a subtle gradient background (Nebula effect)
  SkPaint paint;
  
  // Gradient from top-left (Dark Blue) to bottom-right (Dark Purple)
  SkPoint points[2] = {SkPoint::Make(0, 0), SkPoint::Make((float)getWidth(), (float)getHeight())};
  SkColor colors[2] = {SkColorSetRGB(10, 15, 30), SkColorSetRGB(20, 10, 25)};
  
  paint.setShader(SkGradientShader::MakeLinear(points, colors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawPaint(paint);

  // Draw "Glass" panel
  drawGlassPanel(canvas, getLocalBounds().reduced(20));
#endif
}

void ZenithPolySynthUI::drawGlassPanel(SkCanvas *canvas, const juce::Rectangle<int> &bounds) {
#ifdef ZENITH_USE_SKIA
  SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
  SkRRect rrect = SkRRect::MakeRectXY(rect, 16.0f, 16.0f); // 16px rounded corners

  SkPaint paint;
  
  // 1. Semi-transparent fill
  paint.setColor(SkColorSetARGB(30, 255, 255, 255)); // 12% white
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRRect(rrect, paint);

  // 2. Border/Stroke (Glow effect)
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(50, 255, 255, 255)); // 20% white border
  canvas->drawRRect(rrect, paint);

  // 3. Subtle inner glow (simulated with another stroke)
  // (Simplified for now)
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
  
  // Top: Visualizer
  auto topArea = area.removeFromTop(area.getHeight() * 0.4f);
  if (visualizer_) visualizer_->setBounds(topArea.reduced(10));
  
  // Bottom: Controls
  auto bottomArea = area;
  
  // If Advanced Mode, we have more space at the bottom?
  // No, the window size changes.
  // In Advanced Mode (800x600), the top area is larger, but we want to keep the simple controls in a similar relative position?
  // Let's assume the Simple Mode controls stay in the upper part of the bottom area, and Advanced controls appear below them.
    Created: 2025-11-27
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithPolySynthUI.h"

namespace zenith {

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), processor(p) {
  
  // Set initial size
  setSize(kSimpleWidth, kSimpleHeight);

  // Setup OpenGL
  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(true);
  openGLContext.setComponentPaintingEnabled(false);
  openGLContext.setMultisamplingEnabled(true);
  

  // 3. Amp Envelope (Pink)
  SkColor pink = SkColorSetRGB(255, 0, 150);
  ampAttackSlider_ = std::make_unique<ZenithSlider>("A", pink);
  ampAttackSlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpAttack));
  addAndMakeVisible(ampAttackSlider_.get());

  ampDecaySlider_ = std::make_unique<ZenithSlider>("D", pink);
  ampDecaySlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpDecay));
  addAndMakeVisible(ampDecaySlider_.get());

  ampSustainSlider_ = std::make_unique<ZenithSlider>("S", pink);
  ampSustainSlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpSustain));
  addAndMakeVisible(ampSustainSlider_.get());

  ampReleaseSlider_ = std::make_unique<ZenithSlider>("R", pink);
  ampReleaseSlider_->setParameter(processor.parameters.getParameter(ZenithPolySynthProcessor::AmpRelease));
  grContext_.reset();
#endif
}

void ZenithPolySynthUI::renderOpenGL() {
#ifdef ZENITH_USE_SKIA
  // 1. Initialize Skia if needed
  if (!rendererInitialized_) {
    auto glInterface = GrGLMakeNativeInterface();
    if (glInterface) {
      grContext_ = GrDirectContexts::MakeGL(glInterface);
      if (grContext_) {
        rendererInitialized_ = true;
      }
    }
  }

  if (!grContext_) return;

  // 2. Create Surface
  int fbWidth = getWidth(); // Note: might need scaling for HiDPI
  int fbHeight = getHeight();
  
  // Handle HiDPI
  double scale = openGLContext.getRenderingScale();
  fbWidth = juce::roundToInt(fbWidth * scale);
  fbHeight = juce::roundToInt(fbHeight * scale);

  if (fbWidth <= 0 || fbHeight <= 0) return;

  GrGLFramebufferInfo fbInfo;
  fbInfo.fFBOID = 0;
  fbInfo.fFormat = 0x8058; // GL_RGBA8

  GrBackendRenderTarget backendRT =
      GrBackendRenderTargets::MakeGL(fbWidth, fbHeight, 1, 8, fbInfo);

  SkSurfaceProps props(0, kRGB_H_SkPixelGeometry);
  sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(
      grContext_.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, nullptr, &props);

  if (!surface) return;

  SkCanvas *canvas = surface->getCanvas();
  if (!canvas) return;

  // 3. Draw
  // Scale canvas to match component coordinates
  canvas->save();
  canvas->scale((float)scale, (float)scale);

  // Clear background
  canvas->clear(SkColorSetRGB(20, 20, 25)); // Dark background

  // Draw UI
  drawBackground(canvas);
  
  // Render children
  for (auto *child : getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();

  // 4. Flush
  grContext_->flush();
#else
  // Fallback if Skia not enabled
  juce::OpenGLHelpers::clear(juce::Colours::black);
#endif
}

void ZenithPolySynthUI::renderComponentRecursively(juce::Component *comp, SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (!comp->isVisible()) return;

  canvas->save();
  
  auto bounds = comp->getBounds();
  canvas->translate((SkScalar)bounds.getX(), (SkScalar)bounds.getY());
  canvas->clipRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));

  if (auto *skiaComp = dynamic_cast<SkiaComponent *>(comp)) {
    skiaComp->drawSkia(canvas);
  }

  for (auto *child : comp->getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();
#endif
}

void ZenithPolySynthUI::drawBackground(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  // Draw a subtle gradient background (Nebula effect)
  SkPaint paint;
  
  // Gradient from top-left (Dark Blue) to bottom-right (Dark Purple)
  SkPoint points[2] = {SkPoint::Make(0, 0), SkPoint::Make((float)getWidth(), (float)getHeight())};
  SkColor colors[2] = {SkColorSetRGB(10, 15, 30), SkColorSetRGB(20, 10, 25)};
  
  paint.setShader(SkGradientShader::MakeLinear(points, colors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawPaint(paint);

  // Draw "Glass" panel
  drawGlassPanel(canvas, getLocalBounds().reduced(20));
#endif
}

void ZenithPolySynthUI::drawGlassPanel(SkCanvas *canvas, const juce::Rectangle<int> &bounds) {
#ifdef ZENITH_USE_SKIA
  SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
  SkRRect rrect = SkRRect::MakeRectXY(rect, 16.0f, 16.0f); // 16px rounded corners

  SkPaint paint;
  
  // 1. Semi-transparent fill
  paint.setColor(SkColorSetARGB(30, 255, 255, 255)); // 12% white
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRRect(rrect, paint);

  // 2. Border/Stroke (Glow effect)
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(50, 255, 255, 255)); // 20% white border
  canvas->drawRRect(rrect, paint);

  // 3. Subtle inner glow (simulated with another stroke)
  // (Simplified for now)
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
  
  // Top: Visualizer
  auto topArea = area.removeFromTop(area.getHeight() * 0.4f);
  if (visualizer_) visualizer_->setBounds(topArea.reduced(10));
  
  // Bottom: Controls
  auto bottomArea = area;
  
  // If Advanced Mode, we have more space at the bottom?
  // No, the window size changes.
  // In Advanced Mode (800x600), the top area is larger, but we want to keep the simple controls in a similar relative position?
  // Let's assume the Simple Mode controls stay in the upper part of the bottom area, and Advanced controls appear below them.

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
  }

  // 1. Left: Source (20%)
  auto sourceArea = bottomArea.removeFromLeft(bottomArea.getWidth() * 0.25f);
  subLevelKnob_->setBounds(sourceArea.removeFromTop(sourceArea.getHeight() / 2).reduced(10));
  noiseLevelKnob_->setBounds(sourceArea.reduced(10));

  // 3. Right: Envelopes (25%)
  auto envArea = bottomArea.removeFromRight(bottomArea.getWidth() * 0.33f);
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
  
  // Res and Env Amt below/sides?
  // For now, let's put them in corners of filter area
  resKnob_->setBounds(filterArea.getX(), filterArea.getBottom() - 60, 60, 60);
  envAmtKnob_->setBounds(filterArea.getRight() - 60, filterArea.getBottom() - 60, 60, 60);

  // Buttons
  expandButton_->setBounds(getWidth() - 100, getHeight() - (isAdvancedMode_ ? 240 : 40), 80, 30);
  learningModeButton_->setBounds(getWidth() - 100, 10, 80, 30);
  
  // Overlay
  if (tooltipOverlay_) tooltipOverlay_->setBounds(getLocalBounds());
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
}

void ZenithPolySynthUI::toggleLearningMode() {
    isLearningMode_ = !isLearningMode_;
    if (tooltipOverlay_) {
        tooltipOverlay_->setVisible(isLearningMode_);
        if (!isLearningMode_) tooltipOverlay_->setTarget(nullptr);
    }
}

} // namespace zenith
