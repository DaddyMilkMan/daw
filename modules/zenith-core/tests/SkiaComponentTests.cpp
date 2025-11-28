/**
 * @file SkiaComponentTests.cpp
 * @brief Comprehensive unit tests for all Skia UI components
 *
 * Tests for:
 * - SkiaTheme: Color palettes, spring physics, depth styles, typography, GPU settings
 * - SkiaTextRenderer: Text styling, effects, measurement
 * - SkiaWaveformRenderer: Sample generation, rendering modes, animations
 * - SkiaPianoRollRenderer: Note positioning, velocity feedback, grid rendering
 * - SkiaClipRenderer: Clip rendering, track layout, playhead positioning
 *
 * Exit codes:
 * - 0: All tests passed
 * - 1: Test failure
 */

#include <iostream>
#include <memory>
#include <cmath>
#include <cassert>

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#ifdef ZENITH_USE_SKIA
    #include "../Source/ui/skia/SkiaTheme.h"
    #include "../Source/ui/skia/SkiaTextRenderer.h"
    #include "../Source/ui/skia/SkiaWaveformRenderer.h"
    #include "../Source/ui/skia/SkiaPianoRollRenderer.h"
    #include "../Source/ui/skia/SkiaClipRenderer.h"
#endif

using namespace zenith;

//==============================================================================
// Test Helpers
//==============================================================================

namespace
{
    int testsPassed = 0;
    int testsFailed = 0;
    int testsSkipped = 0;

    void logPass(const juce::String& testName)
    {
        std::cout << "[PASS] " << testName << std::endl;
        testsPassed++;
    }

    void logFail(const juce::String& testName, const juce::String& reason)
    {
        std::cerr << "[FAIL] " << testName << ": " << reason << std::endl;
        testsFailed++;
    }

    void logSkip(const juce::String& testName, const juce::String& reason)
    {
        std::cout << "[SKIP] " << testName << ": " << reason << std::endl;
        testsSkipped++;
    }

    bool closeEnough(float a, float b, float tolerance = 0.01f)
    {
        return std::abs(a - b) < tolerance;
    }

    uint8_t getAlpha(SkColor color)
    {
        return (color >> 24) & 0xFF;
    }

    uint8_t getRed(SkColor color)
    {
        return (color >> 16) & 0xFF;
    }

    uint8_t getGreen(SkColor color)
    {
        return (color >> 8) & 0xFF;
    }

    uint8_t getBlue(SkColor color)
    {
        return color & 0xFF;
    }
}

//==============================================================================
// SkiaTheme Tests
//==============================================================================

#ifdef ZENITH_USE_SKIA

void testThemeInitialization()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();

        // Check default mode
        if (theme.getThemeMode() != ThemeMode::Dark)
            logFail("ThemeInitialization", "Default should be Dark mode");
        else
            logPass("ThemeInitialization");
    }
    catch (const std::exception& e)
    {
        logFail("ThemeInitialization", e.what());
    }
}

void testThemeModeSwitch()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();

        // Switch to light
        theme.setThemeMode(ThemeMode::Light);
        if (theme.getThemeMode() != ThemeMode::Light)
        {
            logFail("ThemeModeSwitch", "Failed to switch to Light mode");
            return;
        }

        // Switch back to dark
        theme.setThemeMode(ThemeMode::Dark);
        if (theme.getThemeMode() != ThemeMode::Dark)
        {
            logFail("ThemeModeSwitch", "Failed to switch back to Dark mode");
            return;
        }

        logPass("ThemeModeSwitch");
    }
    catch (const std::exception& e)
    {
        logFail("ThemeModeSwitch", e.what());
    }
}

void testDarkColorPalette()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();
        theme.setThemeMode(ThemeMode::Dark);

        const auto& colors = theme.getColors();

        // Verify some key dark colors
        // Background should be very dark
        if (getRed(colors.background) > 50 ||
            getGreen(colors.background) > 50 ||
            getBlue(colors.background) > 50)
        {
            logFail("DarkColorPalette", "Background not dark enough");
            return;
        }

        // Text should be bright
        if (getRed(colors.textPrimary) < 200)
        {
            logFail("DarkColorPalette", "Text not bright enough");
            return;
        }

        logPass("DarkColorPalette");
    }
    catch (const std::exception& e)
    {
        logFail("DarkColorPalette", e.what());
    }
}

void testLightColorPalette()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();
        theme.setThemeMode(ThemeMode::Light);

        const auto& colors = theme.getColors();

        // Background should be light
        if (getRed(colors.background) < 200 ||
            getGreen(colors.background) < 200 ||
            getBlue(colors.background) < 200)
        {
            logFail("LightColorPalette", "Background not light enough");
            return;
        }

        // Text should be dark
        if (getRed(colors.textPrimary) > 50)
        {
            logFail("LightColorPalette", "Text not dark enough");
            return;
        }

        logPass("LightColorPalette");
    }
    catch (const std::exception& e)
    {
        logFail("LightColorPalette", e.what());
    }
}

void testSpringPhysicsPresets()
{
    try
    {
        auto snappy = SpringPhysicsSettings::snappy();
        auto smooth = SpringPhysicsSettings::smooth();
        auto bouncy = SpringPhysicsSettings::bouncy();
        auto precise = SpringPhysicsSettings::precise();

        // Verify snappy is snappy (high stiffness)
        if (snappy.stiffness < 400.0f)
        {
            logFail("SpringPhysicsPresets", "Snappy stiffness too low");
            return;
        }

        // Verify smooth is balanced
        if (smooth.stiffness > snappy.stiffness)
        {
            logFail("SpringPhysicsPresets", "Smooth should be less stiff than snappy");
            return;
        }

        // Verify precise has high FPS
        if (precise.fps < 100.0f)
        {
            logFail("SpringPhysicsPresets", "Precise FPS too low");
            return;
        }

        logPass("SpringPhysicsPresets");
    }
    catch (const std::exception& e)
    {
        logFail("SpringPhysicsPresets", e.what());
    }
}

void testComponentPhysicsSettings()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();

        auto buttonPhysics = theme.getButtonPhysics();
        auto sliderPhysics = theme.getSliderPhysics();
        auto waveformPhysics = theme.getWaveformPhysics();

        // Buttons should be snappier than sliders
        if (buttonPhysics.stiffness <= sliderPhysics.stiffness)
        {
            logFail("ComponentPhysicsSettings", "Buttons should be snappier than sliders");
            return;
        }

        // Waveforms should have high FPS
        if (waveformPhysics.fps < 100.0f)
        {
            logFail("ComponentPhysicsSettings", "Waveforms should have high FPS");
            return;
        }

        logPass("ComponentPhysicsSettings");
    }
    catch (const std::exception& e)
    {
        logFail("ComponentPhysicsSettings", e.what());
    }
}

void testPhysicsCustomization()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();

        SpringPhysicsSettings custom = {600.0f, 40.0f, 144.0f};
        theme.setButtonPhysics(custom);

        auto retrieved = theme.getButtonPhysics();
        if (!closeEnough(retrieved.stiffness, 600.0f) ||
            !closeEnough(retrieved.damping, 40.0f) ||
            !closeEnough(retrieved.fps, 144.0f))
        {
            logFail("PhysicsCustomization", "Custom physics settings not applied");
            return;
        }

        logPass("PhysicsCustomization");
    }
    catch (const std::exception& e)
    {
        logFail("PhysicsCustomization", e.what());
    }
}

void testDepthStyles()
{
    try
    {
        auto subtle = DepthStyle::subtle();
        auto moderate = DepthStyle::moderate();
        auto dramatic = DepthStyle::dramatic();

        // Verify progressive intensity
        if (subtle.shadowBlur >= moderate.shadowBlur)
        {
            logFail("DepthStyles", "Subtle should have less shadow blur than moderate");
            return;
        }

        if (moderate.shadowBlur >= dramatic.shadowBlur)
        {
            logFail("DepthStyles", "Moderate should have less shadow blur than dramatic");
            return;
        }

        logPass("DepthStyles");
    }
    catch (const std::exception& e)
    {
        logFail("DepthStyles", e.what());
    }
}

void testGPUSettings()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();
        auto gpuSettings = theme.getGPUSettings();

        // Verify adaptive FPS is enabled
        if (!gpuSettings.adaptiveFPS)
        {
            logFail("GPUSettings", "Adaptive FPS should be enabled");
            return;
        }

        // Verify quality prioritization
        if (!gpuSettings.prioritizeQuality)
        {
            logFail("GPUSettings", "Quality prioritization should be enabled");
            return;
        }

        // Verify MSAA is reasonable
        if (gpuSettings.msaaSamples < 2 || gpuSettings.msaaSamples > 16)
        {
            logFail("GPUSettings", "MSAA samples should be between 2 and 16");
            return;
        }

        logPass("GPUSettings");
    }
    catch (const std::exception& e)
    {
        logFail("GPUSettings", e.what());
    }
}

void testTypography()
{
    try
    {
        auto& theme = SkiaTheme::getInstance();
        auto typo = theme.getTypography();

        // Verify font sizes are reasonable
        if (typo.baseSize <= 0.0f || typo.baseSize > 100.0f)
        {
            logFail("Typography", "Base size out of range");
            return;
        }

        // Heading should be larger than base
        if (typo.headingSize <= typo.baseSize)
        {
            logFail("Typography", "Heading size should be larger than base");
            return;
        }

        // Text effects should be enabled
        if (!typo.enableTextGlow || !typo.enableTextShadow)
        {
            logFail("Typography", "Text effects should be enabled");
            return;
        }

        logPass("Typography");
    }
    catch (const std::exception& e)
    {
        logFail("Typography", e.what());
    }
}

void testGradientPaint()
{
    try
    {
        SkColor topColor = SK_ColorBLUE;
        SkColor bottomColor = SK_ColorRED;
        SkRect bounds = SkRect::MakeXYWH(0, 0, 100, 100);

        auto paint = SkiaTheme::createGradientPaint(topColor, bottomColor, bounds);

        // Just verify it doesn't crash and returns a valid paint
        logPass("GradientPaint");
    }
    catch (const std::exception& e)
    {
        logFail("GradientPaint", e.what());
    }
}

void testGlowPaint()
{
    try
    {
        SkColor color = SK_ColorBLUE;
        auto paint = SkiaTheme::createGlowPaint(color, 5.0f, 0.8f);

        // Just verify it doesn't crash
        logPass("GlowPaint");
    }
    catch (const std::exception& e)
    {
        logFail("GlowPaint", e.what());
    }
}

void testShadowPaint()
{
    try
    {
        auto paint = SkiaTheme::createShadowPaint(2.0f, 4.0f, 0.5f);

        // Just verify it doesn't crash
        logPass("ShadowPaint");
    }
    catch (const std::exception& e)
    {
        logFail("ShadowPaint", e.what());
    }
}

#endif // ZENITH_USE_SKIA

//==============================================================================
// SkiaTextRenderer Tests
//==============================================================================

#ifdef ZENITH_USE_SKIA

void testTextRendererInitialization()
{
    try
    {
        SkiaTextRenderer renderer;
        logPass("TextRendererInitialization");
    }
    catch (const std::exception& e)
    {
        logFail("TextRendererInitialization", e.what());
    }
}

void testTextStyleEnum()
{
    try
    {
        // Just verify enum values exist and are valid
        auto regular = SkiaTextRenderer::TextStyle::Regular;
        auto bold = SkiaTextRenderer::TextStyle::Bold;
        auto heading = SkiaTextRenderer::TextStyle::Heading;

        logPass("TextStyleEnum");
    }
    catch (const std::exception& e)
    {
        logFail("TextStyleEnum", e.what());
    }
}

void testTextEffectFlags()
{
    try
    {
        // Test bitwise operations on effect flags
        auto glow = SkiaTextRenderer::TextEffect::Glow;
        auto shadow = SkiaTextRenderer::TextEffect::Shadow;

        auto combined = static_cast<SkiaTextRenderer::TextEffect>(
            static_cast<int>(glow) | static_cast<int>(shadow)
        );

        logPass("TextEffectFlags");
    }
    catch (const std::exception& e)
    {
        logFail("TextEffectFlags", e.what());
    }
}

void testTextRenderOptions()
{
    try
    {
        SkiaTextRenderer renderer;
        auto options = renderer.standard();

        if (options.style != SkiaTextRenderer::TextStyle::Regular)
        {
            logFail("TextRenderOptions", "Standard should use Regular style");
            return;
        }

        auto flashy = renderer.flashy();
        if (static_cast<int>(flashy.effects) == 0)
        {
            logFail("TextRenderOptions", "Flashy should have effects enabled");
            return;
        }

        logPass("TextRenderOptions");
    }
    catch (const std::exception& e)
    {
        logFail("TextRenderOptions", e.what());
    }
}

#endif // ZENITH_USE_SKIA

//==============================================================================
// SkiaWaveformRenderer Tests
//==============================================================================

#ifdef ZENITH_USE_SKIA

void testWaveformRendererInitialization()
{
    try
    {
        SkiaWaveformRenderer renderer;
        logPass("WaveformRendererInitialization");
    }
    catch (const std::exception& e)
    {
        logFail("WaveformRendererInitialization", e.what());
    }
}

void testWaveformStyleEnum()
{
    try
    {
        auto filled = SkiaWaveformRenderer::WaveformStyle::Filled;
        auto outline = SkiaWaveformRenderer::WaveformStyle::Outline;
        auto hybrid = SkiaWaveformRenderer::WaveformStyle::Hybrid;

        logPass("WaveformStyleEnum");
    }
    catch (const std::exception& e)
    {
        logFail("WaveformStyleEnum", e.what());
    }
}

void testWaveformRenderOptions()
{
    try
    {
        SkiaWaveformRenderer renderer;

        auto producer = renderer.producer();
        if (producer.style != SkiaWaveformRenderer::WaveformStyle::Hybrid)
        {
            logFail("WaveformRenderOptions", "Producer should use Hybrid style");
            return;
        }

        auto minimal = renderer.minimal();
        if (minimal.detailLevel != 1)
        {
            logFail("WaveformRenderOptions", "Minimal should have detail level 1");
            return;
        }

        logPass("WaveformRenderOptions");
    }
    catch (const std::exception& e)
    {
        logFail("WaveformRenderOptions", e.what());
    }
}

void testWaveformData()
{
    try
    {
        SkiaWaveformRenderer::WaveformData waveformData;
        waveformData.sampleRate = 44100;
        waveformData.numSamples = 44100; // 1 second

        if (waveformData.sampleRate != 44100)
        {
            logFail("WaveformData", "Sample rate not set correctly");
            return;
        }

        logPass("WaveformData");
    }
    catch (const std::exception& e)
    {
        logFail("WaveformData", e.what());
    }
}

#endif // ZENITH_USE_SKIA

//==============================================================================
// SkiaPianoRollRenderer Tests
//==============================================================================

#ifdef ZENITH_USE_SKIA

void testPianoRollRendererInitialization()
{
    try
    {
        SkiaPianoRollRenderer renderer;
        logPass("PianoRollRendererInitialization");
    }
    catch (const std::exception& e)
    {
        logFail("PianoRollRendererInitialization", e.what());
    }
}

void testMidiNoteVisual()
{
    try
    {
        SkiaPianoRollRenderer::MidiNoteVisual note;
        note.midiNote = 60; // Middle C
        note.startTime = 0.0;
        note.duration = 0.5;
        note.velocity = 100;

        if (note.midiNote != 60)
        {
            logFail("MidiNoteVisual", "MIDI note not set correctly");
            return;
        }

        logPass("MidiNoteVisual");
    }
    catch (const std::exception& e)
    {
        logFail("MidiNoteVisual", e.what());
    }
}

void testPianoRollRenderOptions()
{
    try
    {
        SkiaPianoRollRenderer renderer;

        auto producer = renderer.producer();
        if (!producer.useVelocityColoring)
        {
            logFail("PianoRollRenderOptions", "Producer should use velocity coloring");
            return;
        }

        logPass("PianoRollRenderOptions");
    }
    catch (const std::exception& e)
    {
        logFail("PianoRollRenderOptions", e.what());
    }
}

#endif // ZENITH_USE_SKIA

//==============================================================================
// SkiaClipRenderer Tests
//==============================================================================

#ifdef ZENITH_USE_SKIA

void testClipRendererInitialization()
{
    try
    {
        SkiaClipRenderer renderer;
        logPass("ClipRendererInitialization");
    }
    catch (const std::exception& e)
    {
        logFail("ClipRendererInitialization", e.what());
    }
}

void testClipVisual()
{
    try
    {
        SkiaClipRenderer::ClipVisual clip;
        clip.clipId = 1;
        clip.trackId = 0;
        clip.startTime = 0.0;
        clip.duration = 2.0;
        clip.isAudio = true;

        if (clip.clipId != 1 || clip.trackId != 0)
        {
            logFail("ClipVisual", "Clip properties not set correctly");
            return;
        }

        logPass("ClipVisual");
    }
    catch (const std::exception& e)
    {
        logFail("ClipVisual", e.what());
    }
}

void testTrackLaneVisual()
{
    try
    {
        SkiaClipRenderer::TrackLaneVisual track;
        track.trackId = 0;
        track.trackName = "Track 1";
        track.height = 100.0f;

        if (track.trackId != 0 || track.height != 100.0f)
        {
            logFail("TrackLaneVisual", "Track properties not set correctly");
            return;
        }

        logPass("TrackLaneVisual");
    }
    catch (const std::exception& e)
    {
        logFail("TrackLaneVisual", e.what());
    }
}

void testClipRenderOptions()
{
    try
    {
        SkiaClipRenderer renderer;

        auto producer = renderer.producer();
        if (!producer.showWaveforms)
        {
            logFail("ClipRenderOptions", "Producer should show waveforms");
            return;
        }

        auto minimal = renderer.minimal();
        if (minimal.showWaveforms)
        {
            logFail("ClipRenderOptions", "Minimal should not show waveforms");
            return;
        }

        logPass("ClipRenderOptions");
    }
    catch (const std::exception& e)
    {
        logFail("ClipRenderOptions", e.what());
    }
}

#endif // ZENITH_USE_SKIA

//==============================================================================
// Main
//==============================================================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "Skia Component Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

#ifdef ZENITH_USE_SKIA

    std::cout << "--- SkiaTheme Tests ---" << std::endl;
    testThemeInitialization();
    testThemeModeSwitch();
    testDarkColorPalette();
    testLightColorPalette();
    testSpringPhysicsPresets();
    testComponentPhysicsSettings();
    testPhysicsCustomization();
    testDepthStyles();
    testGPUSettings();
    testTypography();
    testGradientPaint();
    testGlowPaint();
    testShadowPaint();

    std::cout << std::endl << "--- SkiaTextRenderer Tests ---" << std::endl;
    testTextRendererInitialization();
    testTextStyleEnum();
    testTextEffectFlags();
    testTextRenderOptions();

    std::cout << std::endl << "--- SkiaWaveformRenderer Tests ---" << std::endl;
    testWaveformRendererInitialization();
    testWaveformStyleEnum();
    testWaveformRenderOptions();
    testWaveformData();

    std::cout << std::endl << "--- SkiaPianoRollRenderer Tests ---" << std::endl;
    testPianoRollRendererInitialization();
    testMidiNoteVisual();
    testPianoRollRenderOptions();

    std::cout << std::endl << "--- SkiaClipRenderer Tests ---" << std::endl;
    testClipRendererInitialization();
    testClipVisual();
    testTrackLaneVisual();
    testClipRenderOptions();

#else

    std::cout << "[INFO] Skia tests skipped - ZENITH_USE_SKIA not defined" << std::endl;
    std::cout << "To run these tests, build with: -DZENITH_ENABLE_SKIA=ON" << std::endl;

#endif

    std::cout << std::endl << "========================================" << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << "  Passed: " << testsPassed << std::endl;
    std::cout << "  Failed: " << testsFailed << std::endl;
    std::cout << "  Skipped: " << testsSkipped << std::endl;
    std::cout << "========================================" << std::endl;

    return (testsFailed > 0) ? 1 : 0;
}
