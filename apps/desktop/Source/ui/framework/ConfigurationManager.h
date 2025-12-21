/*
  ==============================================================================

    ConfigurationManager.h
    Created: 2025-12-07
    Author:  AI Assistant

    UI State Management and Configuration System

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "SkiaLayout.h"
#include "ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

// Forward declarations for UI components
namespace zenith {
namespace layout {
class SkiaListBox;
class SkiaVerticalLayout;
} // namespace layout
} // namespace zenith

namespace zenith {
namespace config {

// ============================================================================
// Configuration Keys
// ============================================================================

namespace keys {
// UI State
const juce::String WINDOW_X = "window.x";
const juce::String WINDOW_Y = "window.y";
const juce::String WINDOW_WIDTH = "window.width";
const juce::String WINDOW_HEIGHT = "window.height";
const juce::String WINDOW_MAXIMIZED = "window.maximized";

// Theme
const juce::String THEME_NAME = "theme.name";
const juce::String THEME_ACCENT_COLOR = "theme.accentColor";
const juce::String THEME_BACKGROUND_COLOR = "theme.backgroundColor";
const juce::String THEME_OLED_MODE = "theme.oledMode";
const juce::String THEME_GLOW_INTENSITY = "theme.glowIntensity";
const juce::String THEME_UI_SCALE = "theme.uiScale";

// Layout
const juce::String LAYOUT_BROWSER_VISIBLE = "layout.browser.visible";
const juce::String LAYOUT_BROWSER_WIDTH = "layout.browser.width";
const juce::String LAYOUT_RIGHT_PANEL_VISIBLE = "layout.rightPanel.visible";
const juce::String LAYOUT_RIGHT_PANEL_WIDTH = "layout.rightPanel.width";
const juce::String LAYOUT_BOTTOM_PANEL_VISIBLE = "layout.bottomPanel.visible";
const juce::String LAYOUT_BOTTOM_PANEL_HEIGHT = "layout.bottomPanel.height";
const juce::String LAYOUT_MIXER_VISIBLE = "layout.mixer.visible";
const juce::String LAYOUT_MIXER_HEIGHT = "layout.mixer.height";

// Audio
const juce::String AUDIO_DEVICE_TYPE = "audio.deviceType";
const juce::String AUDIO_OUTPUT_DEVICE = "audio.outputDevice";
const juce::String AUDIO_INPUT_DEVICE = "audio.inputDevice";
const juce::String AUDIO_SAMPLE_RATE = "audio.sampleRate";
const juce::String AUDIO_BUFFER_SIZE = "audio.bufferSize";

// MIDI
const juce::String MIDI_INPUT_DEVICE = "midi.inputDevice";
const juce::String MIDI_OUTPUT_DEVICE = "midi.outputDevice";

// User Preferences
const juce::String PREF_AUTO_SAVE_ENABLED = "preferences.autoSave.enabled";
const juce::String PREF_AUTO_SAVE_INTERVAL = "preferences.autoSave.interval";
const juce::String PREF_TOOLTIPS_ENABLED = "preferences.tooltips.enabled";
const juce::String PREF_TOOLTIPS_DELAY = "preferences.tooltips.delay";
const juce::String PREF_WELCOME_SCREEN = "preferences.welcomeScreen.enabled";
const juce::String PREF_RECENT_FILES_MAX = "preferences.recentFiles.maxCount";
const juce::String PREF_UNDO_LIMIT = "preferences.undo.limit";

// AI Integration
const juce::String AI_API_KEY = "ai.apiKey";
const juce::String AI_PROVIDER = "ai.provider";
const juce::String AI_MODEL = "ai.model";
const juce::String AI_AUTO_SUGGESTIONS = "ai.autoSuggestions.enabled";

// Performance
const juce::String PERF_FPS_LIMIT = "performance.fpsLimit";
const juce::String PERF_VSYNC = "performance.vsync.enabled";
const juce::String PERF_GPU_ACCELERATION =
    "performance.gpuAcceleration.enabled";
const juce::String PERF_METER_REFRESH_RATE = "performance.meterRefreshRate";

// Project Defaults
const juce::String PROJECT_DEFAULT_TEMPO = "project.defaultTempo";
const juce::String PROJECT_DEFAULT_TIMESIG_NUM = "project.defaultTimeSigNum";
const juce::String PROJECT_DEFAULT_TIMESIG_DEN = "project.defaultTimeSigDen";
const juce::String PROJECT_DEFAULT_SAMPLE_RATE = "project.defaultSampleRate";
const juce::String PROJECT_DEFAULT_BUFFER_SIZE = "project.defaultBufferSize";

// Collaboration
const juce::String COLLAB_SALT = "collab.salt";
} // namespace keys

// ============================================================================
// Configuration Value Wrapper
// ============================================================================

class ConfigValue {
public:
  enum class Type { Bool, Int, Float, String, Color, Array, Object };

  ConfigValue() = default;
  explicit ConfigValue(bool value);
  explicit ConfigValue(int value);
  explicit ConfigValue(float value);
  explicit ConfigValue(const juce::String &value);
  explicit ConfigValue(SkColor value);
  explicit ConfigValue(const juce::Array<ConfigValue> &array);
  explicit ConfigValue(const juce::DynamicObject::Ptr &object);

  // Type checking
  Type getType() const { return type_; }
  bool isBool() const { return type_ == Type::Bool; }
  bool isInt() const { return type_ == Type::Int; }
  bool isFloat() const { return type_ == Type::Float; }
  bool isString() const { return type_ == Type::String; }
  bool isColor() const { return type_ == Type::Color; }
  bool isArray() const { return type_ == Type::Array; }
  bool isObject() const { return type_ == Type::Object; }

  // Value access
  bool getBool() const;
  int getInt() const;
  float getFloat() const;
  juce::String getString() const;
  SkColor getColor() const;
  juce::Array<ConfigValue> getArray() const;
  juce::DynamicObject::Ptr getObject() const;

  // Conversion
  juce::var toVar() const;
  static ConfigValue fromVar(const juce::var &var);

private:
  Type type_ = Type::String;
  juce::var value_;
};

// ============================================================================
// Configuration Manager
// ============================================================================

class ConfigurationManager {
public:
  static ConfigurationManager &getInstance();

  // Initialization
  void initialize(const juce::File &configFile);
  void shutdown();
  bool isInitialized() const { return initialized_; }

  // Value access
  ConfigValue getValue(const juce::String &key) const;
  ConfigValue getValue(const juce::String &key,
                       const ConfigValue &defaultValue) const;

  void setValue(const juce::String &key, const ConfigValue &value);
  void setValue(const juce::String &key, bool value);
  void setValue(const juce::String &key, int value);
  void setValue(const juce::String &key, float value);
  void setValue(const juce::String &key, const juce::String &value);
  void setValue(const juce::String &key, SkColor value);

  // Type-specific convenience methods
  bool getBool(const juce::String &key) const;
  bool getBool(const juce::String &key, bool defaultValue) const;
  void setBool(const juce::String &key, bool value);

  int getInt(const juce::String &key) const;
  int getInt(const juce::String &key, int defaultValue) const;
  void setInt(const juce::String &key, int value);

  float getFloat(const juce::String &key) const;
  float getFloat(const juce::String &key, float defaultValue) const;
  void setFloat(const juce::String &key, float value);

  juce::String getString(const juce::String &key) const;
  juce::String getString(const juce::String &key,
                         const juce::String &defaultValue) const;
  void setString(const juce::String &key, const juce::String &value);

  SkColor getColor(const juce::String &key) const;
  SkColor getColor(const juce::String &key, SkColor defaultValue) const;
  void setColor(const juce::String &key, SkColor value);

  // Configuration groups
  juce::DynamicObject::Ptr getGroup(const juce::String &groupKey) const;
  void setGroup(const juce::String &groupKey,
                const juce::DynamicObject::Ptr &group);

  // Persistence
  bool loadConfiguration();
  bool saveConfiguration();
  bool saveConfigurationAs(const juce::File &newFile);

  // Configuration file management
  juce::File getConfigurationFile() const { return configFile_; }
  void setConfigurationFile(const juce::File &file);
  bool exportConfiguration(const juce::File &destination);
  bool importConfiguration(const juce::File &source);

  // Defaults
  void setDefaultValue(const juce::String &key, const ConfigValue &value);
  void setDefaultValues(const juce::DynamicObject::Ptr &defaults);
  void resetToDefaults();
  void resetKeyToDefault(const juce::String &key);

  // Change notifications
  using ConfigChangeCallback =
      std::function<void(const juce::String &key, const ConfigValue &value)>;
  void addChangeListener(ConfigChangeCallback callback);
  void removeChangeListener(ConfigChangeCallback callback);
  void notifyChangeListeners(const juce::String &key, const ConfigValue &value);

  // Validation
  bool hasKey(const juce::String &key) const;
  juce::StringArray getAllKeys() const;
  juce::StringArray getKeysWithPrefix(const juce::String &prefix) const;

  // Type validation
  bool isValidKeyType(const juce::String &key,
                      ConfigValue::Type expectedType) const;

  // Backup and restore
  bool backupConfiguration();
  bool restoreFromBackup();
  juce::File getBackupFile() const;

  // Statistics
  int getConfigurationCount() const;
  juce::String getConfigurationStatistics() const;

  // Migration
  void migrateFromVersion(const juce::String &version);
  juce::String getConfigurationVersion() const;
  void setConfigurationVersion(const juce::String &version);

private:
  ConfigurationManager() = default;
  ~ConfigurationManager() = default;

  ConfigurationManager(const ConfigurationManager &) = delete;
  ConfigurationManager &operator=(const ConfigurationManager &) = delete;

  // Internal methods
  void loadDefaults();
  juce::var getNestedValue(const juce::String &key) const;
  juce::var getNestedValueFromObject(const juce::String &key,
                                     const juce::DynamicObject::Ptr &object) const;
  void setNestedValue(const juce::String &key, const juce::var &value);
  void setNestedValueInObject(const juce::String &key, const juce::var &value,
                              const juce::DynamicObject::Ptr &object);
  juce::StringArray splitKey(const juce::String &key) const;
  juce::DynamicObject::Ptr ensureNestedObject(const juce::String &key);

  // Data members
  bool initialized_ = false;
  juce::File configFile_;
  juce::DynamicObject::Ptr configData_;
  juce::DynamicObject::Ptr defaultData_;
  juce::Array<ConfigChangeCallback> changeListeners_;
  juce::CriticalSection lock_;

  juce::File backupFile_;
  static constexpr int MAX_BACKUP_COUNT = 5;
};

// ============================================================================
// Theme Configuration
// ============================================================================

class ThemeConfiguration {
public:
  static ThemeConfiguration &getInstance();

  // Theme management
  void setTheme(const juce::String &themeName);
  juce::String getCurrentTheme() const;

  juce::StringArray getAvailableThemes() const;
  void registerTheme(const juce::String &name,
                     const juce::DynamicObject::Ptr &themeData);

  // Color management
  void setAccentColor(SkColor color);
  SkColor getAccentColor() const;

  void setBackgroundColor(SkColor color);
  SkColor getBackgroundColor() const;

  // OLED mode
  void setOledMode(bool enabled);
  bool isOledMode() const;

  // Glow effects
  void setGlowIntensity(float intensity);
  float getGlowIntensity() const;

  // UI scaling
  void setUiScale(float scale);
  float getUiScale() const;

  // Apply theme to design system
  void applyCurrentTheme();

private:
  ThemeConfiguration() = default;
  ~ThemeConfiguration() = default;

  juce::String currentTheme_ = "NeonNoir";
  juce::HashMap<juce::String, juce::DynamicObject::Ptr> themes_;
  juce::CriticalSection lock_;
};

// ============================================================================
// Layout Configuration
// ============================================================================

class LayoutConfiguration {
public:
  static LayoutConfiguration &getInstance();

  // Panel visibility
  void setBrowserVisible(bool visible);
  bool isBrowserVisible() const;

  void setRightPanelVisible(bool visible);
  bool isRightPanelVisible() const;

  void setBottomPanelVisible(bool visible);
  bool isBottomPanelVisible() const;

  void setMixerVisible(bool visible);
  bool isMixerVisible() const;

  // Panel sizes
  void setBrowserWidth(int width);
  int getBrowserWidth() const;

  void setRightPanelWidth(int width);
  int getRightPanelWidth() const;

  void setBottomPanelHeight(int height);
  int getBottomPanelHeight() const;

  void setMixerHeight(int height);
  int getMixerHeight() const;

  // Layout presets
  juce::StringArray getLayoutPresets() const;
  void saveLayoutPreset(const juce::String &name);
  void loadLayoutPreset(const juce::String &name);
  void deleteLayoutPreset(const juce::String &name);

  // Window state
  void setWindowBounds(const juce::Rectangle<int> &bounds);
  juce::Rectangle<int> getWindowBounds() const;

  void setWindowMaximized(bool maximized);
  bool isWindowMaximized() const;

private:
  LayoutConfiguration() = default;
  ~LayoutConfiguration() = default;

  juce::CriticalSection lock_;
};

// ============================================================================
// User Preferences
// ============================================================================

class UserPreferences {
public:
  static UserPreferences &getInstance();

  // Auto-save
  void setAutoSaveEnabled(bool enabled);
  bool isAutoSaveEnabled() const;

  void setAutoSaveInterval(int minutes);
  int getAutoSaveInterval() const;

  // Tooltips
  void setTooltipsEnabled(bool enabled);
  bool isTooltipsEnabled() const;

  void setTooltipDelay(int milliseconds);
  int getTooltipDelay() const;

  // Welcome screen
  void setWelcomeScreenEnabled(bool enabled);
  bool isWelcomeScreenEnabled() const;

  // Recent files
  void setRecentFilesMaxCount(int count);
  int getRecentFilesMaxCount() const;

  juce::StringArray getRecentFiles() const;
  void addRecentFile(const juce::File &file);
  void clearRecentFiles();

  // Undo/Redo
  void setUndoLimit(int limit);
  int getUndoLimit() const;

  // Keyboard shortcuts
  juce::DynamicObject::Ptr getKeyboardShortcuts() const;
  void setKeyboardShortcuts(const juce::DynamicObject::Ptr &shortcuts);

private:
  UserPreferences() = default;
  ~UserPreferences() = default;

  juce::CriticalSection lock_;
};

// ============================================================================
// Settings UI Component
// ============================================================================

class SettingsPanel : public SkiaComponent {
public:
  SettingsPanel();
  ~SettingsPanel();

  // Category management
  void addCategory(const juce::String &name, const juce::String &icon);
  void setCurrentCategory(const juce::String &name);
  juce::String getCurrentCategory() const;

  // Settings management
  void addSetting(const juce::String &category, const juce::String &key,
                  const juce::String &label, const ConfigValue &defaultValue);

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

private:
  struct Category {
    juce::String name;
    juce::String icon;
    juce::Array<juce::String> settings;
  };

  struct Setting {
    juce::String key;
    juce::String label;
    ConfigValue defaultValue;
    std::unique_ptr<SkiaComponent> editor;
  };

  juce::HashMap<juce::String, Category> categories_;
  juce::HashMap<juce::String, Setting> settings_;
  juce::String currentCategory_;

  // UI Components
  std::unique_ptr<layout::SkiaListBox> categoryList_;
  std::unique_ptr<layout::SkiaVerticalLayout> settingsLayout_;

  void createUI();
  void refreshSettingsView();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};

} // namespace config
} // namespace zenith