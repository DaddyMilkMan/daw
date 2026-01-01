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

        // =====================================================================
        // Component Token Regression Tests
        // =====================================================================
        // These tests verify that component-level tokens correctly wrap
        // semantic tokens and return expected values.

        beginTest("Button Token Values");
        {
            // Background colors should match semantic colors
            expect(button::getBgPrimary() == colors::CYAN);
            expect(button::getBgSecondary() == colors::BG_02);
            expect(button::getBgDanger() == colors::RED);
            expect(button::getBgSuccess() == colors::GREEN);
            expect(button::getBgWarning() == colors::AMBER);

            // Text colors
            expect(button::getTextDefault() == colors::TEXT_PRIMARY);
            expect(button::getTextOnAccent() == colors::TEXT_INVERSE);
            expect(button::getTextDisabled() == colors::TEXT_TERTIARY);

            // Dimensions
            expectEquals(button::getHeightSm(), 24.0f);
            expectEquals(button::getHeightMd(), 32.0f);
            expectEquals(button::getHeightLg(), 40.0f);
            expect(button::getRadiusSm() == dimensions::RADIUS_SM);
            expect(button::getRadiusLg() == dimensions::RADIUS_LG);

            // Opacities should match semantic opacity values
            expectEquals(button::getHoverOpacity(), opacity::HOVER);
            expectEquals(button::getActiveOpacity(), opacity::ACTIVE);
            expectEquals(button::getDisabledOpacity(), opacity::DISABLED);
        }

        beginTest("Slider Token Values");
        {
            // Track colors
            expect(slider::getTrackBg() == colors::BG_01);
            expect(slider::getTrackFillDefault() == colors::CYAN);
            expect(slider::getTrackFillMuted() == colors::AMBER);
            expect(slider::getTrackFillSolo() == colors::NEON_GREEN);

            // Handle colors
            expect(slider::getHandleDefault() == colors::TEXT_PRIMARY);
            expect(slider::getHandleHover() == colors::CYAN);
            expect(slider::getHandleDisabled() == colors::TEXT_TERTIARY);

            // Dimensions
            expectEquals(slider::getTrackHeightHorizontal(), 4.0f);
            expectEquals(slider::getHandleSize(), 16.0f);
            expectEquals(slider::getHandleSizeLg(), 20.0f);
            expect(slider::getHandleRadius() == dimensions::RADIUS_FULL);
        }

        beginTest("Panel Token Values");
        {
            // Background colors by elevation
            expect(panel::getBg0() == colors::BG_00);
            expect(panel::getBg1() == colors::BG_01);
            expect(panel::getBg2() == colors::BG_02);
            expect(panel::getBg3() == colors::BG_03);
            expect(panel::getBg4() == colors::BG_04);

            // Border colors
            expect(panel::getBorderDefault() == colors::BORDER_DEFAULT);
            expect(panel::getBorderSubtle() == colors::BORDER_SUBTLE);
            expect(panel::getBorderStrong() == colors::BORDER_STRONG);

            // Glassmorphism
            expect(panel::getGlassHighlight() == colors::GLASS_HIGHLIGHT);
            expect(panel::getGlassShadow() == colors::GLASS_SHADOW);
            expectEquals(panel::getGlassBlur(), effects::BLUR_GLASS);

            // Dimensions
            expectEquals(panel::getPadding(), spacing::MD);
            expectEquals(panel::getPaddingSm(), spacing::SM);
            expectEquals(panel::getPaddingLg(), spacing::LG);
            expect(panel::getRadius() == dimensions::RADIUS_LG);
            expectEquals(panel::getHeaderHeight(), 32.0f);
        }

        beginTest("Token Consistency After Theme Change");
        {
            auto& tm = ThemeManager::getInstance();
            
            // Record values before theme change
            SkColor buttonBgBefore = button::getBgSecondary();
            SkColor sliderTrackBefore = slider::getTrackBg();
            SkColor panelBgBefore = panel::getBg2();
            
            // Switch theme
            tm.setActiveTheme(ThemePreset::Darker);
            
            // Component tokens should reflect updated semantic tokens
            // (they're inline functions that read from mutable colors)
            SkColor buttonBgAfter = button::getBgSecondary();
            SkColor sliderTrackAfter = slider::getTrackBg();
            SkColor panelBgAfter = panel::getBg2();
            
            // Values may change with theme, but function should still work
            expect(buttonBgAfter != 0);  // Valid color
            expect(sliderTrackAfter != 0);
            expect(panelBgAfter != 0);
            
            // Reset
            tm.resetToDefault();
            
            // Should return to original values
            expect(button::getBgSecondary() == buttonBgBefore);
            expect(slider::getTrackBg() == sliderTrackBefore);
            expect(panel::getBg2() == panelBgBefore);
        }
    }
};

static ThemeTests themeTests;

} // namespace design
} // namespace zenith
