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

#include "ZenithDesignSystem.h"

// Skia Headers for Effects
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkRRect.h>
#include <include/core/SkPath.h>
#include <include/effects/SkImageFilters.h>

namespace zenith::design {

// ============================================================================
// STATIC MEMBER DEFINITIONS (Settings struct)
// ============================================================================

float Settings::glowIntensity = 1.0f;
float Settings::uiScale = 1.0f;
Settings::Theme Settings::currentTheme = Settings::Theme::NeonNoir;
Settings::BlurQuality Settings::blurQuality = Settings::BlurQuality::High;
bool Settings::reducedMotionEnabled = false;

// ============================================================================
// LAYOUT MANAGER IMPLEMENTATION
// ============================================================================

void LayoutManager::setPanelState(const juce::String &panelId,
                                  const PanelState &state) {
  panels_[panelId] = state;
}

LayoutManager::PanelState
LayoutManager::getPanelState(const juce::String &panelId) const {
  auto it = panels_.find(panelId);
  if (it != panels_.end())
    return it->second;

  // Default state if not found
  return PanelState{panelId, juce::Rectangle<float>(0, 0, 1, 1), true, 0};
}

void LayoutManager::saveLayout(const juce::String &name) {
  auto *root = new juce::DynamicObject();
  juce::Array<juce::var> panelArray;

  for (const auto &[id, state] : panels_) {
    auto *p = new juce::DynamicObject();
    p->setProperty("id", id);
    p->setProperty("x", state.relativeBounds.getX());
    p->setProperty("y", state.relativeBounds.getY());
    p->setProperty("w", state.relativeBounds.getWidth());
    p->setProperty("h", state.relativeBounds.getHeight());
    p->setProperty("visible", state.isVisible);
    panelArray.add(juce::var(p));
  }

  root->setProperty("panels", panelArray);

  juce::File file = getLayoutDir().getChildFile(name + ".layout");
  file.replaceWithText(juce::JSON::toString(juce::var(root)));
}

void LayoutManager::loadLayout(const juce::String &name) {
  juce::File file = getLayoutDir().getChildFile(name + ".layout");
  if (!file.existsAsFile())
    return;

  auto json = juce::JSON::parse(file);
  if (auto *root = json.getDynamicObject()) {
    auto panelArray = root->getProperty("panels").getArray();
    if (panelArray) {
      for (auto &v : *panelArray) {
        if (auto *p = v.getDynamicObject()) {
          juce::String id = p->getProperty("id").toString();
          PanelState state;
          state.id = id;
          state.relativeBounds = juce::Rectangle<float>(
              (float)p->getProperty("x"), (float)p->getProperty("y"),
              (float)p->getProperty("w"), (float)p->getProperty("h"));
          state.isVisible = (bool)p->getProperty("visible");
          panels_[id] = state;
        }
      }
    }
  }
}

} // namespace zenith::design

// ============================================================================
// EFFECT HELPER IMPLEMENTATIONS
// ============================================================================

void zenith::design::drawGlassPanel(SkCanvas* canvas, const juce::Rectangle<float>& bounds, float cornerRad, float panelOpacity) {
    if (!canvas) return;

    SkRect rect = SkRect::MakeLTRB(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getBottom());
    
    // 1. REAL Backdrop Blur using saveLayer with a backdrop filter
    if (Settings::getBlurQuality() != Settings::BlurQuality::Off) {
        float sigma = effects::BLUR_GLASS * (static_cast<float>(Settings::getBlurQuality()) / 3.0f);
        sk_sp<SkImageFilter> blurFilter = SkImageFilters::Blur(sigma, sigma, SkTileMode::kClamp, nullptr);
        
        SkPaint layerPaint;
        layerPaint.setImageFilter(blurFilter);
        
        canvas->saveLayer(nullptr, &layerPaint);
        
        // Draw a clip rect to limit the blur region
        SkRRect roundRect = SkRRect::MakeRectXY(rect, cornerRad, cornerRad);
        canvas->clipRRect(roundRect, true);
        
        canvas->restore();
    }
    
    // 2. Semi-transparent overlay (the "glass tint")
    SkPaint glassPaint;
    glassPaint.setAntiAlias(true);
    glassPaint.setColor(SkColorSetARGB(
        static_cast<uint8_t>(panelOpacity * 255), 
        SkColorGetR(colors::BG_DARKER),
        SkColorGetG(colors::BG_DARKER),
        SkColorGetB(colors::BG_DARKER)));
    
    SkRRect glassRRect = SkRRect::MakeRectXY(rect, cornerRad, cornerRad);
    canvas->drawRRect(glassRRect, glassPaint);
    
    // 3. Top highlight edge (subtle)
    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setColor(0x15FFFFFF); // 8% white
    highlightPaint.setStyle(SkPaint::kStroke_Style);
    highlightPaint.setStrokeWidth(1.0f);
    
    SkPath highlightPath;
    highlightPath.moveTo(rect.fLeft + cornerRad, rect.fTop);
    highlightPath.lineTo(rect.fRight - cornerRad, rect.fTop);
    canvas->drawPath(highlightPath, highlightPaint);
    
    // 4. Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(colors::BORDER_SUBTLE);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawRRect(glassRRect, borderPaint);
}
