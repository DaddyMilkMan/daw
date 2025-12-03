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

void ZenithPolySynthUI::toggleAdvancedMode() {}
void ZenithPolySynthUI::toggleLearningMode() {}
void ZenithPolySynthUI::loadPreset(int index) {}
void ZenithPolySynthUI::loadNextPreset() {}
void ZenithPolySynthUI::loadPrevPreset() {}
void ZenithPolySynthUI::refreshPresetList() {}

} // namespace zenith
