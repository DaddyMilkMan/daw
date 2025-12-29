#include <juce_gui_basics/juce_gui_basics.h>
#include <gtest/gtest.h>
#include "../ui/controls/ZenithButton.h"

// Manual Spy instead of GMock macros to avoid include/version issues
class SpyZenithLookAndFeel : public juce::LookAndFeel_V4 {
public:
    bool drawButtonBackgroundCalled = false;
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& c, bool h, bool d) override {
        drawButtonBackgroundCalled = true;
        // Call base to actually draw something if needed, or just no-op
    }
};

class ZenithButtonTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ZenithButtonTest, InitialStateIsVisible) {
    zenith::ZenithButton button("TestButton");
    EXPECT_TRUE(button.isVisible());
}

TEST_F(ZenithButtonTest, PaintCallsLookAndFeel) {
    zenith::ZenithButton button("TestButton");
    SpyZenithLookAndFeel spyLAF;
    button.setLookAndFeel(&spyLAF);

    // Simulate paint
    juce::Image image(juce::Image::RGB, 100, 30, true);
    juce::Graphics g(image);
    button.paintButton(g, false, false);

    EXPECT_TRUE(spyLAF.drawButtonBackgroundCalled);

    button.setLookAndFeel(nullptr);
}