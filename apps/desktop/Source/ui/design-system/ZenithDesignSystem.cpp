/*
  ==============================================================================

    ZenithDesignSystem.cpp
    Created: 2025-12-01
    Author:  Zenith DAW

    Implementation of global design settings.

  ==============================================================================
*/

#include "ZenithDesignSystem.h"

// Skia Headers for Effects
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkMaskFilter.h>
#include <include/effects/SkImageFilters.h>
#include <include/core/SkPath.h>

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
// THEME MANAGER IMPLEMENTATION
// ============================================================================

void ThemeManager::saveTheme(const juce::String &name) {
  auto *obj = new juce::DynamicObject();

  // Save current colors
  obj->setProperty("CYAN", (juce::int64)colors::CYAN);
  obj->setProperty("MAGENTA", (juce::int64)colors::MAGENTA);
  obj->setProperty("NEON_GREEN", (juce::int64)colors::NEON_GREEN);
  obj->setProperty("BG_DARKEST", (juce::int64)colors::BG_DARKEST);
  obj->setProperty("BG_DARKER", (juce::int64)colors::BG_DARKER);
  obj->setProperty("BG_DARK", (juce::int64)colors::BG_DARK);

  juce::File file = getThemeDir().getChildFile(name + ".json");
  file.replaceWithText(juce::JSON::toString(juce::var(obj)));
}

void ThemeManager::loadTheme(const juce::String &name) {
  juce::File file = getThemeDir().getChildFile(name + ".json");
  if (!file.existsAsFile())
    return;

  auto json = juce::JSON::parse(file);
  if (auto *obj = json.getDynamicObject()) {
    Theme theme;
    theme.name = name;

    if (obj->hasProperty("CYAN"))
      colors::CYAN = (uint32_t)(int)obj->getProperty("CYAN");
    if (obj->hasProperty("MAGENTA"))
      colors::MAGENTA = (uint32_t)(int)obj->getProperty("MAGENTA");
    if (obj->hasProperty("NEON_GREEN"))
      colors::NEON_GREEN = (uint32_t)(int)obj->getProperty("NEON_GREEN");
    if (obj->hasProperty("BG_DARKEST"))
      colors::BG_DARKEST = (uint32_t)(int)obj->getProperty("BG_DARKEST");
    if (obj->hasProperty("BG_DARKER"))
      colors::BG_DARKER = (uint32_t)(int)obj->getProperty("BG_DARKER");
    if (obj->hasProperty("BG_DARK"))
      colors::BG_DARK = (uint32_t)(int)obj->getProperty("BG_DARK");

    // Trigger repaint globally (would need a listener, but for now relies on
    // repaint calls)
  }
}

void ThemeManager::deleteTheme(const juce::String &name) {
  juce::File file = getThemeDir().getChildFile(name + ".json");
  file.deleteFile();
}

juce::StringArray ThemeManager::getAvailableThemes() const {
  juce::StringArray themes;
  auto files =
      getThemeDir().findChildFiles(juce::File::findFiles, false, "*.json");
  for (const auto &f : files) {
    themes.add(f.getFileNameWithoutExtension());
  }
  return themes;
}

// ============================================================================
// THEME PRESET IMPLEMENTATION
// ============================================================================

ThemeManager::ThemeManager() {
  // Initialize with Dark theme
  applyDarkTheme();
}

void ThemeManager::setActiveTheme(ThemePreset preset) {
  if (activePreset_ == preset)
    return;

  activePreset_ = preset;

  switch (preset) {
  case ThemePreset::Dark:
    applyDarkTheme();
    break;
  case ThemePreset::Darker:
    applyDarkerTheme();
    break;
  case ThemePreset::Light:
    applyLightTheme();
    break;
  }

  // Update global design::colors namespace
  colors::BG_DARKEST = currentPalette_.bgDarkest;
  colors::BG_DARKER = currentPalette_.bgDarker;
  colors::BG_DARK = currentPalette_.bgDark;
  colors::BG_MEDIUM = currentPalette_.bgMedium;
  colors::BG_LIGHT = currentPalette_.bgLight;
  colors::CYAN = currentPalette_.accentPrimary;
  colors::ACCENT_PRIMARY = currentPalette_.accentPrimary;
  colors::MAGENTA = currentPalette_.accentSecondary;
  colors::TEXT_PRIMARY = currentPalette_.textPrimary;
  colors::TEXT_SECONDARY = currentPalette_.textSecondary;
  colors::TEXT_TERTIARY = currentPalette_.textTertiary;
  colors::BORDER_DEFAULT = currentPalette_.borderDefault;
  colors::BORDER_SUBTLE = currentPalette_.borderSubtle;
  colors::BORDER_FOCUS = currentPalette_.borderFocus;
  colors::GREEN = currentPalette_.success;
  colors::AMBER = currentPalette_.warning;
  colors::RED = currentPalette_.error;

  notifyListeners();
  sendChangeMessage(); // JUCE ChangeBroadcaster
}

void ThemeManager::addListener(ThemeListener *listener) {
  if (listener && std::find(listeners_.begin(), listeners_.end(), listener) ==
                      listeners_.end()) {
    listeners_.push_back(listener);
  }
}

void ThemeManager::removeListener(ThemeListener *listener) {
  listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener),
                   listeners_.end());
}

void ThemeManager::notifyListeners() {
  for (auto *listener : listeners_) {
    if (listener)
      listener->themeChanged(activePreset_);
  }
}

void ThemeManager::applyDarkTheme() {
  // Premium matte-black theme with restrained electric-blue accents
  currentPalette_.bgDarkest = 0xFF0D0D11; // Opaque main window
  currentPalette_.bgDarker = 0xFF141419;  // Opaque sections
  currentPalette_.bgDark = 0xFF1C1C24;    // Opaque containers
  currentPalette_.bgMedium = 0xE625252D;  // ~90% Opacity for panels
  currentPalette_.bgLight = 0xD92F2F3D;   // ~85% Opacity for overlays

  currentPalette_.accentPrimary = 0xFF3B82F6;   // Professional Blue
  currentPalette_.accentSecondary = 0xFF8B5CF6; // Muted Violet

  currentPalette_.textPrimary = 0xFFF2F2F7;
  currentPalette_.textSecondary = 0xFFA1A1AA;
  currentPalette_.textTertiary = 0xFF71717A;

  currentPalette_.borderDefault = 0x1FFFFFFF;
  currentPalette_.borderSubtle = 0x0FFFFFFF;
  currentPalette_.borderFocus = 0xFF3B82F6;

  currentPalette_.success = 0xFF32D74B;
  currentPalette_.warning = 0xFFFFAB00;
  currentPalette_.error = 0xFFFF453A;

  // Update design::colors with neon-specific variations
  colors::CYAN_GLOW = withAlpha(currentPalette_.accentPrimary, opacity::GLOW_MEDIUM);
  colors::MAGENTA_GLOW = withAlpha(currentPalette_.accentSecondary, opacity::GLOW_MEDIUM);
}

void ThemeManager::applyDarkerTheme() {
  // OLED Black - pure black for power saving, subtle accents
  currentPalette_.bgDarkest = 0xFF000000;
  currentPalette_.bgDarker = 0xFF080808;
  currentPalette_.bgDark = 0xFF101010;
  currentPalette_.bgMedium = 0xFF181818;
  currentPalette_.bgLight = 0xFF202020;

  currentPalette_.accentPrimary = 0xFF2F7CF6;   // Deep Blue
  currentPalette_.accentSecondary = 0xFF7C3AED; // Deep Violet

  currentPalette_.textPrimary = 0xFFE8E8E8;
  currentPalette_.textSecondary = 0xFF888888;
  currentPalette_.textTertiary = 0xFF555555;

  currentPalette_.borderDefault = 0x18FFFFFF;
  currentPalette_.borderSubtle = 0x0AFFFFFF;
  currentPalette_.borderFocus = 0xFF2F7CF6;

  currentPalette_.success = 0xFF00C853;
  currentPalette_.warning = 0xFFFF9800;
  currentPalette_.error = 0xFFFF3D00;

  // Muted glows for OLED to preserve battery and reduce burn-in risk
  colors::CYAN_GLOW = withAlpha(currentPalette_.accentPrimary, opacity::GLOW_SUBTLE);
  colors::MAGENTA_GLOW = withAlpha(currentPalette_.accentSecondary, opacity::GLOW_SUBTLE);
}

void ThemeManager::applyLightTheme() {
  // Light mode - inverted palette for daylight use
  currentPalette_.bgDarkest = 0xFFFFFFFF;
  currentPalette_.bgDarker = 0xFFF8F8FA;
  currentPalette_.bgDark = 0xFFF0F0F4;
  currentPalette_.bgMedium = 0xFFE8E8EC;
  currentPalette_.bgLight = 0xFFDCDCE0;

  currentPalette_.accentPrimary = 0xFF0066CC;   // Deep Blue
  currentPalette_.accentSecondary = 0xFF7C3AED; // Purple

  currentPalette_.textPrimary = 0xFF1A1A1A;
  currentPalette_.textSecondary = 0xFF666666;
  currentPalette_.textTertiary = 0xFF999999;

  currentPalette_.borderDefault = 0x20000000;
  currentPalette_.borderSubtle = 0x10000000;
  currentPalette_.borderFocus = 0xFF0066CC;

  currentPalette_.success = 0xFF059669;
  currentPalette_.warning = 0xFFD97706;
  currentPalette_.error = 0xFFDC2626;
}


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
void zenith::design::ThemeManager::resetToDefault() {
  listeners_.clear();
  setActiveTheme(zenith::design::ThemePreset::Dark);
}

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
        
        // saveLayer with a backdrop filter blurs everything ALREADY on the canvas within these bounds
        SkCanvas::SaveLayerRec rec(&rect, nullptr, blurFilter.get(), 0);
        canvas->saveLayer(rec);
        
        // Now we are inside the layer. We just need to draw the tint.
        SkPaint tintPaint;
        tintPaint.setAntiAlias(true);
        tintPaint.setColor(withAlpha(colors::BG_02, panelOpacity));
        canvas->drawRoundRect(rect, cornerRad, cornerRad, tintPaint);
        
        canvas->restore();
    } else {
        // Fallback: Just draw the panel without blur
        SkPaint flatPaint;
        flatPaint.setAntiAlias(true);
        flatPaint.setColor(withAlpha(colors::BG_02, panelOpacity));
        canvas->drawRoundRect(rect, cornerRad, cornerRad, flatPaint);
    }

    // 2. Rim Light (Highlight top and left edges)
    SkPaint rimPaint;
    rimPaint.setAntiAlias(true);
    rimPaint.setStyle(SkPaint::kStroke_Style);
    rimPaint.setStrokeWidth(1.0f);
    rimPaint.setColor(colors::GLASS_HIGHLIGHT);
    canvas->drawRoundRect(rect, cornerRad, cornerRad, rimPaint);
    
    // 3. Subtle Shadow
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setStyle(SkPaint::kStroke_Style);
    shadowPaint.setStrokeWidth(1.0f);
    shadowPaint.setColor(colors::GLASS_SHADOW);
    SkRect shadowRect = rect.makeOffset(0.0f, 1.0f);
    canvas->drawRoundRect(shadowRect, cornerRad, cornerRad, shadowPaint);
}

void zenith::design::drawGlowLine(SkCanvas* canvas, float x1, float y1, float x2, float y2, SkColor glowColor, float thickness, float radius) {
    if (!canvas) return;

    // 1. Draw the Glow (Bloom)
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStrokeWidth(thickness + radius);
    glowPaint.setColor(withAlpha(glowColor, opacity::GLOW_SUBTLE));
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, radius * 0.5f));
    canvas->drawLine(x1, y1, x2, y2, glowPaint);

    // 2. Draw the Core Line
    SkPaint corePaint;
    corePaint.setAntiAlias(true);
    corePaint.setStrokeWidth(thickness);
    corePaint.setColor(glowColor);
    canvas->drawLine(x1, y1, x2, y2, corePaint);
}

void zenith::design::drawGlowRect(SkCanvas* canvas, const juce::Rectangle<float>& bounds, SkColor glowColor, float cornerRad, float radius) {
    if (!canvas) return;

    SkRect rect = SkRect::MakeLTRB(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getBottom());

    // 1. Draw the Glow (Bloom)
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(radius);
    glowPaint.setColor(withAlpha(glowColor, opacity::GLOW_SUBTLE));
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, radius * 0.5f));
    canvas->drawRoundRect(rect, cornerRad, cornerRad, glowPaint);

    // 2. Draw Core Rect Border
    SkPaint corePaint;
    corePaint.setAntiAlias(true);
    corePaint.setStyle(SkPaint::kStroke_Style);
    corePaint.setStrokeWidth(1.0f);
    corePaint.setColor(glowColor);
    canvas->drawRoundRect(rect, cornerRad, cornerRad, corePaint);
}
