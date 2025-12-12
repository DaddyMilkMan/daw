/*
  ==============================================================================

    FontManager.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the professional custom font management system.

  ==============================================================================
*/

#include "FontManager.h"
#include <core/SkFontTypes.h>
#include <include/core/SkData.h>
#include <include/core/SkStream.h>


// Platform-specific font manager includes
#ifdef _WIN32
#include <include/ports/SkTypeface_win.h>
#endif

namespace zenith {
namespace design {

// ============================================================================
// SINGLETON ACCESS
// ============================================================================

FontManager &FontManager::getInstance() {
  static FontManager instance;
  return instance;
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

FontManager::FontManager() { initialize(); }

// ============================================================================
// INITIALIZATION
// ============================================================================

void FontManager::initialize() {
  std::lock_guard<std::mutex> lock(mutex_);

  // Get font manager - use DirectWrite on Windows for best results
#ifdef _WIN32
  fontMgr_ = SkFontMgr_New_DirectWrite();
  if (!fontMgr_) {
    // Fallback if DirectWrite fails
    fontMgr_ = SkFontMgr::RefEmpty();
    DBG("[FontManager] WARNING: DirectWrite font manager unavailable, using "
        "empty manager");
  }
#else
  fontMgr_ = SkFontMgr::RefEmpty();
#endif

  // Determine font resource directory
  // Try multiple locations for robustness
  juce::File appDir =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getParentDirectory();

  // Check for development layout (running from build directory)
  fontDir_ = appDir.getChildFile("../../apps/desktop/Resources/fonts");
  if (!fontDir_.isDirectory()) {
    // Check for installed layout
    fontDir_ = appDir.getChildFile("Resources/fonts");
  }
  if (!fontDir_.isDirectory()) {
    // Check relative to working directory
    fontDir_ = juce::File::getCurrentWorkingDirectory().getChildFile(
        "apps/desktop/Resources/fonts");
  }
  if (!fontDir_.isDirectory()) {
    // Final fallback - absolute path for development
    fontDir_ = juce::File("C:/zenith/daw/apps/desktop/Resources/fonts");
  }

  if (!fontDir_.isDirectory()) {
    DBG("[FontManager] WARNING: Font directory not found. Tried: "
        << fontDir_.getFullPathName());
    DBG("[FontManager] Custom fonts will not be available - using system "
        "fallback");
    fontsLoaded_ = false;
    return;
  }

  DBG("[FontManager] Loading fonts from: " << fontDir_.getFullPathName());

  // Load Inter fonts (UI family)
  bool interRegular =
      loadFont("Inter-Regular.ttf", FontFamily::UI, FontWeight::Regular);
  bool interMedium =
      loadFont("Inter-Medium.ttf", FontFamily::UI, FontWeight::Medium);
  bool interSemiBold =
      loadFont("Inter-SemiBold.ttf", FontFamily::UI, FontWeight::SemiBold);
  bool interBold = loadFont("Inter-Bold.ttf", FontFamily::UI, FontWeight::Bold);

  // Load JetBrains Mono fonts (Mono family)
  bool monoRegular = loadFont("JetBrainsMono-Regular.ttf", FontFamily::Mono,
                              FontWeight::Regular);
  bool monoMedium = loadFont("JetBrainsMono-Medium.ttf", FontFamily::Mono,
                             FontWeight::Medium);
  bool monoBold =
      loadFont("JetBrainsMono-Bold.ttf", FontFamily::Mono, FontWeight::Bold);

  // Display family shares typefaces with UI family
  // (Could load InterDisplay variants in the future for optical sizing)
  int uiFamilyIdx = static_cast<int>(FontFamily::UI);
  int displayFamilyIdx = static_cast<int>(FontFamily::Display);
  for (int w = 0; w < kNumWeights; ++w) {
    typefaces_[displayFamilyIdx][w] = typefaces_[uiFamilyIdx][w];
  }

  // Check if we have at least the essential fonts
  fontsLoaded_ = interRegular && monoRegular;

  if (fontsLoaded_) {
    DBG("[FontManager] Font initialization complete. Loaded "
        << getLoadedTypefaceCount() << " typefaces.");
  } else {
    DBG("[FontManager] WARNING: Some essential fonts failed to load.");
    DBG("[FontManager]   Inter-Regular: " << (interRegular ? "OK" : "FAILED"));
    DBG("[FontManager]   JetBrainsMono-Regular: " << (monoRegular ? "OK"
                                                                  : "FAILED"));
  }
}

bool FontManager::loadFont(const juce::String &filename, FontFamily family,
                           FontWeight weight) {
  juce::File fontFile = fontDir_.getChildFile(filename);

  if (!fontFile.existsAsFile()) {
    DBG("[FontManager] Font file not found: " << fontFile.getFullPathName());
    return false;
  }

  // Load font data
  juce::MemoryBlock fontData;
  if (!fontFile.loadFileAsData(fontData)) {
    DBG("[FontManager] Failed to read font file: " << filename);
    return false;
  }

  // Create Skia data from font file contents
  sk_sp<SkData> skData =
      SkData::MakeWithCopy(fontData.getData(), fontData.getSize());
  if (!skData) {
    DBG("[FontManager] Failed to create SkData for: " << filename);
    return false;
  }

  // Create typeface from data
  sk_sp<SkTypeface> typeface = fontMgr_->makeFromData(skData, 0);
  if (!typeface) {
    DBG("[FontManager] Failed to create typeface from: " << filename);
    return false;
  }

  // Store in cache
  int familyIdx = static_cast<int>(family);
  int weightIdx = 0;
  switch (weight) {
  case FontWeight::Regular:
    weightIdx = 0;
    break;
  case FontWeight::Medium:
    weightIdx = 1;
    break;
  case FontWeight::SemiBold:
    weightIdx = 2;
    break;
  case FontWeight::Bold:
    weightIdx = 3;
    break;
  }

  typefaces_[familyIdx][weightIdx] = std::move(typeface);

  DBG("[FontManager] Loaded: " << filename);
  return true;
}

// ============================================================================
// FONT ACCESS
// ============================================================================

sk_sp<SkTypeface> FontManager::getTypeface(FontFamily family,
                                           FontWeight weight) const {
  int familyIdx = static_cast<int>(family);

  // Map Display to UI family (they share typefaces)
  if (family == FontFamily::Display) {
    familyIdx = static_cast<int>(FontFamily::UI);
  }

  int weightIdx = 0;
  switch (weight) {
  case FontWeight::Regular:
    weightIdx = 0;
    break;
  case FontWeight::Medium:
    weightIdx = 1;
    break;
  case FontWeight::SemiBold:
    weightIdx = 2;
    break;
  case FontWeight::Bold:
    weightIdx = 3;
    break;
  }

  if (familyIdx >= 0 && familyIdx < kNumFamilies && weightIdx >= 0 &&
      weightIdx < kNumWeights) {
    return typefaces_[familyIdx][weightIdx];
  }

  return nullptr;
}

void FontManager::configureFont(SkFont &font) const {
  // Configure for optimal rendering on Windows
  // Per research: setEdging(kSubpixelAntiAlias), setSubpixel(true), and slight
  // hinting
  font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
  font.setSubpixel(true);
  font.setHinting(static_cast<SkFontHinting>(1));

  // Enable linear metrics for consistent glyph positioning
  font.setLinearMetrics(true);

  // Baseline snapping for pixel-perfect alignment
  font.setBaselineSnap(true);
}

SkFont FontManager::getFont(FontFamily family, FontWeight weight,
                            float size) const {
  std::lock_guard<std::mutex> lock(mutex_);

  sk_sp<SkTypeface> typeface = getTypeface(family, weight);

  if (!typeface) {
    // Fallback to Regular weight if requested weight not available
    typeface = getTypeface(family, FontWeight::Regular);
  }

  if (!typeface && family != FontFamily::UI) {
    // Fallback to UI family if other family not available
    typeface = getTypeface(FontFamily::UI, weight);
    if (!typeface) {
      typeface = getTypeface(FontFamily::UI, FontWeight::Regular);
    }
  }

  SkFont font;

  if (typeface) {
    font.setTypeface(typeface);
  }
  // If no typeface available, SkFont uses default (system font fallback)

  font.setSize(size);
  configureFont(font);

  return font;
}

SkFont FontManager::getUIFont(float size, FontWeight weight) const {
  return getFont(FontFamily::UI, weight, size);
}

SkFont FontManager::getMonoFont(float size, FontWeight weight) const {
  return getFont(FontFamily::Mono, weight, size);
}

SkFont FontManager::getDisplayFont(float size, FontWeight weight) const {
  return getFont(FontFamily::Display, weight, size);
}

// ============================================================================
// STATUS
// ============================================================================

int FontManager::getLoadedTypefaceCount() const {
  std::lock_guard<std::mutex> lock(mutex_);

  int count = 0;
  for (int f = 0; f < kNumFamilies; ++f) {
    for (int w = 0; w < kNumWeights; ++w) {
      if (typefaces_[f][w]) {
        ++count;
      }
    }
  }

  // Subtract display family since it shares with UI
  // (Don't double-count)
  for (int w = 0; w < kNumWeights; ++w) {
    if (typefaces_[static_cast<int>(FontFamily::Display)][w] ==
            typefaces_[static_cast<int>(FontFamily::UI)][w] &&
        typefaces_[static_cast<int>(FontFamily::Display)][w]) {
      --count;
    }
  }

  return count;
}

juce::File FontManager::getFontResourceDirectory() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return fontDir_;
}

} // namespace design
} // namespace zenith
