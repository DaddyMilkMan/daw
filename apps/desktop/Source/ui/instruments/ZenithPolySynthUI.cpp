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
#include "../framework/GlassmorphicPanel.h" // For GlassmorphicPanel

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
  setSize(800, 500); // Expanded size for panels

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
  osc1Wave_ = addWidget<ZenithKnob>("Osc 1 Wave", ZenithPolySynthProcessor::Osc1Wave);
  osc1Wave_->setAccentColor(design::colors::ACCENT_PRIMARY);
  osc1Wave_->setHelpText("Oscillator 1 Waveform", "Selects the primary waveform. Pro Tip: Use the 'Wavetable' setting for complex timbres that cut through the mix.");
  
  osc1Mix_ = addWidget<ZenithKnob>("Osc 1 Mix", ZenithPolySynthProcessor::Osc1Mix);
  osc1Mix_->setAccentColor(design::colors::ACCENT_PRIMARY);
  osc1Mix_->setHelpText("Oscillator 1 Mix", "Adjusts the level of Osc 1. Tip: Reducing this while increasing Resonance can prevent harsh digital clipping.");
  
  osc2Wave_ = addWidget<ZenithKnob>("Osc 2 Wave", ZenithPolySynthProcessor::Osc2Wave);
  osc2Wave_->setAccentColor(design::colors::ACCENT_PRIMARY);
  osc2Wave_->setHelpText("Oscillator 2 Waveform", "Second oscillator waveform. Detune this slightly against Osc 1 for a thicker, 'unison' VA sound.");
  
  osc2Mix_ = addWidget<ZenithKnob>("Osc 2 Mix", ZenithPolySynthProcessor::Osc2Mix);
  osc2Mix_->setAccentColor(design::colors::ACCENT_PRIMARY);
  osc2Mix_->setHelpText("Oscillator 2 Mix", "Level of Osc 2. Use this to blend in a different harmonic structure compared to Osc 1.");

  // Filter
  cutoff_ = addWidget<ZenithKnob>("Cutoff", ZenithPolySynthProcessor::FilterCutoff);
  cutoff_->setAccentColor(design::colors::ACCENT_SECONDARY);
  cutoff_->setHelpText("Filter Cutoff", "Controls the brightness. Pro Tip: Automation of this parameter is the key to creating movement in your basslines.");
  
  resonance_ = addWidget<ZenithKnob>("Resonance", ZenithPolySynthProcessor::FilterResonance);
  resonance_->setAccentColor(design::colors::ACCENT_SECONDARY);
  resonance_->setHelpText("Filter Resonance", "Adds a peak at the cutoff frequency. High values create the classic 'squelch' found in acid house.");
  
  envAmt_ = addWidget<ZenithKnob>("Env Amt", ZenithPolySynthProcessor::FilterEnvAmount);
  envAmt_->setAccentColor(design::colors::ACCENT_SECONDARY);
  envAmt_->setHelpText("Envelope Amount", "Determines how much the Mod Envelope (Env 2) affects the Cutoff. Perfect for creating 'plucky' or 'snappy' sounds.");

  // Amp Envelope
  attack_ = addWidget<ZenithKnob>("Attack", ZenithPolySynthProcessor::AmpAttack);
  attack_->setAccentColor(design::colors::ORANGE);
  attack_->setHelpText("Amp Attack", "Sets the time for the sound to reach full volume. Long attack is great for cinematic pads.");
  
  decay_ = addWidget<ZenithKnob>("Decay", ZenithPolySynthProcessor::AmpDecay);
  decay_->setAccentColor(design::colors::ORANGE);
  decay_->setHelpText("Amp Decay", "The time taken to drop to the sustain level. Short decay creates percussive 'hits'.");
  
  sustain_ = addWidget<ZenithKnob>("Sustain", ZenithPolySynthProcessor::AmpSustain);
  sustain_->setAccentColor(design::colors::ORANGE);
  sustain_->setHelpText("Amp Sustain", "The volume level held while a key is depressed. Set to 0 for short stabs.");
  
  release_ = addWidget<ZenithKnob>("Release", ZenithPolySynthProcessor::AmpRelease);
  release_->setAccentColor(design::colors::ORANGE);
  release_->setHelpText("Amp Release", "How long the sound lingers after releasing the key. Add release for a more natural, acoustic feel.");

  layoutWidgets();
}

void ZenithPolySynthUI::layoutWidgets() {
  auto bounds = getLocalBounds().toFloat();
  SkRect area = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());

  // Margins
  float outerMargin = 20.0f;
  float panelSpacing = 20.0f;
  float titleHeight = 30.0f;

  // Visualizer at the bottom
  float visualizerHeight = 120.0f;
  SkRect visualizerRect = SkRect::MakeLTRB(area.left(), area.bottom() - visualizerHeight,
                                           area.right(), area.bottom());

  if (visualizer_) {
    visualizer_->setBounds(static_cast<int>(visualizerRect.x()),
                           static_cast<int>(visualizerRect.y()),
                           static_cast<int>(visualizerRect.width()),
                           static_cast<int>(visualizerRect.height()));
  }

  // Main Controls Area
  SkRect mainArea = SkRect::MakeLTRB(area.left() + outerMargin, area.top() + outerMargin,
                                     area.right() - outerMargin, visualizerRect.top() - outerMargin);

  // Split into 3 Panels
  float panelWidth = (mainArea.width() - (panelSpacing * 2)) / 3.0f;

  // 1. Oscillators Panel
  oscPanelBounds_ = SkRect::MakeXYWH(mainArea.left(), mainArea.top(), panelWidth, mainArea.height());

  // 2. Filter Panel
  filterPanelBounds_ = SkRect::MakeXYWH(oscPanelBounds_.right() + panelSpacing, mainArea.top(), panelWidth, mainArea.height());

  // 3. Envelope Panel
  envPanelBounds_ = SkRect::MakeXYWH(filterPanelBounds_.right() + panelSpacing, mainArea.top(), panelWidth, mainArea.height());

  // Helper to layout grid of knobs in a panel
  auto layoutKnobsInPanel = [](SkRect panel, std::vector<ZenithKnob*> knobs, float headerH) {
      // Inner content area
      juce::Rectangle<int> content = juce::Rectangle<int>(
          static_cast<int>(panel.x()), static_cast<int>(panel.y()),
          static_cast<int>(panel.width()), static_cast<int>(panel.height())).reduced(10);
      content.removeFromTop(static_cast<int>(headerH));

      // Use ZenithLayout grid
      // Convert vector<ZenithKnob*> to vector<Component*>
      std::vector<juce::Component*> comps;
      for (auto* k : knobs) if (k) comps.push_back(k);

      ZenithLayout::grid(content, comps, 2, 20); // 2 columns
  };

  layoutKnobsInPanel(oscPanelBounds_, {osc1Wave_, osc1Mix_, osc2Wave_, osc2Mix_}, titleHeight);
  layoutKnobsInPanel(filterPanelBounds_, {cutoff_, resonance_, envAmt_}, titleHeight);
  layoutKnobsInPanel(envPanelBounds_, {attack_, decay_, sustain_, release_}, titleHeight);
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
#ifdef ZENITH_USE_SKIA
void ZenithPolySynthUI::drawSkiaContent(SkCanvas *canvas) {
  auto bounds = getLocalBounds();

  // 1. Draw Global Background
  canvas->clear(design::colors::BG_DARKEST);

  // 2. Draw Panels (Backgrounds)
  auto drawPanel = [&](SkRect rect, const char* title) {
      // Glassmorphic Panel
      GlassmorphicPanel::Options opts;
      opts.style = GlassmorphicPanel::Style::Elevated;
      opts.cornerRadius = design::dimensions::RADIUS_MD;
      GlassmorphicPanel::drawWithOptions(canvas, rect, opts);

      // Title
      SkPaint titlePaint;
      titlePaint.setColor(design::colors::TEXT_SECONDARY);
      titlePaint.setAntiAlias(true);
      SkFont titleFont = design::typography::getSkFont(12.0f, design::FontWeight::Bold);

      float textW = titleFont.measureText(title, strlen(title), SkTextEncoding::kUTF8);
      float textX = rect.centerX() - (textW / 2.0f);
      float textY = rect.top() + 24.0f;

      canvas->drawString(title, textX, textY, titleFont, titlePaint);

      // Divider
      SkPaint linePaint;
      linePaint.setColor(design::colors::BORDER_SUBTLE);
      linePaint.setStrokeWidth(1.0f);
      float lineY = textY + 12.0f;
      canvas->drawLine(rect.left() + 10, lineY, rect.right() - 10, lineY, linePaint);
  };

  drawPanel(oscPanelBounds_, "OSCILLATORS");
  drawPanel(filterPanelBounds_, "FILTER");
  drawPanel(envPanelBounds_, "ENVELOPE");

  // 3. Draw widgets with translation
  // Since widgets are components, JUCE usually handles their painting if they are peers.
  // However, Zenith PolySynth uses a mixed mode or custom rendering if this method is active.
  // Assuming SkiaRenderer handles the top-level canvas.
  // If SkiaWidget::drawSkia is virtual and widgets are not JUCE components, we call them.
  // BUT: ZenithKnob is a JUCE Component.

  // IMPORTANT: If widgets are JUCE components and are visible, JUCE's paint() mechanism
  // might interfere or they might need to be drawn manually if we are suppressing standard paint.
  // In `ZenithKnob`, `paint()` is likely overridden to call `drawSkia`.

  // The `SkiaRenderer` implementation typically wraps `paint` calls.
  // If we are here, we are drawing to the SkCanvas.

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
