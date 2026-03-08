/*
  ==============================================================================

    ConfigurationManager.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    UI State Management and Configuration System Implementation

  ==============================================================================
*/

#include "ConfigurationManager.h"
#include "PlatformPathUtils.h"
#include "ZenithDesignSystem.h"

namespace zenith {
namespace config {

// ============================================================================
// ConfigValue Implementation
// ============================================================================

ConfigValue::ConfigValue(bool value) : type_(Type::Bool), value_(value) {}
ConfigValue::ConfigValue(int value) : type_(Type::Int), value_(value) {}
ConfigValue::ConfigValue(float value) : type_(Type::Float), value_(value) {}
ConfigValue::ConfigValue(const juce::String &value)
    : type_(Type::String), value_(value) {}
ConfigValue::ConfigValue(SkColor value)
    : type_(Type::Color), value_((int)value) {}
ConfigValue::ConfigValue(const juce::Array<ConfigValue> &array)
    : type_(Type::Array) {
  juce::Array<juce::var> varArray;
  for (const auto &configValue : array) {
    varArray.add(configValue.toVar());
  }
  value_ = varArray;
}
ConfigValue::ConfigValue(const juce::DynamicObject::Ptr &object)
    : type_(Type::Object), value_(object) {}

bool ConfigValue::getBool() const { return value_; }

int ConfigValue::getInt() const { return value_; }

float ConfigValue::getFloat() const { return value_; }

juce::String ConfigValue::getString() const { return value_.toString(); }

SkColor ConfigValue::getColor() const {
  return static_cast<SkColor>((int)value_);
}

juce::Array<ConfigValue> ConfigValue::getArray() const {
  juce::Array<ConfigValue> result;
  if (value_.isArray()) {
    auto *array = value_.getArray();
    for (const auto &var : *array) {
      result.add(ConfigValue::fromVar(var));
    }
  }
  return result;
}

juce::DynamicObject::Ptr ConfigValue::getObject() const {
  if (value_.isObject()) {
    return value_.getDynamicObject();
  }
  return nullptr;
}

juce::var ConfigValue::toVar() const { return value_; }

ConfigValue ConfigValue::fromVar(const juce::var &var) {
  if (var.isBool()) {
    return ConfigValue((bool)var);
  } else if (var.isInt()) {
    return ConfigValue((int)var);
  } else if (var.isDouble()) {
    return ConfigValue((float)var);
  } else if (var.isString()) {
    return ConfigValue(var.toString());
  } else if (var.isObject()) {
    return ConfigValue(var.getDynamicObject());
  } else if (var.isArray()) {
    // Handle array conversion
    juce::Array<ConfigValue> array;
    auto *varArray = var.getArray();
    for (const auto &item : *varArray) {
      array.add(ConfigValue::fromVar(item));
    }
    return ConfigValue(array);
  }
  return ConfigValue(var.toString());
}

// ============================================================================
// ConfigurationManager Implementation
// ============================================================================

ConfigurationManager &ConfigurationManager::getInstance() {
  static ConfigurationManager instance;
  return instance;
}

juce::File ConfigurationManager::getDefaultConfigurationFile() {
  return PlatformPathUtils::getDefaultConfigurationFile();
}

void ConfigurationManager::initialize(const juce::File &configFile) {
  juce::ScopedLock lock(lock_);

  if (initialized_) {
    return;
  }

  configFile_ = configFile;

  // Use default if file is invalid/empty
  if (configFile_ == juce::File()) {
    configFile_ = getDefaultConfigurationFile();
  }

  // Create default configuration
  configData_ = new juce::DynamicObject();
  defaultData_ = new juce::DynamicObject();

  loadDefaults();

  // Load existing configuration if it exists
  if (configFile_.existsAsFile()) {
    loadConfiguration();
  } else {
    // Create directory if it doesn't exist
    configFile_.getParentDirectory().createDirectory();
    saveConfiguration();
  }

  initialized_ = true;
}

void ConfigurationManager::shutdown() {
  juce::ScopedLock lock(lock_);

  if (!initialized_) {
    return;
  }

  // Save configuration on shutdown
  saveConfiguration();

  configData_ = nullptr;
  defaultData_ = nullptr;
  changeListeners_.clear();

  initialized_ = false;
}

ConfigValue ConfigurationManager::getValue(const juce::String &key) const {
  juce::ScopedLock lock(lock_);

  juce::var value = getNestedValue(key);

  if (!value.isVoid()) {
    return ConfigValue::fromVar(value);
  }

  // Return default if available
  if (defaultData_) {
    juce::var defaultValue = getNestedValueFromObject(key, defaultData_);
    if (!defaultValue.isVoid()) {
      return ConfigValue::fromVar(defaultValue);
    }
  }

  return ConfigValue();
}

ConfigValue
ConfigurationManager::getValue(const juce::String &key,
                               const ConfigValue &defaultValue) const {
  juce::ScopedLock lock(lock_);

  ConfigValue value = getValue(key);

  // If value is not set, return provided default
  if (value.getType() == ConfigValue::Type::String &&
      value.getString().isEmpty()) {
    return defaultValue;
  }

  return value;
}

void ConfigurationManager::setValue(const juce::String &key,
                                    const ConfigValue &value) {
  juce::ScopedLock lock(lock_);

  if (!initialized_) {
    return;
  }

  setNestedValue(key, value.toVar());
  notifyChangeListeners(key, value);
}

void ConfigurationManager::setValue(const juce::String &key, bool value) {
  setValue(key, ConfigValue(value));
}

void ConfigurationManager::setValue(const juce::String &key, int value) {
  setValue(key, ConfigValue(value));
}

void ConfigurationManager::setValue(const juce::String &key, float value) {
  setValue(key, ConfigValue(value));
}

void ConfigurationManager::setValue(const juce::String &key,
                                    const juce::String &value) {
  setValue(key, ConfigValue(value));
}

void ConfigurationManager::setValue(const juce::String &key, SkColor value) {
  setValue(key, ConfigValue(value));
}

bool ConfigurationManager::getBool(const juce::String &key) const {
  return getValue(key).getBool();
}

bool ConfigurationManager::getBool(const juce::String &key,
                                   bool defaultValue) const {
  ConfigValue value = getValue(key);
  if (value.getType() == ConfigValue::Type::Bool) {
    return value.getBool();
  }
  return defaultValue;
}

void ConfigurationManager::setBool(const juce::String &key, bool value) {
  setValue(key, value);
}

int ConfigurationManager::getInt(const juce::String &key) const {
  return getValue(key).getInt();
}

int ConfigurationManager::getInt(const juce::String &key,
                                 int defaultValue) const {
  ConfigValue value = getValue(key);
  if (value.getType() == ConfigValue::Type::Int) {
    return value.getInt();
  }
  return defaultValue;
}

void ConfigurationManager::setInt(const juce::String &key, int value) {
  setValue(key, value);
}

float ConfigurationManager::getFloat(const juce::String &key) const {
  return getValue(key).getFloat();
}

float ConfigurationManager::getFloat(const juce::String &key,
                                     float defaultValue) const {
  ConfigValue value = getValue(key);
  if (value.getType() == ConfigValue::Type::Float) {
    return value.getFloat();
  }
  return defaultValue;
}

void ConfigurationManager::setFloat(const juce::String &key, float value) {
  setValue(key, value);
}

juce::String ConfigurationManager::getString(const juce::String &key) const {
  return getValue(key).getString();
}

juce::String
ConfigurationManager::getString(const juce::String &key,
                                const juce::String &defaultValue) const {
  ConfigValue value = getValue(key);
  juce::String strValue = value.getString();
  return strValue.isEmpty() ? defaultValue : strValue;
}

void ConfigurationManager::setString(const juce::String &key,
                                     const juce::String &value) {
  setValue(key, value);
}

SkColor ConfigurationManager::getColor(const juce::String &key) const {
  return getValue(key).getColor();
}

SkColor ConfigurationManager::getColor(const juce::String &key,
                                       SkColor defaultValue) const {
  ConfigValue value = getValue(key);
  if (value.getType() == ConfigValue::Type::Color) {
    return value.getColor();
  }
  return defaultValue;
}

void ConfigurationManager::setColor(const juce::String &key, SkColor value) {
  setValue(key, value);
}

juce::DynamicObject::Ptr
ConfigurationManager::getGroup(const juce::String &groupKey) const {
  juce::ScopedLock lock(lock_);

  juce::var value = getNestedValue(groupKey);

  if (value.isObject()) {
    return value.getDynamicObject();
  }

  return nullptr;
}

void ConfigurationManager::setGroup(const juce::String &groupKey,
                                    const juce::DynamicObject::Ptr &group) {
  juce::ScopedLock lock(lock_);

  if (!initialized_ || !group) {
    return;
  }

  setNestedValue(groupKey, juce::var(group.get()));
}

bool ConfigurationManager::loadConfiguration() {
  juce::ScopedLock lock(lock_);

  if (!configFile_.existsAsFile()) {
    return false;
  }

  try {
    auto json = configFile_.loadFileAsString();
    auto parsed = juce::JSON::parse(json);

    if (parsed.isObject()) {
      configData_ = parsed.getDynamicObject();
      return true;
    }
  } catch (const std::exception &e) {
    DBG("Failed to load configuration: " << e.what());
  }

  return false;
}

bool ConfigurationManager::saveConfiguration() {
  juce::ScopedLock lock(lock_);

  if (!initialized_ || !configData_) {
    return false;
  }

  try {
    juce::String json =
        juce::JSON::toString(juce::var(configData_.get()), true);
    return configFile_.replaceWithText(json);
  } catch (const std::exception &e) {
    DBG("Failed to save configuration: " << e.what());
    return false;
  }
}

bool ConfigurationManager::saveConfigurationAs(const juce::File &newFile) {
  juce::ScopedLock lock(lock_);

  if (!initialized_ || !configData_) {
    return false;
  }

  try {
    juce::String json =
        juce::JSON::toString(juce::var(configData_.get()), true);
    bool success = newFile.replaceWithText(json);

    if (success) {
      configFile_ = newFile;
    }

    return success;
  } catch (const std::exception &e) {
    DBG("Failed to save configuration: " << e.what());
    return false;
  }
}

void ConfigurationManager::setConfigurationFile(const juce::File &file) {
  juce::ScopedLock lock(lock_);
  configFile_ = file;
}

bool ConfigurationManager::exportConfiguration(const juce::File &destination) {
  return saveConfigurationAs(destination);
}

bool ConfigurationManager::importConfiguration(const juce::File &source) {
  juce::ScopedLock lock(lock_);

  if (!source.existsAsFile()) {
    return false;
  }

  juce::File oldConfigFile = configFile_;
  configFile_ = source;

  if (loadConfiguration()) {
    // Save to current config file location
    bool success = saveConfiguration();
    configFile_ = oldConfigFile;
    return success;
  }

  configFile_ = oldConfigFile;
  return false;
}

void ConfigurationManager::setDefaultValue(const juce::String &key,
                                           const ConfigValue &value) {
  juce::ScopedLock lock(lock_);

  if (!defaultData_) {
    defaultData_ = new juce::DynamicObject();
  }

  setNestedValueInObject(key, value.toVar(), defaultData_);
}

void ConfigurationManager::setDefaultValues(
    const juce::DynamicObject::Ptr &defaults) {
  juce::ScopedLock lock(lock_);
  defaultData_ = defaults;
}

void ConfigurationManager::resetToDefaults() {
  juce::ScopedLock lock(lock_);

  if (defaultData_) {
    configData_ = new juce::DynamicObject(*defaultData_);
    saveConfiguration();
  }
}

void ConfigurationManager::resetKeyToDefault(const juce::String &key) {
  juce::ScopedLock lock(lock_);

  if (defaultData_) {
    juce::var defaultValue = getNestedValueFromObject(key, defaultData_);
    if (!defaultValue.isVoid()) {
      setNestedValue(key, defaultValue);
    }
  }
}

void ConfigurationManager::addChangeListener(ConfigChangeCallback callback) {
  juce::ScopedLock lock(lock_);
  changeListeners_.add(callback);
}

void ConfigurationManager::removeChangeListener(ConfigChangeCallback callback) {
  // NOOP: std::function cannot be compared with operator==
  // In a real implementation, you would need to use wrapper with an ID
  juce::ignoreUnused(callback);
}

void ConfigurationManager::notifyChangeListeners(const juce::String &key,
                                                 const ConfigValue &value) {
  for (auto &callback : changeListeners_) {
    callback(key, value);
  }
}

bool ConfigurationManager::hasKey(const juce::String &key) const {
  juce::ScopedLock lock(lock_);
  juce::var value = getNestedValue(key);
  return !value.isVoid();
}

juce::StringArray ConfigurationManager::getAllKeys() const {
  juce::ScopedLock lock(lock_);

  juce::StringArray keys;
  if (configData_) {
    for (auto &pair : configData_->getProperties()) {
      keys.add(pair.name.toString());
    }
  }
  return keys;
}

juce::StringArray
ConfigurationManager::getKeysWithPrefix(const juce::String &prefix) const {
  juce::ScopedLock lock(lock_);

  juce::StringArray keys;
  juce::StringArray allKeys = getAllKeys();

  for (const auto &key : allKeys) {
    if (key.startsWith(prefix)) {
      keys.add(key);
    }
  }

  return keys;
}

bool ConfigurationManager::isValidKeyType(
    const juce::String &key, ConfigValue::Type expectedType) const {
  ConfigValue value = getValue(key);
  return value.getType() == expectedType;
}

bool ConfigurationManager::backupConfiguration() {
  juce::ScopedLock lock(lock_);

  if (!configFile_.existsAsFile()) {
    return false;
  }

  juce::File backupDir =
      configFile_.getParentDirectory().getChildFile("backups");
  backupDir.createDirectory();

  juce::String timestamp =
      juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
  juce::File backupFile = backupDir.getChildFile(
      configFile_.getFileNameWithoutExtension() + "_" + timestamp + ".json");

  return configFile_.copyFileTo(backupFile);
}

bool ConfigurationManager::restoreFromBackup() {
  juce::ScopedLock lock(lock_);

  juce::File backupDir =
      configFile_.getParentDirectory().getChildFile("backups");

  if (!backupDir.exists()) {
    return false;
  }

  // Find the most recent backup
  juce::Array<juce::File> backupFiles;
  backupDir.findChildFiles(backupFiles, juce::File::findFiles, false, "*.json");

  if (backupFiles.isEmpty()) {
    return false;
  }

  // Sort by modification time (newest first)
  std::sort(backupFiles.begin(), backupFiles.end(),
            [](const juce::File &a, const juce::File &b) {
              return a.getLastModificationTime() > b.getLastModificationTime();
            });

  // Restore from most recent backup
  return importConfiguration(backupFiles[0]);
}

juce::File ConfigurationManager::getBackupFile() const { return backupFile_; }

int ConfigurationManager::getConfigurationCount() const {
  juce::ScopedLock lock(lock_);
  return getAllKeys().size();
}

juce::String ConfigurationManager::getConfigurationStatistics() const {
  juce::ScopedLock lock(lock_);

  juce::String stats;
  stats << "=== Configuration Statistics ===\n";
  stats << "Total Keys: " << getConfigurationCount() << "\n";
  stats << "Configuration File: " << configFile_.getFullPathName() << "\n";
  stats << "Initialized: " << (initialized_ ? "Yes" : "No") << "\n";

  return stats;
}

void ConfigurationManager::migrateFromVersion(const juce::String &version) {
  juce::ScopedLock lock(lock_);

  // Migration logic would go here
  // This would handle upgrading from older configuration versions
  juce::ignoreUnused(version);
}

juce::String ConfigurationManager::getConfigurationVersion() const {
  return getString("config.version", "1.0.0");
}

void ConfigurationManager::setConfigurationVersion(
    const juce::String &version) {
  setString("config.version", version);
}

void ConfigurationManager::loadDefaults() {
  // UI State defaults
  setDefaultValue(keys::WINDOW_X, ConfigValue(100));
  setDefaultValue(keys::WINDOW_Y, ConfigValue(100));
  setDefaultValue(keys::WINDOW_WIDTH, ConfigValue(1400));
  setDefaultValue(keys::WINDOW_HEIGHT, ConfigValue(900));
  setDefaultValue(keys::WINDOW_MAXIMIZED, ConfigValue(false));

  // Arranger defaults
  setDefaultValue(keys::ARRANGER_ZOOM, ConfigValue(50.0f)); // pixelsPerBeat
  setDefaultValue(keys::ARRANGER_SCROLL_X, ConfigValue(0.0f));
  setDefaultValue(keys::ARRANGER_SCROLL_Y, ConfigValue(0.0f));
  setDefaultValue(keys::ARRANGER_FOLLOW_PLAYHEAD, ConfigValue(true));

  // Theme defaults
  setDefaultValue(keys::THEME_NAME, ConfigValue("NeonNoir"));
  setDefaultValue(keys::THEME_ACCENT_COLOR, ConfigValue(design::colors::CYAN));
  setDefaultValue(keys::THEME_BACKGROUND_COLOR,
                  ConfigValue(design::colors::BG_DARKEST));
  setDefaultValue(keys::THEME_OLED_MODE, ConfigValue(false));
  setDefaultValue(keys::THEME_GLOW_INTENSITY, ConfigValue(1.0f));
  setDefaultValue(keys::THEME_UI_SCALE, ConfigValue(1.0f));

  // Layout defaults
  setDefaultValue(keys::LAYOUT_BROWSER_VISIBLE, ConfigValue(true));
  setDefaultValue(keys::LAYOUT_BROWSER_WIDTH, ConfigValue(280));
  setDefaultValue(keys::LAYOUT_RIGHT_PANEL_VISIBLE, ConfigValue(true));
  setDefaultValue(keys::LAYOUT_RIGHT_PANEL_WIDTH, ConfigValue(320));
  setDefaultValue(keys::LAYOUT_BOTTOM_PANEL_VISIBLE, ConfigValue(false));
  setDefaultValue(keys::LAYOUT_BOTTOM_PANEL_HEIGHT, ConfigValue(200));
  setDefaultValue(keys::LAYOUT_MIXER_VISIBLE, ConfigValue(false));
  setDefaultValue(keys::LAYOUT_MIXER_HEIGHT, ConfigValue(300));

  // Performance defaults
  setDefaultValue(keys::PERF_FPS_LIMIT, ConfigValue(60));
  setDefaultValue(keys::PERF_VSYNC, ConfigValue(true));
  setDefaultValue(keys::PERF_GPU_ACCELERATION, ConfigValue(true));
  setDefaultValue(keys::PERF_METER_REFRESH_RATE, ConfigValue(30));

  // User preference defaults
  setDefaultValue(keys::PREF_AUTO_SAVE_ENABLED, ConfigValue(true));
  setDefaultValue(keys::PREF_AUTO_SAVE_INTERVAL, ConfigValue(5));
  setDefaultValue(keys::PREF_TOOLTIPS_ENABLED, ConfigValue(true));
  setDefaultValue(keys::PREF_TOOLTIPS_DELAY, ConfigValue(500));
  setDefaultValue(keys::PREF_WELCOME_SCREEN, ConfigValue(true));
  setDefaultValue(keys::PREF_RECENT_FILES_MAX, ConfigValue(10));
  setDefaultValue(keys::PREF_UNDO_LIMIT, ConfigValue(100));

  // Project defaults
  setDefaultValue(keys::PROJECT_DEFAULT_TEMPO, ConfigValue(120.0f));
  setDefaultValue(keys::PROJECT_DEFAULT_TIMESIG_NUM, ConfigValue(4));
  setDefaultValue(keys::PROJECT_DEFAULT_TIMESIG_DEN, ConfigValue(4));
  setDefaultValue(keys::PROJECT_DEFAULT_SAMPLE_RATE, ConfigValue(44100));
  setDefaultValue(keys::PROJECT_DEFAULT_BUFFER_SIZE, ConfigValue(512));

  // Collaboration defaults
  setDefaultValue(keys::COLLAB_SERVER_IP, ConfigValue("216.126.231.46"));
}

juce::var ConfigurationManager::getNestedValue(const juce::String &key) const {
  return getNestedValueFromObject(key, configData_);
}

juce::var ConfigurationManager::getNestedValueFromObject(
    const juce::String &key, const juce::DynamicObject::Ptr &object) const {
  if (!object) {
    return juce::var();
  }

  juce::StringArray keyParts = splitKey(key);

  juce::DynamicObject::Ptr currentObject = object;
  for (int i = 0; i < keyParts.size() - 1; ++i) {
    juce::var value = currentObject->getProperty(keyParts[i]);
    if (value.isObject()) {
      currentObject = value.getDynamicObject();
    } else {
      return juce::var();
    }
  }

  return currentObject->getProperty(keyParts[keyParts.size() - 1]);
}

void ConfigurationManager::setNestedValue(const juce::String &key,
                                          const juce::var &value) {
  setNestedValueInObject(key, value, configData_);
}

void ConfigurationManager::setNestedValueInObject(
    const juce::String &key, const juce::var &value,
    const juce::DynamicObject::Ptr &object) {
  if (!object) {
    return;
  }

  juce::StringArray keyParts = splitKey(key);

  juce::DynamicObject::Ptr currentObject = object;
  for (int i = 0; i < keyParts.size() - 1; ++i) {
    juce::var existingValue = currentObject->getProperty(keyParts[i]);

    if (!existingValue.isObject()) {
      juce::DynamicObject::Ptr newObject = new juce::DynamicObject();
      currentObject->setProperty(keyParts[i], juce::var(newObject.get()));
      currentObject = newObject;
    } else {
      currentObject = existingValue.getDynamicObject();
    }
  }

  currentObject->setProperty(keyParts[keyParts.size() - 1], value);
}

juce::StringArray
ConfigurationManager::splitKey(const juce::String &key) const {
  return juce::StringArray::fromTokens(key, ".", "\"");
}

juce::DynamicObject::Ptr
ConfigurationManager::ensureNestedObject(const juce::String &key) {
  juce::StringArray keyParts = splitKey(key);

  juce::DynamicObject::Ptr currentObject = configData_;
  for (int i = 0; i < keyParts.size() - 1; ++i) {
    juce::var existingValue = currentObject->getProperty(keyParts[i]);

    if (!existingValue.isObject()) {
      juce::DynamicObject::Ptr newObject = new juce::DynamicObject();
      currentObject->setProperty(keyParts[i], juce::var(newObject.get()));
      currentObject = newObject;
    } else {
      currentObject = existingValue.getDynamicObject();
    }
  }

  return currentObject;
}

} // namespace config
} // namespace zenith