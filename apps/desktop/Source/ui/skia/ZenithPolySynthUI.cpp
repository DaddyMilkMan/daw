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
#include "../../instruments/ZenithFilter.h" // For FilterType
#include "../../instruments/ZenithPolySynth.h"
#include "ZenithDesignSystem.h"
#include "ZenithLayout.h"
#include "ZenithUIComponents.h" // For ZenithVisualizer
#include "ZenithUtils.h"

#include <algorithm> // For std::clamp
#include <map>       // For std::map used in presets
#include <vector>

namespace zenith {
using namespace design;

//==============================================================================
ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), processor(p),
      renderer_(std::make_unique<SkiaRenderer>(
          *this, SkiaRenderer::Backend::Auto)), // Use proper Backend enum
      visualizer_(std::make_unique<ZenithVisualizer>(
          processor)) // Pass processor to visualizer
{
  addAndMakeVisible(*visualizer_);

  setOpaque(false);         // Enable transparency if needed for glassmorphism
  setBufferedToImage(true); // Double buffering for smoother rendering

  // Set initial size
  setSize(600, 400);

  // Layout components
  buildUI();

  // Start UI update timer
  startTimerHz(60);

  // Initial sync
  syncProcessorToUI();

  // Listen for theme changes
  ThemeManager::getInstance().addChangeListener(this);

  // Initialize Skia Renderer
  if (!renderer_->initialize()) {
    DBG("ZenithPolySynthUI: SkiaRenderer failed to initialize!");
  }

  DBG("ZenithPolySynthUI: Created");
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
  stopTimer();
  renderer_->shutdown();
  ThemeManager::getInstance().removeChangeListener(this);
  DBG("ZenithPolySynthUI: Destroyed");
}

//==============================================================================
void ZenithPolySynthUI::paint(juce::Graphics &g) {
  juce::ignoreUnused(g);
  // Render the UI using SkiaRenderer
  renderer_->render([this](SkCanvas *canvas) { drawSkiaContent(canvas); });
}

void ZenithPolySynthUI::resized() {
  // Layout components dynamically
  layoutWidgets();
}

template <typename T>
T *ZenithPolySynthUI::addWidget(const juce::String &name,
                                const juce::String &paramId) {
  auto widget = std::make_unique<T>(name);

  // Find parameter in APVTS
  auto *param = processor.getParameters().getParameter(paramId);
  if (auto *rangedParam = dynamic_cast<juce::RangedAudioParameter *>(param)) {
    widget->setParameter(rangedParam);
  }

  T *ptr = widget.get();
  addAndMakeVisible(*widget);
  widgets_.push_back(std::move(widget));
  return ptr;
}

void ZenithPolySynthUI::buildUI() {
  widgets_.clear();

  // Oscillators
  addWidget<ZenithKnob>("Osc 1 Wave", ZenithPolySynthProcessor::Osc1Wave);
  addWidget<ZenithKnob>("Osc 1 Mix", ZenithPolySynthProcessor::Osc1Mix);
  addWidget<ZenithKnob>("Osc 2 Wave", ZenithPolySynthProcessor::Osc2Wave);
  addWidget<ZenithKnob>("Osc 2 Mix", ZenithPolySynthProcessor::Osc2Mix);

  // Filter
  addWidget<ZenithKnob>("Cutoff", ZenithPolySynthProcessor::FilterCutoff);
  addWidget<ZenithKnob>("Resonance", ZenithPolySynthProcessor::FilterResonance);
  addWidget<ZenithKnob>("Env Amt", ZenithPolySynthProcessor::FilterEnvAmount);

  // Amp Envelope
  addWidget<ZenithKnob>("Attack", ZenithPolySynthProcessor::AmpAttack);
  addWidget<ZenithKnob>("Decay", ZenithPolySynthProcessor::AmpDecay);
  addWidget<ZenithKnob>("Sustain", ZenithPolySynthProcessor::AmpSustain);
  addWidget<ZenithKnob>("Release", ZenithPolySynthProcessor::AmpRelease);

  layoutWidgets();
}

void ZenithPolySynthUI::layoutWidgets() {
  auto area = getLocalBounds();

  // Visualizer at the bottom
  auto visualizerArea = area.removeFromBottom(100);
  if (visualizer_)
    visualizer_->setBounds(visualizerArea);

  // Simple Grid Layout for Controls
  int cols = 6;
  int rows = 2; // Approx
  int margin = 10;
  int w = (area.getWidth() - (cols + 1) * margin) / cols;
  int h = 100;

  int x = 0;
  int y = 0;

  for (const auto &widget : widgets_) {
    widget->setBounds(margin + x * (w + margin), margin + y * (h + margin), w,
                      h);
    x++;
    if (x >= cols) {
      x = 0;
      y++;
    }
  }
}

//==============================================================================
// Timer callback for UI updates
void ZenithPolySynthUI::timerCallback() {
  // Trigger UI repaint - visualizer handles data internally via timerCallback
  repaint();
}

//==============================================================================
// Skia Integration (Render callback for SkiaRenderer)
#ifdef ZENITH_USE_SKIA
void ZenithPolySynthUI::drawSkiaContent(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  int width = bounds.getWidth();
  int height = bounds.getHeight();

  // Draw background
  canvas->clear(design::colors::BG_DARKEST);
  SkRect drawBounds = SkRect::MakeXYWH(0, 0, (float)width, (float)height);
  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_DARKER);
  canvas->drawRect(drawBounds, bgPaint);

  // Draw widgets with translation
  for (const auto &widget : widgets_) {
    if (widget->isVisible()) {
      canvas->save();
      canvas->translate((SkScalar)widget->getX(), (SkScalar)widget->getY());
      widget->drawSkia(canvas);
      canvas->restore();
    }
  }

  // Draw visualizer with translation
  if (visualizer_ != nullptr && visualizer_->isVisible()) {
    canvas->save();
    canvas->translate((SkScalar)visualizer_->getX(),
                      (SkScalar)visualizer_->getY());
    visualizer_->drawSkia(canvas);
    canvas->restore();
  }
}
#endif

void ZenithPolySynthUI::syncProcessorToUI() {
  // Update UI elements based on processor state
  // For example, set knob values
}

void ZenithPolySynthUI::changeListenerCallback(
    juce::ChangeBroadcaster *source) {
  if (source == &ThemeManager::getInstance()) {
    repaint();
  }
}

//==============================================================================
// Event handlers for UI interaction (to update processor parameters)
//==============================================================================

void ZenithPolySynthUI::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Handle mouse down events on custom components
}

void ZenithPolySynthUI::mouseDrag(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Handle mouse drag events
}

void ZenithPolySynthUI::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Handle mouse up events
}

void ZenithPolySynthUI::mouseMove(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Handle mouse move events
}

} // namespace zenith
