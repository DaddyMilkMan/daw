/*
  ==============================================================================

    PresetGeneticistView.cpp
    Created: 2025-12-09
    Author:  Zenith DAW AI Team

  ==============================================================================
*/

#include "PresetGeneticistView.h"
#include "../design-system/ZenithTheme.h"
#include "../design-system/ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>

namespace zenith {
namespace ui {
namespace views {

// Local helper to convert juce::Path to SkPath
static SkPath jucePathToSkPath(const juce::Path& path) {
    SkPath skPath;
    juce::Path::Iterator it(path);
    while (it.next()) {
        switch (it.elementType) {
            case juce::Path::Iterator::startNewSubPath:
                skPath.moveTo(it.x1, it.y1);
                break;
            case juce::Path::Iterator::lineTo:
                skPath.lineTo(it.x1, it.y1);
                break;
            case juce::Path::Iterator::quadraticTo:
                skPath.quadTo(it.x1, it.y1, it.x2, it.y2);
                break;
            case juce::Path::Iterator::cubicTo:
                skPath.cubicTo(it.x1, it.y1, it.x2, it.y2, it.x3, it.y3);
                break;
            case juce::Path::Iterator::closePath:
                skPath.close();
                break;
        }
    }
    return skPath;
}


PresetGeneticistView::PresetGeneticistView(
    zenith::ai::PresetGeneticistAgent &agent)
    : agent_(agent) {
  // Setup controls
  addAndMakeVisible(startButton_);
  startButton_.setToggleable(true);
  startButton_.setToggleState(false);
  startButton_.setButtonStyle(SkiaButton::Style::Primary);

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
  loadTargetButton_.setButtonStyle(SkiaButton::Style::Secondary);
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

void PresetGeneticistView::drawSkia(SkCanvas *canvas) {
  using namespace zenith::design;
  auto area = getLocalBounds().toFloat();

  // Background
  canvas->drawColor(colors::BG_DARKEST);

  // Draw Grid Boundary
  SkPaint borderPaint;
  borderPaint.setColor(colors::BORDER_SUBTLE);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawRect(SkRect::MakeXYWH(0, 0, area.getWidth(), area.getHeight()), borderPaint);

  // Draw Display Area
  SkRect displayArea = SkRect::MakeXYWH(10, 10, area.getWidth() - 20, area.getHeight() - 60);

  SkPaint displayBgPaint;
  displayBgPaint.setColor(colors::BG_DARKER);
  canvas->drawRect(displayArea, displayBgPaint);

  SkPaint displayBorderPaint;
  displayBorderPaint.setColor(colors::BORDER_DEFAULT);
  displayBorderPaint.setStyle(SkPaint::kStroke_Style);
  displayBorderPaint.setStrokeWidth(1.0f);
  canvas->drawRect(displayArea, displayBorderPaint);

  // Visualize Target (Ghost)
  if (!targetSpectrumPath_.isEmpty()) {
    SkPath skTarget = jucePathToSkPath(targetSpectrumPath_);
    
    SkPaint targetPaint;
    targetPaint.setColor(withAlpha(colors::TEXT_SECONDARY, 0.3f));
    targetPaint.setStyle(SkPaint::kStroke_Style);
    targetPaint.setStrokeWidth(2.0f);
    targetPaint.setAntiAlias(true);
    canvas->drawPath(skTarget, targetPaint);

    // Fill gradient
    SkPoint gradPts[2] = {{0, displayArea.fBottom}, {0, displayArea.fTop}};
    SkColor gradColors[2] = {withAlpha(colors::TEXT_SECONDARY, 0.1f), 0};
    SkPaint targetFillPaint;
    targetFillPaint.setShader(SkGradientShader::MakeLinear(gradPts, gradColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawPath(skTarget, targetFillPaint);
  }

  // Visualize Current (Glowing)
  if (!currentSpectrumPath_.isEmpty()) {
    SkPath skCurrent = jucePathToSkPath(currentSpectrumPath_);

    // Outer Glow
    SkPaint glowPaint;
    glowPaint.setColor(withAlpha(zenith::design::colors::CYAN, 0.1f));
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(8.0f);
    glowPaint.setAntiAlias(true);
    canvas->drawPath(skCurrent, glowPaint);

    glowPaint.setColor(withAlpha(zenith::design::colors::CYAN, 0.3f));
    glowPaint.setStrokeWidth(4.0f);
    canvas->drawPath(skCurrent, glowPaint);

    // Main line
    SkPaint linePaint;
    linePaint.setColor(zenith::design::colors::CYAN);
    linePaint.setStyle(SkPaint::kStroke_Style);
    linePaint.setStrokeWidth(2.0f);
    linePaint.setAntiAlias(true);
    canvas->drawPath(skCurrent, linePaint);
  }

  // Stats Overlay
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  SkFont font = typography::getSkFont(typography::FONT_XS, FontWeight::Regular);

  auto stats = agent_.getStats();
  juce::String statusLine =
      "Gen: " + juce::String(stats.generation) +
      " | Best Fitness: " + juce::String(stats.bestFitness, 2) +
      " | Patch: " + stats.bestPresetName;

  if (!startButton_.getToggleState())
    statusLine += " [PAUSED]";

  SkString skStatus(statusLine.toRawUTF8());
  canvas->drawString(skStatus, displayArea.fLeft + 10, displayArea.fTop + 20, font, textPaint);
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
