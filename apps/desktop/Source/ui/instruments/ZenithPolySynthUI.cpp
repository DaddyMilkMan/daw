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
#include "../controls/ZenithUIComponents.h" // For ZenithVisualizer
#include "ZenithDesignSystem.h"
#include "ZenithLayout.h"
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
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60);

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
  auto* osc1Wave = addWidget<ZenithKnob>("Osc 1 Wave", ZenithPolySynthProcessor::Osc1Wave);
  osc1Wave->setHelpText("Oscillator 1 Waveform", "Selects the primary waveform. Pro Tip: Use the 'Wavetable' setting for complex timbres that cut through the mix.");
  
  auto* osc1Mix = addWidget<ZenithKnob>("Osc 1 Mix", ZenithPolySynthProcessor::Osc1Mix);
  osc1Mix->setHelpText("Oscillator 1 Mix", "Adjusts the level of Osc 1. Tip: Reducing this while increasing Resonance can prevent harsh digital clipping.");
  
  auto* osc2Wave = addWidget<ZenithKnob>("Osc 2 Wave", ZenithPolySynthProcessor::Osc2Wave);
  osc2Wave->setHelpText("Oscillator 2 Waveform", "Second oscillator waveform. Detune this slightly against Osc 1 for a thicker, 'unison' VA sound.");
  
  auto* osc2Mix = addWidget<ZenithKnob>("Osc 2 Mix", ZenithPolySynthProcessor::Osc2Mix);
  osc2Mix->setHelpText("Oscillator 2 Mix", "Level of Osc 2. Use this to blend in a different harmonic structure compared to Osc 1.");

  // Filter
  auto* cutoff = addWidget<ZenithKnob>("Cutoff", ZenithPolySynthProcessor::FilterCutoff);
  cutoff->setHelpText("Filter Cutoff", "Controls the brightness. Pro Tip: Automation of this parameter is the key to creating movement in your basslines.");
  
  auto* resonance = addWidget<ZenithKnob>("Resonance", ZenithPolySynthProcessor::FilterResonance);
  resonance->setHelpText("Filter Resonance", "Adds a peak at the cutoff frequency. High values create the classic 'squelch' found in acid house.");
  
  auto* envAmt = addWidget<ZenithKnob>("Env Amt", ZenithPolySynthProcessor::FilterEnvAmount);
  envAmt->setHelpText("Envelope Amount", "Determines how much the Mod Envelope (Env 2) affects the Cutoff. Perfect for creating 'plucky' or 'snappy' sounds.");

  // Amp Envelope
  auto* attack = addWidget<ZenithKnob>("Attack", ZenithPolySynthProcessor::AmpAttack);
  attack->setHelpText("Amp Attack", "Sets the time for the sound to reach full volume. Long attack is great for cinematic pads.");
  
  auto* decay = addWidget<ZenithKnob>("Decay", ZenithPolySynthProcessor::AmpDecay);
  decay->setHelpText("Amp Decay", "The time taken to drop to the sustain level. Short decay creates percussive 'hits'.");
  
  auto* sustain = addWidget<ZenithKnob>("Sustain", ZenithPolySynthProcessor::AmpSustain);
  sustain->setHelpText("Amp Sustain", "The volume level held while a key is depressed. Set to 0 for short stabs.");
  
  auto* release = addWidget<ZenithKnob>("Release", ZenithPolySynthProcessor::AmpRelease);
  release->setHelpText("Amp Release", "How long the sound lingers after releasing the key. Add release for a more natural, acoustic feel.");

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
  // Update visualizer data
  if (visualizer_) {
    visualizer_->updateAudioData();
  }
  
  // Trigger UI repaint - visualizer handles data internally via timerCallback
  repaint();
}

//==============================================================================
// Skia Integration (Render callback for SkiaRenderer)
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
  if (visualizer_ && visualizer_->isVisible()) {
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