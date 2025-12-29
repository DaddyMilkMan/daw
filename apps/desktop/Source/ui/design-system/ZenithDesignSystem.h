/*
  ==============================================================================

    ZenithDesignSystem.h
    Created: 2025-11-30
    Authors: Leo "Lil Bit" Rossi & Yuki Tanaka
    Refactored: 2025-12-27 (Global State Cleanup)

    The complete Neon Noir design system for Zenith DAW.
    "If it doesn't glow, it doesn't go!" - Leo
    "Every pixel has a purpose." - Yuki

  ==============================================================================
*/

#pragma once
#include "FontManager.h"
#include <core/SkBlurTypes.h>
#include <include/core/SkColor.h>
#include <include/core/SkFont.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <map>
#include <functional>

namespace zenith {
namespace design {

// ============================================================================
// PALETTE DEFINITION
// ============================================================================

struct ZenithPalette {
    // Brand Colors
    SkColor cyan = 0xFF00F0FF;        // Electric Blue (Primary Brand)
    SkColor magenta = 0xFFFF00D4;     // Hot Pink (Secondary Brand)
    SkColor violet = 0xFF7000FF;      // Deep Violet
    SkColor orange = 0xFFFF8800;      // Orange
    SkColor neonGreen = 0xFF00FF9D;   // Spring Green
    SkColor neonPink = 0xFFFF1493;    // Deep Pink
    SkColor neonRed = 0xFFFF073A;     // Neon Red
    SkColor neonYellow = 0xFFFFFF00;  // Yellow
    SkColor neonPurple = 0xFFAA00FF;  // Purple
    SkColor cyanDark = 0xFF008888;    // Dark Cyan

    // Functional Palette
    SkColor blue = 0xFF3B82F6;        // Standard Blue (Info/Action)
    SkColor green = 0xFF10B981;       // Success/Safe
    SkColor yellow = 0xFFF59E0B;      // Warning/Caution
    SkColor red = 0xFFEF4444;         // Error/Danger
    SkColor pink = 0xFFEC4899;        // Automation
    SkColor amber = 0xFFFFAB00;       // Warm Warning

    // Background Layers
    SkColor bg00 = 0xFF0D0D11;       // Obsidian (Deepest/App Background)
    SkColor bg01 = 0xFF1C1C24;       // Charcoal (Canvas/Main)
    SkColor bg02 = 0xFF24242C;       // Panels
    SkColor bg03 = 0xFF2E2E36;       // Elevated Surfaces
    SkColor bg04 = 0xFF383840;       // Highest Elevation

    // Text Hierarchy
    SkColor textPrimary = 0xFFF2F2F7;
    SkColor textSecondary = 0xFFA1A1AA;
    SkColor textTertiary = 0xFF71717A;
    SkColor textDisabled = 0xFF52525B;
    SkColor textInverse = 0xFF1C1C24;

    // Borders & Dividers
    SkColor borderSubtle = 0x0FFFFFFF;
    SkColor borderDefault = 0x1FFFFFFF;
    SkColor borderStrong = 0x33FFFFFF;
    SkColor borderFocus = 0xFF00F0FF;
    SkColor borderGreeting = 0x1AFFFFFF;

    // Semantic Map
    SkColor accentPrimary = 0xFF00F0FF;
    SkColor accentSecondary = 0xFFFF00D4;
    SkColor surfaceBase = 0xFF121212;
    SkColor surfaceElevated = 0xFF1A1A1A;
    SkColor success = 0xFF10B981;
    SkColor danger = 0xFFEF4444;
    SkColor warning = 0xFFFFAB00;
    SkColor info = 0xFF3B82F6;

    // Glass
    SkColor glassHighlight = 0x1AFFFFFF;
    SkColor glassShadow = 0x66000000;
    SkColor glassHover = 0x0DFFFFFF;
    SkColor glass10 = 0x1AFFFFFF;

    // Audio Viz
    SkColor waveformAudio = 0xFF3B82F6;
    SkColor waveformMidi = 0xFFFF00D4;
    SkColor automation = 0xFFEC4899;
    SkColor playhead = 0xFFFF8800;
};

// ============================================================================
// THEME MANAGER
// ============================================================================

enum class ThemePreset {
  Dark,   // Neon Noir
  Darker, // OLED Black
  Light   // Light mode
};

class ThemeListener {
public:
  virtual ~ThemeListener() = default;
  virtual void themeChanged(ThemePreset newTheme) = 0;
};

// ============================================================================
// GLOBAL SETTINGS
// ============================================================================

struct Settings {
  float glowIntensity = 1.0f;
  float uiScale = 1.0f;
  enum class Theme { NeonNoir, OLEDBlack, Classic };
  Theme currentTheme = Theme::NeonNoir;
  enum class BlurQuality { Off, Low, Medium, High };
  BlurQuality blurQuality = BlurQuality::Medium;
};

class ThemeManager : public juce::ChangeBroadcaster {
public:
  static ThemeManager &getInstance() {
    static ThemeManager instance;
    return instance;
  }

  // Basic theme data for serialization
  struct Theme {
    juce::String name;
    std::map<juce::String, uint32_t> colors;
  };

  void setActiveTheme(ThemePreset preset);
  ThemePreset getActiveTheme() const { return activePreset_; }

  void addListener(ThemeListener *listener);
  void removeListener(ThemeListener *listener);

  void saveTheme(const juce::String &name);
  void loadTheme(const juce::String &name);
  void deleteTheme(const juce::String &name);

  juce::StringArray getAvailableThemes() const;

  void applyTheme(const Theme &theme);
  
  void resetToDefault();

  const ZenithPalette &getPalette() const { return currentPalette_; }
  
  // Settings Management
  const struct Settings& getSettings() const { return settings_; }
  
  void updateSettings(const std::function<void(struct Settings&)>& mutator) {
      mutator(settings_);
      sendChangeMessage(); // Broadcast settings change
  }

private:
  ThemeManager();
  
  Settings settings_;
  
  // No copy/move
  ThemeManager(const ThemeManager&) = delete;
  ThemeManager& operator=(const ThemeManager&) = delete;

  void applyDarkTheme();
  void applyDarkerTheme();
  void applyLightTheme();
  void notifyListeners();

  ThemePreset activePreset_ = ThemePreset::Dark;
  ZenithPalette currentPalette_;
  std::vector<ThemeListener *> listeners_;

  juce::File getThemeDir() const {
    auto dir =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW/Themes");
    if (!dir.exists())
      dir.createDirectory();
    return dir;
  }
};

// ============================================================================
// COLOR PALETTE (Immutable References to ThemeManager)
// ============================================================================

namespace colors {
    // Helper to access the singleton's palette
    inline const ZenithPalette& p() { return ThemeManager::getInstance().getPalette(); }

    // Brand Colors
    inline const SkColor& CYAN = p().cyan;
    inline const SkColor& MAGENTA = p().magenta;
    inline const SkColor& VIOLET = p().violet;
    inline const SkColor& ORANGE = p().orange;
    inline const SkColor& NEON_GREEN = p().neonGreen;
    inline const SkColor& NEON_PINK = p().neonPink;
    inline const SkColor& NEON_RED = p().neonRed;
    inline const SkColor& NEON_YELLOW = p().neonYellow;
    inline const SkColor& NEON_PURPLE = p().neonPurple;
    inline const SkColor& CYAN_DARK = p().cyanDark;

    // Functional Palette
    inline const SkColor& BLUE = p().blue;
    inline const SkColor& GREEN = p().green;
    inline const SkColor& YELLOW = p().yellow;
    inline const SkColor& RED = p().red;
    inline const SkColor& PINK = p().pink;
    inline const SkColor& AMBER = p().amber;

    // Background Layers
    inline const SkColor& BG_00 = p().bg00;
    inline const SkColor& BG_01 = p().bg01;
    inline const SkColor& BG_02 = p().bg02;
    inline const SkColor& BG_03 = p().bg03;
    inline const SkColor& BG_04 = p().bg04;

    // Legacy Aliases
    inline const SkColor& BG_DARKEST = p().bg00;
    inline const SkColor& BG_DARKER = p().bg01;
    inline const SkColor& BG_DARK = p().bg02;
    inline const SkColor& BG_MEDIUM = p().bg03;
    inline const SkColor& BG_LIGHT = p().bg04;

    // Semantic Aliases
    inline const SkColor& ACCENT_PRIMARY = p().accentPrimary;
    inline const SkColor& ACCENT_SECONDARY = p().accentSecondary;
    inline const SkColor& SURFACE_BASE = p().surfaceBase;
    inline const SkColor& SURFACE_ELEVATED = p().surfaceElevated;
    inline const SkColor& SUCCESS = p().success;
    inline const SkColor& DANGER = p().danger;
    inline const SkColor& WARNING = p().warning;
    inline const SkColor& INFO = p().info;

    // Text Hierarchy
    inline const SkColor& TEXT_PRIMARY = p().textPrimary;
    inline const SkColor& TEXT_SECONDARY = p().textSecondary;
    inline const SkColor& TEXT_TERTIARY = p().textTertiary;
    inline const SkColor& TEXT_DISABLED = p().textDisabled;
    inline const SkColor& TEXT_INVERSE = p().textInverse;

    // Borders & Dividers
    inline const SkColor& BORDER_SUBTLE = p().borderSubtle;
    inline const SkColor& BORDER_DEFAULT = p().borderDefault;
    inline const SkColor& BORDER_STRONG = p().borderStrong;
    inline const SkColor& BORDER_FOCUS = p().borderFocus;
    inline const SkColor& BORDER_GREETING = p().borderGreeting;

    // Glassmorphism System
    inline const SkColor& GLASS_HIGHLIGHT = p().glassHighlight;
    inline const SkColor& GLASS_SHADOW = p().glassShadow;
    inline const SkColor& GLASS_HOVER = p().glassHover;
    inline const SkColor& GLASS_10 = p().glass10;

    // Audio Visualization
    inline const SkColor& WAVEFORM_AUDIO = p().waveformAudio;
    inline const SkColor& WAVEFORM_MIDI = p().waveformMidi;
    inline const SkColor& AUTOMATION = p().automation;
    inline const SkColor& PLAYHEAD = p().playhead;

    // Forward reset to manager
    inline void resetToDefault() {
        ThemeManager::getInstance().resetToDefault();
    }
} // namespace colors

// ============================================================================
// LAYOUT MANAGER
// ============================================================================

class LayoutManager {
public:
  static LayoutManager &getInstance() {
    static LayoutManager instance;
    return instance;
  }

  struct PanelState {
    juce::String id;
    juce::Rectangle<float> relativeBounds; // 0.0-1.0 relative to window
    bool isVisible = true;
    int zOrder = 0;
  };

  void setPanelState(const juce::String &panelId, const PanelState &state);
  PanelState getPanelState(const juce::String &panelId) const;

  void saveLayout(const juce::String &name);
  void loadLayout(const juce::String &name);

  bool isEditModeEnabled() const { return editMode_; }
  void setEditModeEnabled(bool enabled) { editMode_ = enabled; }

private:
  LayoutManager() = default;
  std::map<juce::String, PanelState> panels_;
  bool editMode_ = false;

  juce::File getLayoutDir() const {
    auto dir =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW/Layouts");
    if (!dir.exists())
      dir.createDirectory();
    return dir;
  }
};

// ============================================================================
// SPACING SYSTEM - "The Grid"
// ============================================================================

namespace spacing {
constexpr float XS = 4.0f;   // Tiny gaps
constexpr float SM = 8.0f;   // Small gaps
constexpr float MD = 16.0f;  // Standard gaps
constexpr float LG = 24.0f;  // Large gaps
constexpr float XL = 32.0f;  // Extra large gaps
constexpr float XXL = 48.0f; // Huge gaps

// Component-specific
constexpr float PANEL_PADDING = MD;
constexpr float COMPONENT_GAP = SM;
constexpr float SECTION_GAP = LG;
} // namespace spacing

// ============================================================================
// TYPOGRAPHY - "Readable & Beautiful"
// ============================================================================

namespace typography {

using FontWeight = zenith::design::FontWeight;

// ============================================================================
// FONT ACCESS FUNCTIONS
// ============================================================================

inline SkFont getSkFont(float size, FontWeight weight = FontWeight::Regular) {
  return FontManager::getInstance().getUIFont(size, weight);
}

inline SkFont getMonoFont(float size, FontWeight weight = FontWeight::Regular) {
  return FontManager::getInstance().getMonoFont(size, weight);
}

inline SkFont getDisplayFont(float size, FontWeight weight = FontWeight::Bold) {
  return FontManager::getInstance().getDisplayFont(size, weight);
}

// ============================================================================
// JUCE FONT COMPATIBILITY
// ============================================================================

inline juce::Font getJuceFont(float size, FontWeight weight = FontWeight::Regular) {
    juce::FontOptions options;
    options = options.withHeight(size);
    options = options.withName("Inter");
    
    if (weight == FontWeight::Bold) options = options.withStyle("Bold");
    else if (weight == FontWeight::SemiBold) options = options.withStyle("SemiBold");
    else if (weight == FontWeight::Medium) options = options.withStyle("Medium");
    
    return juce::Font(options);
}

inline juce::Font getJuceMonoFont(float size) {
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
}

// ============================================================================
// LEGACY COMPATIBILITY
// ============================================================================

[[deprecated("Use getSkFont(size, weight) instead.")]]
inline SkFont getSkFontWithSize(float size) {
  return zenith::design::typography::getSkFont(size, FontWeight::Regular);
}
// FONT SIZE CONSTANTS
// ============================================================================

constexpr float FONT_XS = 10.0f;  // Labels, hints, small captions
constexpr float FONT_SM = 12.0f;  // Secondary text, metadata
constexpr float FONT_MD = 14.0f;  // Body text, default UI text
constexpr float FONT_LG = 16.0f;  // Headings, emphasized text
constexpr float FONT_XL = 20.0f;  // Large headings, section titles
constexpr float FONT_XXL = 24.0f; // Display text, major titles

constexpr float LINE_HEIGHT_TIGHT = 1.2f;
constexpr float LINE_HEIGHT_NORMAL = 1.5f;
constexpr float LINE_HEIGHT_RELAXED = 1.8f;

} // namespace typography

using typography::getDisplayFont;
using typography::getMonoFont;
using typography::getSkFont;

// ============================================================================
// DIMENSIONS
// ============================================================================

namespace dimensions {
constexpr float BUTTON_HEIGHT = 32.0f;
constexpr float BUTTON_HEIGHT_SM = 24.0f;
constexpr float BUTTON_HEIGHT_LG = 40.0f;

constexpr float INPUT_HEIGHT = 32.0f;
constexpr float KNOB_SIZE = 64.0f;
constexpr float KNOB_SIZE_SM = 48.0f;
constexpr float KNOB_SIZE_LG = 80.0f;

constexpr float TRANSPORT_BAR_HEIGHT = 60.0f;
constexpr float LEFT_SIDEBAR_WIDTH = 280.0f;
constexpr float RIGHT_SIDEBAR_WIDTH = 320.0f;
constexpr float BOTTOM_PANEL_HEIGHT = 200.0f;

constexpr float MIN_PANEL_WIDTH = 200.0f;
constexpr float MIN_PANEL_HEIGHT = 100.0f;

constexpr float RADIUS_NONE = 0.0f;
constexpr float RADIUS_SM = 8.0f;
constexpr float RADIUS_LG = 16.0f;
constexpr float RADIUS_FULL = 9999.0f;

// ============================================================================
// ARRANGER VIEW LAYOUT (Single Source of Truth!)
// ============================================================================
// CRITICAL: All arranger files MUST use these constants to avoid coordinate chaos
constexpr float ARRANGER_HEADER_WIDTH = 200.0f;
constexpr float ARRANGER_SECTION_HEIGHT = 24.0f;
constexpr float ARRANGER_RULER_HEIGHT = 32.0f;
constexpr float ARRANGER_TRACK_HEIGHT = 80.0f;
constexpr float ARRANGER_TOP_MARGIN = ARRANGER_SECTION_HEIGHT + ARRANGER_RULER_HEIGHT;

// Grid visibility constants
constexpr float ARRANGER_MIN_GRID_SPACING = 20.0f;
constexpr float ARRANGER_BAR_LINE_WIDTH = 1.0f;
constexpr float ARRANGER_BEAT_LINE_WIDTH = 1.0f;

// ============================================================================
// SESSION VIEW LAYOUT (Vertical tracks - Ableton-style)
// ============================================================================
constexpr float SESSION_CELL_WIDTH = 100.0f;
constexpr float SESSION_CELL_HEIGHT = 80.0f;
constexpr float SESSION_SCENE_HEADER_WIDTH = 80.0f;
constexpr float SESSION_TRACK_HEADER_HEIGHT = 48.0f;
constexpr float SESSION_CELL_CORNER_RADIUS = 6.0f;
constexpr float SESSION_CELL_GAP = 2.0f;

} // namespace dimensions

// ============================================================================
// TRACK COLORS PALETTE
// ============================================================================
namespace colors {

// Neon-inspired track color palette
static const std::vector<juce::Colour> TRACK_COLORS = {
    juce::Colour(0xFF00F0FF),  // Cyan
    juce::Colour(0xFFFF00D4),  // Magenta
    juce::Colour(0xFF00FF9D),  // Neon Green
    juce::Colour(0xFFFF8800),  // Orange
    juce::Colour(0xFF7000FF),  // Violet
    juce::Colour(0xFFFF1493),  // Deep Pink
    juce::Colour(0xFF3B82F6),  // Blue
    juce::Colour(0xFF10B981),  // Teal
    juce::Colour(0xFFF59E0B),  // Amber
    juce::Colour(0xFFEC4899),  // Pink
    juce::Colour(0xFFAA00FF),  // Purple
    juce::Colour(0xFF00FF00),  // Lime
};

// Note: BG_00 through BG_04 already defined above as SkColor references
// Use design::toJuceColour() to convert if JUCE Colour is needed

} // namespace colors

// ============================================================================
// EFFECTS
// ============================================================================

namespace effects {
constexpr float GLOW_SUBTLE = 2.0f;
constexpr float GLOW_MEDIUM = 4.0f;
constexpr float GLOW_STRONG = 6.0f;
constexpr float GLOW_INTENSE = 8.0f;
constexpr float BLUR_GLASS = 20.0f;

constexpr float OPACITY_SUBTLE = 0.2f;
constexpr float OPACITY_MEDIUM = 0.4f;
constexpr float OPACITY_STRONG = 0.6f;
constexpr float OPACITY_INTENSE = 0.8f;

constexpr float SHADOW_OFFSET_SM = 2.0f;
constexpr float SHADOW_OFFSET_MD = 4.0f;
constexpr float SHADOW_OFFSET_LG = 8.0f;
} // namespace effects

// ============================================================================
// ANIMATION
// ============================================================================

namespace animation {
constexpr int DURATION_INSTANT = 0;
constexpr int DURATION_FAST = 100;
constexpr int DURATION_NORMAL = 200;
constexpr int DURATION_SLOW = 300;
constexpr int DURATION_SLOWER = 500;

// Easing curves (for reference, actual implementation in animation system)
// - ease-in: slow start, fast end
// - ease-out: fast start, slow end
// - ease-in-out: slow start and end
// - spring: physics-based bounce

// Frame Rate Targets
constexpr int FPS_TARGET = 60;
constexpr int FPS_HIGH = 120;
constexpr float FRAME_TIME_60FPS = 16.67f; // milliseconds
constexpr float FRAME_TIME_120FPS = 8.33f; // milliseconds

enum class Curve {
  Linear,
  EaseInQuad,
  EaseOutQuad,
  EaseInOutQuad,
  EaseInCubic,
  EaseOutCubic,
  EaseInOutCubic,
  Spring
};

class Animator : private juce::Timer {
public:
  static Animator& getInstance() {
      static Animator instance;
      return instance;
  }

  using UpdateCallback = std::function<void(float)>;
  using CompleteCallback = std::function<void()>;

  struct Animation {
      juce::Identifier id;
      float startValue;
      float endValue;
      float durationMs;
      float startTimeMs;
      Curve curve;
      UpdateCallback onUpdate;
      CompleteCallback onComplete;
      bool isRunning = true;
  };

  void animate(const juce::String& idStr, float start, float end, float durationMs, Curve curve, UpdateCallback update, CompleteCallback complete = nullptr) {
      juce::Identifier id(idStr);
      juce::ScopedLock lock(mutex_);
      
      // Remove existing animation with same ID
      activeAnimations_.erase(std::remove_if(activeAnimations_.begin(), activeAnimations_.end(),
          [&](const Animation& a) { return a.id == id; }), activeAnimations_.end());

      Animation anim;
      anim.id = id;
      anim.startValue = start;
      anim.endValue = end;
      anim.durationMs = durationMs;
      anim.startTimeMs = (float)juce::Time::getMillisecondCounterHiRes();
      anim.curve = curve;
      anim.onUpdate = update;
      anim.onComplete = complete;

      activeAnimations_.push_back(std::move(anim));
      
      if (update) update(start);

      if (!isTimerRunning()) startTimerHz(60); // Target 60 FPS
  }

  void cancel(const juce::String& idStr) {
      juce::Identifier id(idStr);
      juce::ScopedLock lock(mutex_);
      activeAnimations_.erase(std::remove_if(activeAnimations_.begin(), activeAnimations_.end(),
          [&](const Animation& a) { return a.id == id; }), activeAnimations_.end());
      
      if (activeAnimations_.empty()) stopTimer();
  }

  // Easing Functions
  static float applyCurve(float t, Curve curve) {
      t = juce::jlimit(0.0f, 1.0f, t);
      switch (curve) {
          case Curve::Linear: return t;
          case Curve::EaseInQuad: return t * t;
          case Curve::EaseOutQuad: return t * (2.0f - t);
          case Curve::EaseInOutQuad: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
          case Curve::EaseInCubic: return t * t * t;
          case Curve::EaseOutCubic: return (--t) * t * t + 1.0f;
          case Curve::EaseInOutCubic: return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
          case Curve::Spring: {
              const float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;
              return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
          }
          default: return t;
      }
  }

private:
  Animator() {}
  
  void timerCallback() override {
      std::vector<Animation> processingList;
      {
          juce::ScopedLock lock(mutex_);
          if (activeAnimations_.empty()) {
              stopTimer();
              return;
          }
          processingList = activeAnimations_;
      }

      float currentTime = (float)juce::Time::getMillisecondCounterHiRes();
      std::vector<juce::Identifier> finishedIds;

      for (auto& anim : processingList) {
          float elapsed = currentTime - anim.startTimeMs;
          float t = elapsed / anim.durationMs;
          bool finished = t >= 1.0f;

          if (finished) t = 1.0f;

          float curvedT = applyCurve(t, anim.curve);
          float currentValue = anim.startValue + (anim.endValue - anim.startValue) * curvedT;

          if (anim.onUpdate) anim.onUpdate(currentValue);

          if (finished) {
              if (anim.onComplete) anim.onComplete();
              finishedIds.push_back(anim.id);
          }
      }

      if (!finishedIds.empty()) {
          juce::ScopedLock lock(mutex_);
          activeAnimations_.erase(std::remove_if(activeAnimations_.begin(), activeAnimations_.end(),
              [&](const Animation& a) {
                  for (const auto& id : finishedIds) {
                      if (a.id == id) return a.startTimeMs < currentTime; 
                  }
                  return false;
              }), activeAnimations_.end());
          
          if (activeAnimations_.empty()) stopTimer();
      }
  }

  std::vector<Animation> activeAnimations_;
  juce::CriticalSection mutex_;
};
} // namespace animation


// ============================================================================
// Z-INDEX
// ============================================================================

namespace zindex {
constexpr int BACKGROUND = 0;
constexpr int PANEL = 10;
constexpr int COMPONENT = 20;
constexpr int CONTROL = 30;
constexpr int OVERLAY = 40;
constexpr int MODAL = 50;
constexpr int TOOLTIP = 60;
constexpr int NOTIFICATION = 70;
} // namespace zindex

// ============================================================================
// HELPERS
// ============================================================================

inline SkColor withAlpha(SkColor color, float alpha) {
  return SkColorSetARGB(static_cast<U8CPU>(alpha * 255.0f), SkColorGetR(color),
                        SkColorGetG(color), SkColorGetB(color));
}

inline SkColor lighten(SkColor color, float amount) {
  return SkColorSetARGB(
      SkColorGetA(color),
      std::min(255, static_cast<int>(SkColorGetR(color) * (1.0f + amount))),
      std::min(255, static_cast<int>(SkColorGetG(color) * (1.0f + amount))),
      std::min(255, static_cast<int>(SkColorGetB(color) * (1.0f + amount))));
}

inline SkColor darken(SkColor color, float amount) {
  return SkColorSetARGB(SkColorGetA(color),
                        static_cast<int>(SkColorGetR(color) * (1.0f - amount)),
                        static_cast<int>(SkColorGetG(color) * (1.0f - amount)),
                        static_cast<int>(SkColorGetB(color) * (1.0f - amount)));
}

inline float interpolate(float a, float b, float t) { return a + (b - a) * t; }

inline SkColor interpolateColor(SkColor c1, SkColor c2, float t) {
  float invT = 1.0f - t;
  int a = (int)(SkColorGetA(c1) * invT + SkColorGetA(c2) * t);
  int r = (int)(SkColorGetR(c1) * invT + SkColorGetR(c2) * t);
  int g = (int)(SkColorGetG(c1) * invT + SkColorGetG(c2) * t);
  int b = (int)(SkColorGetB(c1) * invT + SkColorGetB(c2) * t);
  return SkColorSetARGB(a, r, g, b);
}

// ============================================================================
// GLOBAL SETTINGS
// ============================================================================

// ============================================================================
// GLOBAL SETTINGS
// ============================================================================

// Settings moved to top

// Accessors delegated to ThemeManager (Singleton)
inline const Settings& getSettings() {
    return ThemeManager::getInstance().getSettings();
}

inline void updateSettings(const std::function<void(Settings&)>& mutator) {
    ThemeManager::getInstance().updateSettings(mutator);
}

// Global properties helpers (Backward Compatibility / Ease of Use)
inline float getGlowIntensity() { return getSettings().glowIntensity; }

} // namespace design
} // namespace zenith


