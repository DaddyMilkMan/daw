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
#include <core/SkColor.h>
#include <core/SkSurface.h>
#include <core/SkRefCnt.h>
#include <core/SkPoint.h>
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), 
      SkiaRenderer(this), 
      processor(p) {
  
  setSize(kSimpleWidth, kSimpleHeight);

#ifdef ZENITH_USE_SKIA
  // Initialize Widgets
  addWidget<SkiaKnob>("CUTOFF", ZenithPolySynthProcessor::FilterCutoff);
  addWidget<SkiaKnob>("RES", ZenithPolySynthProcessor::FilterResonance);
  addWidget<SkiaKnob>("ENV AMT", ZenithPolySynthProcessor::FilterEnvAmount);
  addWidget<SkiaKnob>("SUB", ZenithPolySynthProcessor::SubOscLevel);
  addWidget<SkiaKnob>("NOISE", ZenithPolySynthProcessor::NoiseLevel);
  
  addWidget<SkiaSlider>("A", ZenithPolySynthProcessor::AmpAttack);
  addWidget<SkiaSlider>("D", ZenithPolySynthProcessor::AmpDecay);
  addWidget<SkiaSlider>("S", ZenithPolySynthProcessor::AmpSustain);
  addWidget<SkiaSlider>("R", ZenithPolySynthProcessor::AmpRelease);
  
  // Visualizer (JUCE Component wrapper)
  visualizer_ = std::make_unique<ZenithVisualizer>(processor);
  addAndMakeVisible(visualizer_.get());
  
  // Preset Bar (JUCE Component)
  presetBar_ = std::make_unique<ZenithPresetBar>();
  presetBar_->setCallbacks(
      [this]() { loadPrevPreset(); },
      [this]() { loadNextPreset(); },
      []() {} 
  );
  addAndMakeVisible(presetBar_.get());
  
  refreshPresetList();
  setLookAndFeel(&zenithLookAndFeel_);
  
  // Trigger initial layout
  resized();
  
  // PHASE 1: Start frame capture timer (60 FPS)
  startTimer(16); // ~60Hz frame capture
#endif
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
  setLookAndFeel(nullptr);
}

template <typename T>
T* ZenithPolySynthUI::addWidget(const juce::String& name, const juce::String& paramId) {
    auto widget = std::make_unique<T>(name);
    T* ptr = widget.get();
    
    // Bind parameter
    auto* param = processor.getParameters().getParameter(paramId);
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param)) {
        widget->setParameter(ranged);
    }
    
    widgets_.push_back(std::move(widget));
    return ptr;
}

void ZenithPolySynthUI::paint(juce::Graphics &g) {
  // Fallback / Placeholder
  g.fillAll(juce::Colours::black);
}

void ZenithPolySynthUI::drawSkiaContent(SkCanvas* canvas) {
#ifdef ZENITH_USE_SKIA
  if (!canvas) return;

  auto scale = static_cast<float>(openGLContext_.getRenderingScale());
  canvas->save();
  canvas->scale(scale, scale);

  drawBackground(canvas);
  
  // Draw Widgets
  for (auto& widget : widgets_) {
      widget->draw(canvas);
  }
  
  // Draw Child Components (Visualizer, PresetBar)
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
  canvas->clipRect(SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()));

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
  auto bounds = getLocalBounds().toFloat();
  SkPaint paint;
  paint.setAntiAlias(true);

  SkPoint center = { bounds.getCentreX(), bounds.getCentreY() };
  SkColor colors[2] = { SkColorSetRGB(30, 30, 40), SkColorSetRGB(10, 10, 15) };
  float radius = std::max(bounds.getWidth(), bounds.getHeight()) * 0.8f;
  
  paint.setShader(SkGradientShader::MakeRadial(center, radius, colors, nullptr, 2, SkTileMode::kClamp));
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), paint);
  paint.setShader(nullptr);
#else
  juce::ignoreUnused(canvas);
#endif
}

void ZenithPolySynthUI::resized() {
  auto area = getLocalBounds().reduced(20);
  
  if (presetBar_) presetBar_->setBounds(area.removeFromTop(40).reduced(100, 0));
  if (visualizer_) visualizer_->setBounds(area.removeFromTop(150).reduced(10));
  
  layoutWidgets();
}

void ZenithPolySynthUI::layoutWidgets() {
    // Simple manual layout for now (matching previous resized)
    int y = 220;
    
    // Iterate widgets and layout by name (brittle but functional for refactor)
    for (auto& w : widgets_) {
        if (w->name == "CUTOFF") w->bounds = juce::Rectangle<float>(250, y, 100, 100);
        else if (w->name == "RES") w->bounds = juce::Rectangle<float>(370, y, 80, 80);
        else if (w->name == "ENV AMT") w->bounds = juce::Rectangle<float>(460, y, 80, 80);
        else if (w->name == "SUB") w->bounds = juce::Rectangle<float>(20, y, 60, 60);
        else if (w->name == "NOISE") w->bounds = juce::Rectangle<float>(90, y, 60, 60);
        else if (w->name == "A") w->bounds = juce::Rectangle<float>(550, y, 30, 150);
        else if (w->name == "D") w->bounds = juce::Rectangle<float>(590, y, 30, 150);
        else if (w->name == "S") w->bounds = juce::Rectangle<float>(630, y, 30, 150);
        else if (w->name == "R") w->bounds = juce::Rectangle<float>(670, y, 30, 150);
    }
}

// Mouse Handling
void ZenithPolySynthUI::mouseDown(const juce::MouseEvent& e) {
    // Hit test widgets
    for (auto& w : widgets_) {
        if (w->hitTest(e.x, e.y)) {
            activeWidget_ = w.get();
            activeWidget_->onMouseDown(e);
            return;
        }
    }
    juce::Component::mouseDown(e); // Pass to children
}

void ZenithPolySynthUI::mouseDrag(const juce::MouseEvent& e) {
    if (activeWidget_) {
        activeWidget_->onMouseDrag(e);
        repaint(); // Request Skia repaint
    } else {
        juce::Component::mouseDrag(e);
    }
}

void ZenithPolySynthUI::mouseUp(const juce::MouseEvent& e) {
    if (activeWidget_) {
        activeWidget_->onMouseUp(e);
        activeWidget_ = nullptr;
        repaint();
    } else {
        juce::Component::mouseUp(e);
    }
}

void ZenithPolySynthUI::mouseMove(const juce::MouseEvent& e) {
    SkiaWidget* newHover = nullptr;
    for (auto& w : widgets_) {
        if (w->hitTest(e.x, e.y)) {
            newHover = w.get();
            break;
        }
    }
    
    if (newHover != hoveredWidget_) {
        if (hoveredWidget_) hoveredWidget_->isHovered = false;
        if (newHover) newHover->isHovered = true;
        hoveredWidget_ = newHover;
        repaint();
    }
    
    juce::Component::mouseMove(e);
}

void ZenithPolySynthUI::paint(juce::Graphics& g) {
    // Fallback for non-Skia builds
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.drawText("Zenith PolySynth", getLocalBounds(), juce::Justification::centred);
}

void ZenithPolySynthUI::drawSkiaContent(SkCanvas* canvas) {
#ifdef ZENITH_USE_SKIA
    if (!canvas) return;
    
    // Debug: Verify we're NOT on Message Thread (we're on OpenGL thread)
    jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    // Get latest frame snapshot (lock-free read, NO Component access!)
    const auto* frame = frameBuffer_.getLatestFrame();
    
    // Clear background
    canvas->clear(SkColorSetRGB(20, 20, 25));
    
    // Draw radial gradient background
    auto bounds = frame->componentBounds.toFloat();
    if (!bounds.isEmpty()) {
        SkPaint paint;
        paint.setAntiAlias(true);
        
        SkPoint center = { bounds.getCentreX(), bounds.getCentreY() };
        SkColor colors[2] = { SkColorSetRGB(30, 30, 40), SkColorSetRGB(10, 10, 15) };
        float radius = std::max(bounds.getWidth(), bounds.getHeight()) * 0.8f;
        
        paint.setShader(SkGradientShader::MakeRadial(center, radius, colors, nullptr, 2, SkTileMode::kClamp));
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), paint);
        paint.setShader(nullptr);
    }
    
    // Draw all knobs from snapshot (thread-safe!)
    for (const auto& knob : frame->knobs) {
        drawKnobFromState(canvas, knob);
    }
    
    // TODO: Draw sliders, buttons, visualizer from state
    
#else
    juce::ignoreUnused(canvas);
#endif
}

// ============================================================================
// PHASE 1: THREAD-SAFE RENDERING
// ============================================================================

void ZenithPolySynthUI::timerCallback() {
    // Called on Message Thread at 60Hz
    captureFrameSnapshot();
}

void ZenithPolySynthUI::captureFrameSnapshot() {
    // Debug: Verify we're on Message Thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    // Get writable buffer (Message Thread safe)
    auto* frame = frameBuffer_.getWriteBuffer();
    frame->clear();
    
    // Capture global state
    frame->isAdvancedMode = isAdvancedMode_;
    frame->componentBounds = getLocalBounds();
    frame->frameNumber++;
    frame->timestamp = juce::Time::getCurrentTime().toMilliseconds();
    
    // Snapshot all knob widgets
    for (auto& widget : widgets_) {
        if (auto* knob = dynamic_cast<SkiaKnob*>(widget.get())) {
            frame->knobs.push_back(knob->captureRenderState());
        }
    }
    
    // TODO: Snapshot sliders, buttons, visualizer when ready
    
    // Atomic swap to ready buffer (lock-free)
    frameBuffer_.swapWriteToReady();
}

void ZenithPolySynthUI::drawKnobFromState(SkCanvas* canvas, const render::KnobRenderState& state) {
    if (state.bounds.isEmpty()) return;
    
    canvas->save();
    
    // Apply hover scale animation
    if (state.scale > 1.0f) {
        float cx = state.bounds.centerX();
        float cy = state.bounds.centerY();
        canvas->translate(cx, cy);
        canvas->scale(state.scale, state.scale);
        canvas->translate(-cx, -cy);
    }
    
    // Calculate knob geometry
    float cx = state.bounds.centerX();
    float cy = state.bounds.centerY();
    float radius = std::min(state.bounds.width(), state.bounds.height()) * 0.35f;
    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    
    // Arc parameters (270° rotation range)
    float startAngle = -135.0f; // -270/2 - 90
    float sweepAngle = state.value * 270.0f;
    
    // 1. Background track (full range)
    SkPaint trackPaint;
    trackPaint.setStyle(SkPaint::kStroke_Style);
    trackPaint.setStrokeWidth(2.5f);
    trackPaint.setColor(SkColorSetARGB(77, 255, 255, 255)); // 30% white
    trackPaint.setAntiAlias(true);
    trackPaint.setStrokeCap(SkPaint::kRound_Cap);
    canvas->drawArc(arcRect, startAngle, 270.0f, false, trackPaint);
    
    // 2. Glow layer (if hovered)
    if (state.glowIntensity > 0.01f) {
        SkPaint glowPaint;
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(5.0f);
        glowPaint.setColor(SkColorSetA(state.glowColor, static_cast<U8CPU>(state.glowIntensity * 102))); // 40% max alpha
        glowPaint.setAntiAlias(true);
        glowPaint.setStrokeCap(SkPaint::kRound_Cap);
        canvas->drawArc(arcRect, startAngle, sweepAngle, false, glowPaint);
    }
    
    // 3. Value arc (current value)
    SkPaint valuePaint;
    valuePaint.setStyle(SkPaint::kStroke_Style);
    valuePaint.setStrokeWidth(2.5f);
    valuePaint.setColor(state.baseColor);
    valuePaint.setAntiAlias(true);
    valuePaint.setStrokeCap(SkPaint::kRound_Cap);
    canvas->drawArc(arcRect, startAngle, sweepAngle, false, valuePaint);
    
    // 4. Label text
    if (state.labelText.isNotEmpty()) {
        SkFont font;
        font.setSize(12.0f);
        
        SkPaint textPaint;
        textPaint.setColor(SkColorSetRGB(160, 160, 165));
        
        float textY = cy + radius + 15.0f;
        float width = font.measureText(state.labelText.toRawUTF8(), state.labelText.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
        canvas->drawString(state.labelText.toRawUTF8(), cx - width / 2.0f, textY, font, textPaint);
    }
    
    canvas->restore();
}

// ============================================================================
// STUB IMPLEMENTATIONS (to be removed/implemented later)
// ============================================================================

void ZenithPolySynthUI::toggleAdvancedMode() {}
void ZenithPolySynthUI::toggleLearningMode() {}
void ZenithPolySynthUI::loadPreset(int index) {}
void ZenithPolySynthUI::loadNextPreset() {}
void ZenithPolySynthUI::loadPrevPreset() {}
void ZenithPolySynthUI::refreshPresetList() {}

} // namespace zenith
