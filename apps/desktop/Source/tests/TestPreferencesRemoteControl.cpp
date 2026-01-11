/**
 * @file TestPreferencesRemoteControl.cpp
 * @brief Unit tests for remote control preference setting
 * @author Zenith DAW Team
 *
 * Tests for the allowRemoteControl preference:
 * - Default value is false
 * - Toggling to true persists after save/load
 * - Engine::isRemoteControlAllowed() reflects persisted value
 */

#include "../Settings.h"
#include "../engine/Engine.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

/**
 * @class RemoteControlPreferenceTests
 * @brief Tests for remote control preference functionality
 */
class RemoteControlPreferenceTests : public juce::UnitTest {
public:
  RemoteControlPreferenceTests()
      : juce::UnitTest("Remote Control Preference", "Preferences") {}

  void runTest() override {
    beginTest("Default value is false");
    {
      // Get a fresh Settings instance
      auto& settings = Settings::getInstance();
      
      // Load settings (will use default if not set)
      settings.load();
      
      // Verify default is false
      expect(!settings.getAllowRemoteControl(), 
             "Default allowRemoteControl should be false");
    }

    beginTest("Setting to true and reading back");
    {
      auto& settings = Settings::getInstance();
      
      // Set to true
      settings.setAllowRemoteControl(true);
      
      // Verify it was set
      expect(settings.getAllowRemoteControl(), 
             "getAllowRemoteControl should return true after setting to true");
      
      // Set back to false
      settings.setAllowRemoteControl(false);
      
      // Verify it was changed
      expect(!settings.getAllowRemoteControl(), 
             "getAllowRemoteControl should return false after setting to false");
    }

    beginTest("Persistence after save/load cycle");
    {
      auto& settings = Settings::getInstance();
      
      // Set to true
      settings.setAllowRemoteControl(true);
      
      // Save (setAllowRemoteControl calls save internally, but be explicit)
      settings.save();
      
      // Verify still true before reload
      expect(settings.getAllowRemoteControl(), 
             "Should be true before reload");
      
      // Reload settings from disk
      settings.load();
      
      // Verify persisted value is true
      expect(settings.getAllowRemoteControl(), 
             "allowRemoteControl should persist as true after save/load cycle");
      
      // Clean up: set back to false
      settings.setAllowRemoteControl(false);
      settings.save();
    }

    beginTest("Engine::isRemoteControlAllowed reflects setting");
    {
      auto& settings = Settings::getInstance();
      
      // Create an Engine instance
      Engine engine;
      
      // Set to false
      settings.setAllowRemoteControl(false);
      
      // Verify Engine API reflects false
      expect(!engine.isRemoteControlAllowed(), 
             "Engine::isRemoteControlAllowed should return false when setting is false");
      
      // Set to true
      settings.setAllowRemoteControl(true);
      
      // Verify Engine API reflects true
      expect(engine.isRemoteControlAllowed(), 
             "Engine::isRemoteControlAllowed should return true when setting is true");
      
      // Clean up
      settings.setAllowRemoteControl(false);
    }

    beginTest("Multiple save/load cycles");
    {
      auto& settings = Settings::getInstance();
      
      // Cycle 1: Set to true
      settings.setAllowRemoteControl(true);
      settings.save();
      settings.load();
      expect(settings.getAllowRemoteControl(), 
             "Cycle 1: Should be true after save/load");
      
      // Cycle 2: Set to false
      settings.setAllowRemoteControl(false);
      settings.save();
      settings.load();
      expect(!settings.getAllowRemoteControl(), 
             "Cycle 2: Should be false after save/load");
      
      // Cycle 3: Set to true again
      settings.setAllowRemoteControl(true);
      settings.save();
      settings.load();
      expect(settings.getAllowRemoteControl(), 
             "Cycle 3: Should be true after save/load");
      
      // Final cleanup
      settings.setAllowRemoteControl(false);
      settings.save();
    }
  }
};

// Register the test
static RemoteControlPreferenceTests remoteControlPreferenceTests;

} // namespace tests
} // namespace zenith
