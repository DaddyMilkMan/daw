/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

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
#include "../../instruments/ZenithPolySynthDefs.h" // For FilterModelType
#include "../../instruments/ZenithPolySynth.h"
#include "../controls/ZenithUIComponents.h" // For ZenithVisualizer
#include "ZenithDesignSystem.h"
#include "ZenithLayout.h"
#include "ZenithUtils.h"

#include <algorithm> // For std::clamp
#include <map>       // For std::map used in presets
#include <vector>
#include "../design-system/ThemeManager.h"

namespace zenith {
using namespace design;

//==============================================================================
ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), processor(p),
      renderer_(std::make_unique<SkiaRenderer>(
          *this, SkiaRenderer::Backend::Auto)), // Use proper Backend enum
      visualizer_(std::make_unique<ZenithVisualizer>(
          processor)), // Pass processor to visualizer
      filterResponseDisplay_(std::make_unique<FilterResponseDisplay>())
{
  addAndMakeVisible(*visualizer_);
  addAndMakeVisible(*filterResponseDisplay_);
  addAndMakeVisible(*modulationVisualizer_);
  addAndMakeVisible(*oscilloscope_);

  // Configure filter response display
  filterResponseDisplay_->setSampleRate(processor.getSampleRate());
  filterResponseDisplay_->setFrequencyRange(20.0f, 20000.0f);
  filterResponseDisplay_->setDbRange(-60.0f, 10.0f);

  // Configure modulation visualizer
  modulationVisualizer_->setDisplayMode(ModulationVisualizer::DisplayMode::Compact);

  // Configure oscilloscope
  oscilloscope_->setTimeScale(2048.0f);
  oscilloscope_->setTriggerLevel(0.0f);
  oscilloscope_->setTriggerMode(::zenith::TriggerMode::Auto);

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
  design::ThemeManager::getInstance().addChangeListener(this);
  
  // Register as Wingman listener for real-time parameter animation
  if (auto* bridge = processor.getWingmanBridge()) {
    bridge->addListener(this);
  }

  // Initialize Skia Renderer
  if (!renderer_->initialize()) {
    DBG("ZenithPolySynthUI: SkiaRenderer failed to initialize!");
  }

  DBG("ZenithPolySynthUI: Created");
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
  // Unregister from Wingman
  if (auto* bridge = processor.getWingmanBridge()) {
    bridge->removeListener(this);
  }
  
  stopTimer();
  renderer_->shutdown();
  design::ThemeManager::getInstance().removeChangeListener(this);
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

  // Increase window size to accommodate visualizations
  int newHeight = std::max(700, getHeight());
  int newWidth = std::max(900, getWidth());
  if (getWidth() < newWidth || getHeight() < newHeight) {
    setSize(newWidth, newHeight);
    area = getLocalBounds();
  }

  // Bottom visualization section (larger to accommodate all components)
  auto vizArea = area.removeFromBottom(280);

  // Split into 2x2 grid for the 4 visualization components:
  // Top row: Filter Response | Oscilloscope
  // Bottom row: Waveform Visualizer | Modulation Matrix
  auto topRow = vizArea.removeFromTop(vizArea.getHeight() / 2);

  // Top row
  auto filterResponseArea = topRow.removeFromLeft(topRow.getWidth() / 2);
  auto oscilloscopeArea = topRow; // Remaining space

  // Bottom row
  auto waveformArea = vizArea.removeFromLeft(vizArea.getWidth() / 2);
  auto modulationArea = vizArea; // Remaining space

  if (filterResponseDisplay_)
    filterResponseDisplay_->setBounds(filterResponseArea.reduced(4));

  if (oscilloscope_)
    oscilloscope_->setBounds(oscilloscopeArea.reduced(4));

  if (visualizer_)
    visualizer_->setBounds(waveformArea.reduced(4));

  if (modulationVisualizer_)
    modulationVisualizer_->setBounds(modulationArea.reduced(4));

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
  // Update Wingman parameter animations
  if (!activeAnimations_.isEmpty()) {
    double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    
    for (int i = activeAnimations_.size(); --i >= 0;) {
      auto& anim = activeAnimations_.getReference(i);
      
      anim.progress += 0.016 * anim.speed; // ~60fps
      
      if (anim.progress >= 1.0f) {
        anim.widget->setValue(anim.targetValue, false);
        activeAnimations_.remove(i);
      } else {
        // Ease-out interpolation for smooth feel
        float t = anim.progress;
        t = 1.0f - std::pow(1.0f - t, 3.0f);
        anim.widget->setValue(anim.startValue + (anim.targetValue - anim.startValue) * t, false);
      }
    }
    
    if (activeAnimations_.isEmpty()) {
      stopTimer();
    }
    
    repaint();
  }
  
  // Update visualizer data
  if (visualizer_) {
    visualizer_->updateAudioData();

    // Also feed data to oscilloscope if visualizer has buffer access
    // The visualizer should share its buffer with the oscilloscope
  }

  // Update filter response display with current parameters
  if (filterResponseDisplay_) {
    // Get current filter parameters from processor
    auto* cutoffParam = processor.getParameters().getParameter(
        ZenithPolySynthProcessor::FilterCutoff);
    auto* resonanceParam = processor.getParameters().getParameter(
        ZenithPolySynthProcessor::FilterResonance);
    auto* typeParam = processor.getParameters().getParameter(
        ZenithPolySynthProcessor::FilterType);
    auto* modelParam = processor.getParameters().getParameter(
        ZenithPolySynthProcessor::FilterModel);

    if (cutoffParam && resonanceParam) {
      float cutoff = cutoffParam->getValue();
      float resonance = resonanceParam->getValue();
      FilterType fType = FilterType::Lowpass;
      FilterModelType fModel = FilterModelType::MoogLadder;

      // Get filter type
      if (typeParam) {
        int typeIndex = static_cast<int>(typeParam->getValue());
        fType = static_cast<FilterType>(juce::jlimit(0, static_cast<int>(FilterType::NumTypes) - 1, typeIndex));
      }

      // Get filter model
      if (modelParam) {
        int modelIndex = static_cast<int>(modelParam->getValue());
        fModel = static_cast<FilterModelType>(juce::jlimit(0, static_cast<int>(FilterModelType::NumModels) - 1, modelIndex));
      }

      // Convert cutoff from normalized to Hz (assuming 20-20000 Hz range)
      float cutoffHz = 20.0f + cutoff * (20000.0f - 20.0f);

      // Update the display
      filterResponseDisplay_->setFilterParameters(cutoffHz, resonance, fType, fModel);
    }
  }
  
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
  if (visualizer_ && visualizer_->isVisible()) {
    canvas->save();
    canvas->translate((SkScalar)visualizer_->getX(),
                      (SkScalar)visualizer_->getY());
    visualizer_->drawSkia(canvas);
    canvas->restore();
  }

  // Draw filter response display with translation
  if (filterResponseDisplay_ && filterResponseDisplay_->isVisible()) {
    canvas->save();
    canvas->translate((SkScalar)filterResponseDisplay_->getX(),
                      (SkScalar)filterResponseDisplay_->getY());
    filterResponseDisplay_->drawSkia(canvas);
    canvas->restore();
  }

  // Draw modulation visualizer with translation
  if (modulationVisualizer_ && modulationVisualizer_->isVisible()) {
    canvas->save();
    canvas->translate((SkScalar)modulationVisualizer_->getX(),
                      (SkScalar)modulationVisualizer_->getY());
    modulationVisualizer_->drawSkia(canvas);
    canvas->restore();
  }

  // Draw oscilloscope with translation
  if (oscilloscope_ && oscilloscope_->isVisible()) {
    canvas->save();
    canvas->translate((SkScalar)oscilloscope_->getX(),
                      (SkScalar)oscilloscope_->getY());
    oscilloscope_->drawSkia(canvas);
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
  if (source == &design::ThemeManager::getInstance()) {
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

//==============================================================================
// WingmanSynthListener Implementation
//==============================================================================

void ZenithPolySynthUI::wingmanParameterChanged(const WingmanParameterChange& change) {
  // Find widget by parameter ID and animate to new value
  for (auto& widget : widgets_) {
    auto* param = widget->getParameter();
    if (param != nullptr && param->paramID == change.parameterId) {
      animateWidgetToValue(widget.get(), change.newValue, change.animationSpeed);
      repaint();
      break;
    }
  }
}

void ZenithPolySynthUI::wingmanBatchStart() {
  // Optimization: Disable individual repaints during batch operations
  setBufferedToImage(true);
}

void ZenithPolySynthUI::wingmanBatchEnd() {
  // Re-enable normal rendering and trigger final repaint
  setBufferedToImage(false);
  repaint();
}

void ZenithPolySynthUI::wingmanSoundGenerated(const juce::String& description) {
  DBG("Wingman generated: " + description);
  // TODO: Show notification in UI
}

void ZenithPolySynthUI::animateWidgetToValue(ZenithControl* widget, float targetValue, float speed) {
  if (!widget) return;
  
  WidgetAnimation anim;
  anim.widget = widget;
  anim.startValue = widget->getValue();
  anim.targetValue = targetValue;
  anim.progress = 0.0f;
  anim.speed = speed;
  anim.startTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
  
  activeAnimations_.add(anim);
  
  if (!isTimerRunning()) {
    startTimerHz(60); // 60 FPS for smooth animations
  }
}

} // namespace zenith
