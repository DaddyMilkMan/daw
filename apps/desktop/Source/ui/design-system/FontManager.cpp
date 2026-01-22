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
#include <core/SkData.h>
#include <core/SkStream.h>

// Platform-specific font manager includes
#include "PlatformFontUtils.h"

namespace zenith {
namespace design {

// Helper to map weight for indexing
static int getWeightIndex(FontWeight weight) {
  switch (weight) {
  case FontWeight::Regular:
    return 0;
  case FontWeight::Medium:
    return 1;
  case FontWeight::SemiBold:
    return 2;
  case FontWeight::Bold:
    return 3;
  default:
    // Unknown weight - fallback to Regular. This handles future weight additions
    // gracefully without crashing.
    DBG("[FontManager] Unknown FontWeight value: " + juce::String(static_cast<int>(weight)) +
        ", falling back to Regular");
    return 0;
  }
}
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

  // Get font manager - use platform-specific implementations for best results
  fontMgr_ = PlatformFontUtils::createDefaultFontManager();

  // Determine font resource directory
  // Robust search strategy for various deployment scenarios
  juce::File exeDir =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getParentDirectory();
  
  // Potential font locations in order of preference
  juce::Array<juce::File> candidateDirs;

  // 1. Standard Install / App Bundle (exe/Resources/fonts)
  candidateDirs.add(exeDir.getChildFile("Resources/fonts"));

  // 2. Development Build (relative to build/bin output)
  // Assuming build/debug/bin, so ../../../apps/desktop/Resources/fonts
  candidateDirs.add(exeDir.getChildFile("../../../apps/desktop/Resources/fonts"));
  
  // 3. Fallback Development (relative to standard CMake build folder)
  candidateDirs.add(exeDir.getChildFile("../../apps/desktop/Resources/fonts"));

  // 4. Current Working Directory (CLI usage)
  candidateDirs.add(juce::File::getCurrentWorkingDirectory().getChildFile(
        "apps/desktop/Resources/fonts"));
        
  // 5. User AppData (e.g. C:/Users/Name/AppData/Roaming/ZenithDAW/Fonts)
  candidateDirs.add(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW/Fonts"));

  // Find first valid directory
  bool found = false;
  for (const auto& dir : candidateDirs) {
    if (dir.isDirectory()) {
        fontDir_ = dir;
        found = true;
        break;
    }
  }

  if (!fontDir_.isDirectory()) {
    DBG("[FontManager] WARNING: Font directory not found. Tried: "
        << fontDir_.getFullPathName());
    DBG("[FontManager] Custom fonts will not be available - using system fallback");
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
  bool monoRegular = loadFont("JetBrainsMono-Regular.ttf", FontFamily::Mono, FontWeight::Regular);
  bool monoMedium = loadFont("JetBrainsMono-Medium.ttf", FontFamily::Mono, FontWeight::Medium);
  bool monoSemiBold = loadFont("JetBrainsMono-SemiBold.ttf", FontFamily::Mono, FontWeight::SemiBold);
  bool monoBold = loadFont("JetBrainsMono-Bold.ttf", FontFamily::Mono, FontWeight::Bold);

  // Create synthetic weight fallbacks for missing fonts
  // This ensures getFont() always returns a usable typeface
  int monoIdx = static_cast<int>(FontFamily::Mono);

  // If SemiBold missing, use Medium as fallback
  if (!monoSemiBold && monoMedium) {
    typefaces_[monoIdx][2] = typefaces_[monoIdx][1]; // SemiBold = Medium
    DBG("[FontManager] Using Medium as SemiBold fallback for Mono");
  }
  // If SemiBold still missing but Bold exists, use Bold
  if (!typefaces_[monoIdx][2] && monoBold) {
    typefaces_[monoIdx][2] = typefaces_[monoIdx][3]; // SemiBold = Bold
    DBG("[FontManager] Using Bold as SemiBold fallback for Mono");
  }

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
    DBG("[FontManager]   JetBrainsMono-Regular: " << (monoRegular ? "OK" : "FAILED"));
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
  if (!fontMgr_) return false;
  
  sk_sp<SkTypeface> typeface = fontMgr_->makeFromData(skData, 0);
  if (!typeface) {
    DBG("[FontManager] Failed to create typeface from: " << filename);
    return false;
  }

  // Store in cache
  int familyIdx = static_cast<int>(family);
  int weightIdx = getWeightIndex(weight);

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

  int weightIdx = getWeightIndex(weight);

  if (familyIdx >= 0 && familyIdx < kNumFamilies && weightIdx >= 0 &&
      weightIdx < kNumWeights) {
    return typefaces_[familyIdx][weightIdx];
  }

  return nullptr;
}

void FontManager::configureFont(SkFont &font) const {
  // Configure for optimal rendering
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

  // CRITICAL: If still no typeface, use system default to prevent crashes
  // SkFont with null typeface can cause SIGSEGV in FreeType when measuring text
  if (!typeface && fontMgr_) {
    // Try to get a system font as fallback
    typeface = fontMgr_->matchFamilyStyle("sans-serif", SkFontStyle::Normal());
    if (!typeface) {
      typeface = fontMgr_->matchFamilyStyle(nullptr, SkFontStyle::Normal());
    }
  }

  SkFont font;

  if (typeface) {
    font.setTypeface(typeface);
  }
  // Note: If typeface is still null, SkFont will use its internal default

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
