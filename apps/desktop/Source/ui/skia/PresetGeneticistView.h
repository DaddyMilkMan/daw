/*
  ==============================================================================

    PresetGeneticistView.h
    Created: 2025-12-07
    Author:  Zenith DAW AI Team

    Skia-based UI for the Preset Geneticist Agent.
    Visualizes the evolution process (DNA helix, fitness stats).

  ==============================================================================
*/

#pragma once

#include "../../ai/PresetGeneticistAgent.h"
#include "SkiaComponent.h"
#include <algorithm>
#include <cmath>


namespace zenith {
namespace ui {
namespace skia {

class PresetGeneticistView
    : public zenith::SkiaComponent,
      public zenith::ai::PresetGeneticistAgent::Listener {
public:
  PresetGeneticistView(zenith::ai::PresetGeneticistAgent &agent)
      : agent_(agent) {
    agent_.addListener(this);
    setGlowEnabled(true);
    setGlowColor(zenith::design::colors::NEON_CYAN);
  }

  ~PresetGeneticistView() override { agent_.removeListener(this); }

  // SkiaComponent overrides
  void drawSkia(SkCanvas *canvas) override {
    // Background
    canvas->clear(SkColorSetARGB(255, 30, 30, 35));

    SkPaint paint;
    paint.setAntiAlias(true);

    // Title
    paint.setColor(SK_ColorWHITE);
    SkFont font(SkTypeface::MakeDefault(), 20.0f);
    canvas->drawString("Preset Geneticist: Evolution Lab", 20, 40, font, paint);

    // Status
    auto &stats = agent_.getStats();
    juce::String status =
        agent_.isRunning() ? "Breeding Population..." : "Ready";
    if (agent_.isPaused())
      status = "Paused";

    font.setSize(14.0f);
    paint.setColor(SK_ColorLTGRAY);
    canvas->drawString(status.toStdString().c_str(), 20, 70, font, paint);

    juce::String genText = "Generation: " + juce::String(stats.generation);
    canvas->drawString(genText.toStdString().c_str(), 20, 90, font, paint);

    juce::String fitnessText =
        "Best Fitness: " + juce::String(stats.bestFitness, 2);
    canvas->drawString(fitnessText.toStdString().c_str(), 20, 110, font, paint);

    juce::String presetText = "Best Patch: " + stats.bestPresetName;
    canvas->drawString(presetText.toStdString().c_str(), 20, 130, font, paint);

    // Draw DNA Helix Visualization
    drawDNAHelix(canvas, 300, 100, 200, 400);

    // Draw Population Grid (simulated visuals of individuals)
    drawPopulationGrid(canvas, 20, 160, 250, 200);
  }

  // Listener overrides
  void generationCompleted(int generation, float bestFitness) override {
    markDirty(); // Trigger repaint on main thread
  }

  void
  evolutionCompleted(const std::vector<zenith::ai::Individual> &best) override {
    markDirty();
  }

  void individualEvaluated(const zenith::ai::Individual &individual) override {
    // Optional: animate specific individual evaluation
    markDirty();
  }

  // Interaction
  void mouseDown(const juce::MouseEvent &e) override {
    // Simple click handling for "buttons" drawn in Skia
    // (In a real app, we'd use separate button components or hit testing)
    if (e.y > 380 && e.y < 420) {
      if (e.x > 20 && e.x < 100) {
        if (!agent_.isRunning())
          agent_.startEvolution();
        else
          agent_.stopEvolution();
      }
    }
    markDirty();
  }

private:
  zenith::ai::PresetGeneticistAgent &agent_;
  float phase_ = 0.0f; // For animation

  void timerCallback() override {
    if (agent_.isRunning()) {
      phase_ += 0.1f;
      markDirty(); // Animate continuously while running
    }
  }

  void drawDNAHelix(SkCanvas *canvas, float x, float y, float w, float h) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3.0f);

    // Simple sine wave helix simulation
    int steps = 20;
    float stepHeight = h / steps;

    for (int i = 0; i < steps; ++i) {
      float yPos = y + i * stepHeight;
      float offset = std::sin(phase_ + i * 0.5f) * (w * 0.4f);

      float x1 = x + w / 2 + offset;
      float x2 = x + w / 2 - offset;

      // Strand 1
      paint.setColor(zenith::design::colors::NEON_CYAN);
      canvas->drawCircle(x1, yPos, 4.0f, paint);

      // Strand 2
      paint.setColor(zenith::design::colors::NEON_MAGENTA);
      canvas->drawCircle(x2, yPos, 4.0f, paint);

      // Connection
      paint.setColor(SkColorSetA(SK_ColorWHITE, 50));
      paint.setStrokeWidth(1.0f);
      canvas->drawLine(x1, yPos, x2, yPos, paint);
    }
  }

  void drawPopulationGrid(SkCanvas *canvas, float x, float y, float w,
                          float h) {
    auto pop = agent_.getPopulation();
    if (pop.empty())
      return;

    int cols = 10;
    int rows = 5;
    float cellW = w / cols;
    float cellH = h / rows;

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);

    for (int i = 0; i < std::min((int)pop.size(), cols * rows); ++i) {
      float cx = x + (i % cols) * cellW;
      float cy = y + (i / cols) * cellH;

      // Color based on fitness
      float fitness = pop[i].fitness;

      if (pop[i].isDead) {
        paint.setColor(SkColorSetARGB(100, 255, 50, 50)); // Red for dead
      } else {
        // Gradient from Blue (low) to Green (high)
        int r = 0;
        int g = (int)(fitness * 255);
        int b = (int)((1.0f - fitness) * 255);
        paint.setColor(SkColorSetRGB(r, g, b));
      }

      canvas->drawRect(SkRect::MakeXYWH(cx + 2, cy + 2, cellW - 4, cellH - 4),
                       paint);
    }
  }
};

} // namespace skia
} // namespace ui
} // namespace zenith
