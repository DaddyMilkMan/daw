/*
  ==============================================================================

    AutoSaveIndicator.h
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include "../../engine/ProjectState.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/SkiaComponent.h"
#include <JuceHeader.h>

namespace zenith {

class AutoSaveIndicator : public SkiaComponent {
public:
  AutoSaveIndicator(ProjectState &state) : projectState(state) {
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(500); // Check state every 500ms
    setWantsKeyboardFocus(false);
  }

  ~AutoSaveIndicator() override { stopTimer(); }

  void timerCallback() override {
    SkiaComponent::timerCallback(); // Call base implementation first

    bool dirty = projectState.hasUnsavedChanges();

    // DESIGN NOTE: We show dirty/clean indicated with pulse animation.
    // A real-time "Saving..." state would require ProjectFileIO to expose
    // an isSaving atomic flag. The current UX with pulse animation provides
    // sufficient feedback for the "unsaved changes" state without the
    // complexity of tracking save-in-progress across threads.

    if (dirty != wasDirty) {
      wasDirty = dirty;
      repaint();
    }

    // Pulse animation if dirty
    if (dirty) {
      pulsePhase += 0.1f;
      if (pulsePhase > juce::MathConstants<float>::twoPi) {
        pulsePhase -= juce::MathConstants<float>::twoPi;
      }
      repaint();
    }
  }

  void drawSkia(SkCanvas *canvas) override {
    auto bounds = getLocalBounds().toFloat();
    float cy = bounds.getHeight() / 2.0f;
    float cx = 10.0f;
    float radius = 4.0f;

    SkPaint paint;
    paint.setAntiAlias(true);

    if (wasDirty) {
      // Unsaved: Amber pulse with aura
      float pulse = 0.6f + 0.4f * std::sin(pulsePhase);
      SkColor amber = design::colors::WARNING;
      
      // Aura
      paint.setColor(design::withAlpha(amber, 0.3f * pulse));
      canvas->drawCircle(cx, cy, radius + 3.0f, paint);

      // Core
      paint.setColor(amber);
      canvas->drawCircle(cx, cy, radius, paint);

      // Text: "Unsaved"
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(design::colors::TEXT_SECONDARY);
      auto font = design::typography::getSkFont(design::typography::FONT_XS, design::typography::FontWeight::Medium);
      canvas->drawString("Unsaved", cx + 12.0f, cy + 4.0f, font, textPaint);
    } else {
      // Saved: Emerald static
      SkColor emerald = design::colors::SUCCESS;
      
      // Core
      paint.setColor(emerald);
      canvas->drawCircle(cx, cy, radius, paint);

      // Text: "Saved"
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(design::colors::TEXT_TERTIARY);
      auto font = design::typography::getSkFont(design::typography::FONT_XS, design::typography::FontWeight::Medium);
      canvas->drawString("Saved", cx + 12.0f, cy + 4.0f, font, textPaint);
    }
  }

private:
  ProjectState &projectState;
  bool wasDirty = false;
  float pulsePhase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoSaveIndicator)
};

} // namespace zenith
