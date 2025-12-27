/*
  ==============================================================================

    FontManager.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Professional custom font management system for Zenith DAW.
    Provides centralized font loading, caching, and configuration with
    proper subpixel antialiasing for crisp text rendering on Windows.

    Fonts used:
    - Inter: Primary UI font (Regular, Medium, SemiBold, Bold)
    - JetBrains Mono: Monospace font for code/timing displays

  ==============================================================================
*/

#pragma once

#include <array>
#include <include/core/SkRefCnt.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkTypeface.h>
#include <juce_core/juce_core.h>
#include <mutex>

namespace zenith {
namespace design {

// ============================================================================
// FONT ENUMERATIONS
// ============================================================================

/**
 * Font family selection.
 * - UI: Inter font for general interface text
 * - Mono: JetBrains Mono for code, timing displays, and fixed-width text
 * - Display: Inter font for large headings (may use Display optical size in
 * future)
 */
enum class FontFamily {
  UI,     // Inter - UI text
  Mono,   // JetBrains Mono - fixed-width
  Display // Inter - large headings (uses Inter for now)
};

/**
 * Font weight selection.
 * Maps to standard CSS font-weight values.
 */
enum class FontWeight {
  Regular = 400,
  Medium = 500,
  SemiBold = 600,
  Bold = 700
};

// ============================================================================
// FONT MANAGER SINGLETON
// ============================================================================

/**
 * Thread-safe singleton for managing custom fonts.
 *
 * Loads fonts from the Resources/fonts directory at startup and caches
 * SkTypeface objects for efficient reuse. All SkFont objects returned
 * are configured with optimal rendering settings for Windows.
 *
 * Thread Safety: All public methods are thread-safe and can be called
 * from any thread, including the audio thread for value displays.
 */
class FontManager final {
public:
  /**
   * Get the singleton instance.
   * Thread-safe via Meyer's singleton pattern.
   */
  static FontManager &getInstance();

  // Delete copy/move
  FontManager(const FontManager &) = delete;
  FontManager &operator=(const FontManager &) = delete;
  FontManager(FontManager &&) = delete;
  FontManager &operator=(FontManager &&) = delete;

  // ========================================================================
  // FONT ACCESS
  // ========================================================================

  /**
   * Get a configured SkFont for the specified family, weight, and size.
   *
   * @param family  Font family (UI, Mono, Display)
   * @param weight  Font weight (Regular, Medium, SemiBold, Bold)
   * @param size    Font size in points
   * @return        Configured SkFont with subpixel antialiasing enabled
   */
  SkFont getFont(FontFamily family, FontWeight weight, float size) const;

  /**
   * Get the UI font (Inter) with specified weight and size.
   * Convenience wrapper for getFont(FontFamily::UI, ...).
   */
  SkFont getUIFont(float size, FontWeight weight = FontWeight::Regular) const;

  /**
   * Get the monospace font (JetBrains Mono) with specified size.
   * Convenience wrapper for getFont(FontFamily::Mono, ...).
   */
  SkFont getMonoFont(float size, FontWeight weight = FontWeight::Regular) const;

  /**
   * Get the display font (Inter) with specified weight and size.
   * Convenience wrapper for getFont(FontFamily::Display, ...).
   */
  SkFont getDisplayFont(float size, FontWeight weight = FontWeight::Bold) const;

  // ========================================================================
  // STATUS
  // ========================================================================

  /**
   * Check if fonts were loaded successfully.
   * If false, the system will fall back to default Skia fonts.
   */
  bool isInitialized() const { return fontsLoaded_; }

  /**
   * Get the number of successfully loaded typefaces.
   */
  int getLoadedTypefaceCount() const;

  /**
   * Get the font resource directory path.
   */
  juce::File getFontResourceDirectory() const;

private:
  FontManager();
  ~FontManager() = default;

  /**
   * Initialize fonts from the Resources/fonts directory.
   * Called from constructor.
   */
  void initialize();

  /**
   * Load a single font file and cache the typeface.
   *
   * @param filename  Font filename (e.g., "Inter-Regular.ttf")
   * @param family    Target font family
   * @param weight    Target font weight
   * @return          True if loaded successfully
   */
  bool loadFont(const juce::String &filename, FontFamily family,
                FontWeight weight);

  /**
   * Get the cached typeface for a family/weight combination.
   * Returns nullptr if not loaded.
   */
  sk_sp<SkTypeface> getTypeface(FontFamily family, FontWeight weight) const;

  /**
   * Apply optimal font rendering settings for Windows.
   */
  void configureFont(SkFont &font) const;

  // ========================================================================
  // STORAGE
  // ========================================================================

  // Typeface cache: [Family][Weight] -> SkTypeface
  // Family: 0=UI, 1=Mono, 2=Display (shares with UI)
  // Weight: 0=Regular, 1=Medium, 2=SemiBold, 3=Bold
  static constexpr int kNumFamilies = 3;
  static constexpr int kNumWeights = 4;

  std::array<std::array<sk_sp<SkTypeface>, kNumWeights>, kNumFamilies>
      typefaces_;

  // Font manager for loading
  sk_sp<SkFontMgr> fontMgr_;

  // Initialization state
  bool fontsLoaded_ = false;

  // Thread safety
  mutable std::mutex mutex_;

  // Resource directory cache
  juce::File fontDir_;
};

// ============================================================================
// CONVENIENCE FUNCTIONS (Global Access)
// ============================================================================
// Notes:
// Convenience functions are now located in ZenithDesignSystem.h under the
// zenith::design::typography namespace to avoid ambiguity.

} // namespace design
} // namespace zenith
