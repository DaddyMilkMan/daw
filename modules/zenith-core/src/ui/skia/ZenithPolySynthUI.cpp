/*
  ==============================================================================

    ZenithPolySynthUI.cpp
    Created: 2025-11-27
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithPolySynthUI.h"

// Skia headers (conditional)
#ifdef ZENITH_USE_SKIA
#include <include/core/SkSurface.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkRRect.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/effects/SkGradientShader.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

namespace zenith {

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), processor(p) {
  
  // Set initial size
  setSize(kSimpleWidth, kSimpleHeight);

  // Setup OpenGL Context for Skia
  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(true);
  openGLContext.setComponentPaintingEnabled(false); // Disable JUCE painting
  openGLContext.attachTo(*this);

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
#ifdef ZENITH_ENABLE_SKIA
  SkColor blue = SkColorSetRGB(0, 200, 255);
#else
  uint32_t blue = 0xFF00C8FF;
#endif
  subLevelKnob_ = std::make_unique<ZenithKnob>("SUB", blue);
  subLevelKnob_->setParameter(processor.getParameters().getParameter("sub_level"));
  addChildComponent(subLevelKnob_.get());
  setupControl(subLevelKnob_.get(), "Sub Oscillator Level. Adds low-end weight.");

  noiseLevelKnob_ = std::make_unique<ZenithKnob>("NOISE", blue);
  noiseLevelKnob_->setParameter(processor.getParameters().getParameter("noise_level"));
  addChildComponent(noiseLevelKnob_.get());
  setupControl(noiseLevelKnob_.get(), "Noise Level. Adds texture and grit.");

  // 2. Filter Controls (Amber)
#ifdef ZENITH_ENABLE_SKIA
  SkColor amber = SkColorSetRGB(255, 150, 0);
#else
  uint32_t amber = 0xFFFF9600;
#endif
  cutoffKnob_ = std::make_unique<ZenithKnob>("CUTOFF", amber);
  cutoffKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::FilterCutoff));
  addChildComponent(cutoffKnob_.get());
  setupControl(cutoffKnob_.get(), "Filter Cutoff. The most important knob. Controls brightness.");

  resKnob_ = std::make_unique<ZenithKnob>("RES", amber);
  resKnob_->setParameter(processor.getParameters().getParameter(ZenithPolySynthProcessor::FilterResonance));
  addChildComponent(resKnob_.get());
  setupControl(resKnob_.get(), "Filter Resonance. Emphasizes the cutoff frequency.");

  envAmtKnob_ = std::make_unique<ZenithKnob>("ENV AMT", amber);
  envAmtKnob_->setParameter(processor.getParameters().getParameter("filter_env_amount"));
  addChildComponent(envAmtKnob_.get());
  setupControl(envAmtKnob_.get(), "Filter Envelope Amount. How much the envelope affects the cutoff.");

  // 3. Amp Envelope (Pink)
#ifdef ZENITH_ENABLE_SKIA
  SkColor pink = SkColorSetRGB(255, 0, 150);
#else
  uint32_t pink = 0xFFFF0096;
#endif
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
#ifdef ZENITH_ENABLE_SKIA
  SkColor purple = SkColorSetRGB(200, 100, 255);
#else
  uint32_t purple = 0xFFC864FF;
#endif
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
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
    openGLContext.detach();
}

//==============================================================================
// OpenGL / Skia Lifecycle
//==============================================================================

void ZenithPolySynthUI::newOpenGLContextCreated() {
#ifdef ZENITH_USE_SKIA
    auto glInterface = GrGLMakeNativeInterface();
    auto ctx = GrDirectContexts::MakeGL(glInterface);
    if (grContext_) grContext_->unref();
    grContext_ = ctx.release();
    
    if (!grContext_) {
        DBG("Failed to create Skia GrDirectContext!");
        return;
    }
    recreateSurface();
#endif
}

void ZenithPolySynthUI::openGLContextClosing() {
#ifdef ZENITH_USE_SKIA
    if (surface_) {
        surface_->unref();
        surface_ = nullptr;
    }
    if (grContext_) {
        grContext_->abandonContext(); // Important for cleanup order
        grContext_->unref();
        grContext_ = nullptr;
    }
#endif
}

void ZenithPolySynthUI::recreateSurface() {
#ifdef ZENITH_USE_SKIA
    if (!grContext_) return;

    auto width = getWidth();
    auto height = getHeight();
    
    // Ensure valid dimensions
    if (width <= 0 || height <= 0) return;

    // GrGLFramebufferInfo for default framebuffer (ID 0)
    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = 0; 
    framebufferInfo.fFormat = 0x8058; // GL_RGBA8

    auto backendRT = GrBackendRenderTargets::MakeGL(
        width, height,
        0, // sample count
        8, // stencil bits
        framebufferInfo
    );

    auto s = SkSurfaces::WrapBackendRenderTarget(
        grContext_,
        backendRT,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr
    );

    if (surface_) surface_->unref();
    surface_ = s.release();
#endif
}

void ZenithPolySynthUI::renderOpenGL() {
#ifdef ZENITH_USE_SKIA
    if (!grContext_) return;

    // Check resize
    if (getWidth() != lastWidth_ || getHeight() != lastHeight_) {
        recreateSurface();
        lastWidth_ = getWidth();
        lastHeight_ = getHeight();
    }

    if (!surface_) return;

    auto canvas = surface_->getCanvas();
    // Clear background
    canvas->clear(SkColorSetARGB(255, 10, 10, 15));

    // Update animation
    animationTime_ += 0.01f; // Simple tick

    // Call the main draw method
    drawSkia(canvas);

    // Flush to GPU
    grContext_->flushAndSubmit();
#else
    // Fallback
    juce::OpenGLHelpers::clear(juce::Colours::black);
#endif
}

//==============================================================================
// Drawing
//==============================================================================

void ZenithPolySynthUI::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  // 1. Draw Background
  drawBackground(canvas);

  // 2. Recursive Draw for Children
  for (auto *child : getChildren()) {
      if (!child->isVisible()) continue;

      canvas->save();
      auto bounds = child->getBounds();
      canvas->translate((SkScalar)bounds.getX(), (SkScalar)bounds.getY());
      
      // Clip to bounds
      canvas->clipRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));

      // If the child is a SkiaComponent, call its custom draw method
      if (auto *skiaComp = dynamic_cast<SkiaComponent *>(child)) {
          skiaComp->drawSkia(canvas);
      }
      
      canvas->restore();
  }
#endif
}

void ZenithPolySynthUI::drawBackground(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  // Draw a subtle gradient background (Nebula effect)
  SkPaint paint;
  
  // Animated Gradient
  // Rotate points based on time
  float angle = animationTime_ * 0.1f;
  float cx = getWidth() * 0.5f;
  float cy = getHeight() * 0.5f;
  float radius = std::max(getWidth(), getHeight()) * 0.8f;
  
  SkPoint points[2] = {
      SkPoint::Make(cx + cos(angle) * radius, cy + sin(angle) * radius),
      SkPoint::Make(cx - cos(angle) * radius, cy - sin(angle) * radius)
  };
  
  // Pulsing colors
  float pulse = (sin(animationTime_ * 0.5f) + 1.0f) * 0.5f; // 0 to 1
  SkColor color1 = SkColorSetRGB(10 + (int)(pulse * 10), 15, 30 + (int)(pulse * 20));
  SkColor color2 = SkColorSetRGB(20, 10 + (int)(pulse * 10), 25);
  
  SkColor colors[2] = {color1, color2};
  
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
}

// Implement remaining helpers stubbed for brevity
void ZenithPolySynthUI::toggleAdvancedMode() {
    isAdvancedMode_ = !isAdvancedMode_;
    if (isAdvancedMode_) {
        setSize(kAdvancedWidth, kAdvancedHeight);
        expandButton_->setButtonText("SIMPLE");
    } else {
        setSize(kSimpleWidth, kSimpleHeight);
        expandButton_->setButtonText("EXPAND");
    }
    resized();
}

void ZenithPolySynthUI::toggleLearningMode() {
    isLearningMode_ = !isLearningMode_;
    learningModeButton_->setButtonText(isLearningMode_ ? "EXIT LEARN" : "LEARN");
    tooltipOverlay_->setVisible(isLearningMode_);
}

void ZenithPolySynthUI::loadPreset(int index) {
    // Stub
}

void ZenithPolySynthUI::loadNextPreset() {
    // Stub
}

void ZenithPolySynthUI::loadPrevPreset() {
    // Stub
}

void ZenithPolySynthUI::refreshPresetList() {
    // Stub
}

} // namespace zenith