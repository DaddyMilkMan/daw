/**
 * @file TestPreferencesRemoteControl.cpp
 * @brief Unit tests for remote control preference setting
 * @author Zenith DAW Testing Team
 *
 * Tests the allowRemoteControl preference including:
 * - Default value is false
 * - Setting persists across save/load
 * - Engine accessor reflects persisted value
 */

#include "../../Settings.h"
#include "../../engine/Engine.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

/**
 * @class RemoteControlPreferenceTests
 * @brief Tests for remote control preference setting
 */
class RemoteControlPreferenceTests : public juce::UnitTest {
public:
    RemoteControlPreferenceTests()
        : juce::UnitTest("Remote Control Preferences", "Settings") {}

    void runTest() override {
        beginTest("Default value is false");
        {
            // Clear any existing settings to ensure clean state
            Settings::getInstance().setAllowRemoteControl(false);
            Settings::getInstance().save();
            
            // Create a fresh instance and verify default
            expect(!Settings::getInstance().getAllowRemoteControl(),
                   "Remote control should be disabled by default");
        }

        beginTest("Setting can be enabled");
        {
            Settings::getInstance().setAllowRemoteControl(true);
            expect(Settings::getInstance().getAllowRemoteControl(),
                   "Remote control should be enabled after setting to true");
        }

        beginTest("Setting can be disabled");
        {
            Settings::getInstance().setAllowRemoteControl(true);
            Settings::getInstance().setAllowRemoteControl(false);
            expect(!Settings::getInstance().getAllowRemoteControl(),
                   "Remote control should be disabled after setting to false");
        }

        beginTest("Setting persists across save");
        {
            // Enable and save
            Settings::getInstance().setAllowRemoteControl(true);
            Settings::getInstance().save();
            
            // Verify it persisted
            expect(Settings::getInstance().getAllowRemoteControl(),
                   "Remote control should remain enabled after save");
            
            // Disable and save
            Settings::getInstance().setAllowRemoteControl(false);
            Settings::getInstance().save();
            
            // Verify it persisted
            expect(!Settings::getInstance().getAllowRemoteControl(),
                   "Remote control should remain disabled after save");
        }

        beginTest("Setting persists across load");
        {
            // Set to true and save
            Settings::getInstance().setAllowRemoteControl(true);
            Settings::getInstance().save();
            
            // Reload settings
            Settings::getInstance().load();
            
            // Verify it loaded correctly
            expect(Settings::getInstance().getAllowRemoteControl(),
                   "Remote control should be enabled after load");
            
            // Set to false and save
            Settings::getInstance().setAllowRemoteControl(false);
            Settings::getInstance().save();
            
            // Reload settings
            Settings::getInstance().load();
            
            // Verify it loaded correctly
            expect(!Settings::getInstance().getAllowRemoteControl(),
                   "Remote control should be disabled after load");
        }

        beginTest("Engine accessor reflects setting");
        {
            Engine engine;
            
            // Test with disabled
            Settings::getInstance().setAllowRemoteControl(false);
            expect(!engine.isRemoteControlAllowed(),
                   "Engine should report remote control as disabled");
            
            // Test with enabled
            Settings::getInstance().setAllowRemoteControl(true);
            expect(engine.isRemoteControlAllowed(),
                   "Engine should report remote control as enabled");
            
            // Reset to default
            Settings::getInstance().setAllowRemoteControl(false);
        }

        beginTest("Multiple enable/disable cycles");
        {
            for (int i = 0; i < 5; ++i) {
                Settings::getInstance().setAllowRemoteControl(true);
                expect(Settings::getInstance().getAllowRemoteControl(),
                       "Remote control should be enabled in cycle " + juce::String(i));
                
                Settings::getInstance().setAllowRemoteControl(false);
                expect(!Settings::getInstance().getAllowRemoteControl(),
                       "Remote control should be disabled in cycle " + juce::String(i));
            }
        }

        // Clean up - ensure we leave settings in default state
        Settings::getInstance().setAllowRemoteControl(false);
        Settings::getInstance().save();
    }
};

// Register the test
static RemoteControlPreferenceTests remoteControlPreferenceTests;

} // namespace tests
} // namespace zenith
