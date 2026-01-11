/*
  ==============================================================================

    SettingsTests.cpp
    Created: 2026-01-11
    Author:  Zenith DAW

    Unit tests for Settings class, including remote control setting.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../Settings.h"
#include "../engine/Engine.h"

namespace zenith {

class SettingsTests : public juce::UnitTest {
public:
    SettingsTests() : juce::UnitTest("Settings", "Settings") {}

    void runTest() override {
        beginTest("Remote Control Setting - Default Value");
        {
            // The setting should default to false for security
            Settings& settings = Settings::getInstance();
            expect(!settings.getAllowRemoteControl(), 
                   "Remote control should be disabled by default");
        }

        beginTest("Remote Control Setting - Set and Get");
        {
            Settings& settings = Settings::getInstance();
            
            // Enable remote control
            settings.setAllowRemoteControl(true);
            expect(settings.getAllowRemoteControl(), 
                   "Remote control should be enabled after setting to true");
            
            // Disable remote control
            settings.setAllowRemoteControl(false);
            expect(!settings.getAllowRemoteControl(), 
                   "Remote control should be disabled after setting to false");
        }

        beginTest("Remote Control Setting - Persistence");
        {
            Settings& settings = Settings::getInstance();
            
            // Enable remote control
            settings.setAllowRemoteControl(true);
            
            // Force save (save is called automatically in setter)
            settings.save();
            
            // Reload settings
            settings.load();
            
            expect(settings.getAllowRemoteControl(), 
                   "Remote control setting should persist after save/load");
            
            // Clean up - disable for other tests
            settings.setAllowRemoteControl(false);
            settings.save();
        }

        beginTest("Remote Control Setting - Engine API");
        {
            Settings& settings = Settings::getInstance();
            Engine engine;
            
            // Test default state via Engine
            expect(!engine.isRemoteControlAllowed(), 
                   "Engine should report remote control disabled by default");
            
            // Enable via Settings
            settings.setAllowRemoteControl(true);
            
            // Verify via Engine
            expect(engine.isRemoteControlAllowed(), 
                   "Engine should report remote control enabled after setting");
            
            // Disable via Settings
            settings.setAllowRemoteControl(false);
            
            // Verify via Engine
            expect(!engine.isRemoteControlAllowed(), 
                   "Engine should report remote control disabled after unsetting");
        }

        beginTest("Remote Control Setting - Change Notification");
        {
            Settings& settings = Settings::getInstance();
            
            class SettingsListener : public juce::ChangeListener {
            public:
                int changeCount = 0;
                void changeListenerCallback(juce::ChangeBroadcaster*) override {
                    changeCount++;
                }
            };
            
            SettingsListener listener;
            settings.addChangeListener(&listener);
            
            // Initial state
            expect(listener.changeCount == 0, 
                   "No changes should be broadcast initially");
            
            // Toggle setting
            settings.setAllowRemoteControl(true);
            expect(listener.changeCount == 1, 
                   "One change should be broadcast after enabling");
            
            settings.setAllowRemoteControl(false);
            expect(listener.changeCount == 2, 
                   "Two changes should be broadcast after disabling");
            
            // Setting to same value should not broadcast
            settings.setAllowRemoteControl(false);
            expect(listener.changeCount == 2, 
                   "No additional changes should be broadcast for same value");
            
            settings.removeChangeListener(&listener);
        }
    }
};

// Register the test
static SettingsTests settingsTests;

} // namespace zenith
