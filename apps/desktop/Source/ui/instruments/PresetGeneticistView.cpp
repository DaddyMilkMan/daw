/*
  ==============================================================================

    PresetGeneticistView.cpp
    Created: 2025-12-09
    Author:  Zenith DAW AI Team

  ==============================================================================
*/

#include "PresetGeneticistView.h"
#include "../design-system/ZenithTheme.h"

namespace zenith {
namespace ui {
namespace views {

PresetGeneticistView::PresetGeneticistView(
    zenith::ai::PresetGeneticistAgent &agent)
    : agent_(agent) {
  // Setup controls
  addAndMakeVisible(startButton_);
  startButton_.setToggleState(false,
                              juce::NotificationType::dontSendNotification);
  startButton_.setClickingTogglesState(true);
  startButton_.setColour(juce::TextButton::buttonColourId,
                         ZenithTheme::Colors::bg_02);
  startButton_.setColour(juce::TextButton::buttonOnColourId,
                         ZenithTheme::Colors::accent_primary);
  startButton_.setColour(juce::TextButton::textColourOnId,
                         ZenithTheme::Colors::bg_01);

  startButton_.onClick = [this] {
    if (startButton_.getToggleState()) {
      startButton_.setButtonText("STOP EVOLUTION");
      if (!agent_.isRunning()) {
        agent_.startEvolution();
      } else {
        agent_.resumeEvolution();
      }
    } else {
      startButton_.setButtonText("START EVOLUTION");
      agent_.pauseEvolution();
    }
  };

  addAndMakeVisible(loadTargetButton_);
  loadTargetButton_.setColour(juce::TextButton::buttonColourId,
                              ZenithTheme::Colors::bg_02);
  loadTargetButton_.onClick = [this] {
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select Target Sample",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aif;*.mp3");

    auto flags = juce::FileBrowserComponent::openMode |
                 juce::FileBrowserComponent::canSelectFiles;

    fileChooser_->launchAsync(flags, [this](const juce::FileChooser &fc) {
      auto file = fc.getResult();
      if (file.exists()) {
        agent_.setTargetAudio(file);
      }
    });
  };

  // Start UI update timer at 60Hz
  startTimerHz(60);
}

PresetGeneticistView::~PresetGeneticistView() { stopTimer(); }

void PresetGeneticistView::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.fillAll(ZenithTheme::Colors::bg_01);

  // Draw Grid / Context for Sci-fi look
  g.setColour(ZenithTheme::Colors::border_subtle);
  g.drawRect(bounds, 1.0f);

  // Draw Display Area
  auto displayArea = bounds.reduced(10.0f, 40.0f); // Leave room for buttons
  displayArea.removeFromBottom(10);                // Spacing

  g.setColour(ZenithTheme::Colors::bg_02);
  g.fillRect(displayArea);
  g.setColour(ZenithTheme::Colors::border_default);
  g.drawRect(displayArea, 1.0f);

  // Visualize Target (Ghost)
  if (!targetSpectrumPath_.isEmpty()) {
    g.setColour(ZenithTheme::Colors::text_secondary.withAlpha(0.3f));
    g.strokePath(targetSpectrumPath_, juce::PathStrokeType(2.0f));

    // Fill gradient
    juce::ColourGradient grad(
        ZenithTheme::Colors::text_secondary.withAlpha(0.1f),
        displayArea.getBottomLeft(),
        ZenithTheme::Colors::text_secondary.withAlpha(0.0f),
        displayArea.getTopLeft(), false);
    g.setGradientFill(grad);
    g.fillPath(targetSpectrumPath_);
  }

  // Visualize Current (Glowing)
  if (!currentSpectrumPath_.isEmpty()) {
    // Outer Glow (simulated)
    g.setColour(ZenithTheme::Colors::accent_primary.withAlpha(0.1f));
    g.strokePath(currentSpectrumPath_, juce::PathStrokeType(8.0f));

    g.setColour(ZenithTheme::Colors::accent_primary.withAlpha(0.3f));
    g.strokePath(currentSpectrumPath_, juce::PathStrokeType(4.0f));

    // Main line
    g.setColour(ZenithTheme::Colors::accent_primary);
    g.strokePath(currentSpectrumPath_, juce::PathStrokeType(2.0f));
  }

  // Stats Overlay
  g.setColour(ZenithTheme::Colors::text_primary);
  g.setFont(ZenithTheme::Typography::getSmallFont());

  auto stats = agent_.getStats();
  juce::String statusText =
      "Gen: " + juce::String(stats.generation) +
      " | Best Fitness: " + juce::String(stats.bestFitness, 2) +
      " | Patch: " + stats.bestPresetName;

  if (!startButton_.getToggleState())
    statusText += " [PAUSED]";

  g.drawText(statusText, displayArea.reduced(10), juce::Justification::topLeft);
}

void PresetGeneticistView::resized() {
  auto area = getLocalBounds();
  auto buttonArea = area.removeFromBottom(40).reduced(10, 5);

  startButton_.setBounds(buttonArea.removeFromLeft(150));
  buttonArea.removeFromLeft(10);
  loadTargetButton_.setBounds(buttonArea.removeFromLeft(150));
}

void PresetGeneticistView::timerCallback() {
  auto bounds = getLocalBounds().toFloat().reduced(10.0f, 40.0f);
  bounds.removeFromBottom(10); // Match paint area

  // Get Data
  auto currentSpec = agent_.getCurrentBestSpectrum();
  auto targetSpec = agent_.getTargetSpectrum();

  // Regenerate Paths
  generatePathFromSpectrum(targetSpec, targetSpectrumPath_, bounds);
  generatePathFromSpectrum(currentSpec, currentSpectrumPath_, bounds);

  repaint();
}

void PresetGeneticistView::generatePathFromSpectrum(
    const std::vector<float> &spectrum, juce::Path &path,
    juce::Rectangle<float> bounds) {
  path.clear();
  if (spectrum.empty())
    return;

  // Logarithmic X-axis usually looks better for audio, but linear is simpler
  // for now. Let's do linear mapping as Proof of Concept, or simple log
  // approximation.

  float xStep = bounds.getWidth() / static_cast<float>(spectrum.size());
  bool first = true;

  // Skip DC (index 0)
  for (size_t i = 1; i < spectrum.size(); ++i) {
    float mag = spectrum[i];

    // Convert mag to dB-ish or normalized height
    // Mag is likely small, 0.0-1.0 range or higher depending on FFT norm.
    // Let's assume input needs boosting or logging.
    // AudioBuffer extract usually not normalized to DB yet in computeSpectrum?
    // computeSpectrum extracted magnitude directly.

    float normY = std::sqrt(mag) * 5.0f; // Simple scaling for visual
    normY = juce::jlimit(0.0f, 1.0f, normY);

    float x = bounds.getX() + static_cast<float>(i) * xStep;
    float y = bounds.getBottom() - (normY * bounds.getHeight());

    if (first) {
      path.startNewSubPath(x, bounds.getBottom());
      path.lineTo(x, y);
      first = false;
    } else {
      path.lineTo(x, y);
    }
  }

  // Close path for filling
  path.lineTo(bounds.getRight(), bounds.getBottom());
  path.closeSubPath();
}

} // namespace views
} // namespace ui
} // namespace zenith
