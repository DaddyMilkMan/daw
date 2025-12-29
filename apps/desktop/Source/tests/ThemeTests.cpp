#include "../ui/design-system/ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace design {

class ThemeTests : public juce::UnitTest {
public:
    ThemeTests() : juce::UnitTest("Theme System Tests") {}

    void initialise() override {
        ThemeManager::getInstance().resetToDefault();
    }

    void shutdown() override {
        ThemeManager::getInstance().resetToDefault();
    }

    void runTest() override {
        beginTest("ThemeManager Preset Switching");
        {
            auto& tm = ThemeManager::getInstance();
            
            // Switch to Dark
            tm.setActiveTheme(ThemePreset::Dark);
            expect(tm.getActiveTheme() == ThemePreset::Dark);
            
            // Check a representative color
            expect(tm.getPalette().bgDarkest == SkColorSetARGB(255, 13, 13, 17));
            
            // Switch to Light
            tm.setActiveTheme(ThemePreset::Light);
            expect(tm.getActiveTheme() == ThemePreset::Light);
            expect(tm.getPalette().bgDarkest == SK_ColorWHITE);
            
            // Switch to Darker
            tm.setActiveTheme(ThemePreset::Darker);
            expect(tm.getActiveTheme() == ThemePreset::Darker);
            expect(tm.getPalette().bgDarkest == SK_ColorBLACK);
        }

        beginTest("Theme Listener Notification");
        {
            auto& tm = ThemeManager::getInstance();
            
            struct MockListener : public ThemeListener {
                int changeCount = 0;
                void themeChanged(ThemePreset) override {
                    changeCount++;
                }
            } listener;
            
            // Ensure we are NOT in Light mode initially (singleton state persistence)
            tm.setActiveTheme(ThemePreset::Darker);
            
            tm.addListener(&listener);
            
            // Start with Light theme
            tm.setActiveTheme(ThemePreset::Light);
            expectEquals(listener.changeCount, 1);
            
            // Switch back to Dark
            tm.setActiveTheme(ThemePreset::Dark);
            expectEquals(listener.changeCount, 2);
            
            tm.removeListener(&listener);
        }

        beginTest("FontManager Initialization");
        {
            auto& fm = FontManager::getInstance();
            // We can't easily test file loading without resources, 
            // but we can check if it returns a font.
            SkFont font = fm.getUIFont(14.0f);
            expect(font.getSize() == 14.0f);
            // Reset for next test
            auto& tm = ThemeManager::getInstance();
            tm.resetToDefault();
        }
    }
};

static ThemeTests themeTests;

} // namespace design
} // namespace zenith
