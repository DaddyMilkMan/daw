/**
 * @file MasterOutputComponent.cpp
 * @brief Master output component implementation with smooth animations
 * 
 * DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens
 */

#include "MasterOutputComponent.h"
#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkRRect.h>
    #include <include/effects/SkGradientShader.h>
    #include "../Source/ui/skia/SkiaTheme.h"
#endif
#include "../../include/Engine.h"
#include "ZenithLookAndFeel.h"  // DESIGN SYSTEM: Include for design tokens

namespace zenith {

//==============================================================================
MasterOutputComponent::MasterOutputComponent(Engine& eng)
    : engine_(eng)
{
    setSize(100, 300);
    startTimer(16);  // 60 Hz refresh rate
}

MasterOutputComponent::~MasterOutputComponent()
{
    stopTimer();
}

//==============================================================================
void MasterOutputComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // DESIGN SYSTEM: Dark background with elevation token
    g.fillAll(juce::Colour(ZenithLookAndFeel::Elevation::dp4));
    
    // DESIGN SYSTEM: Border using borderSubtle token
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    g.drawRect(bounds, 1);

    // Layout: left side = fader, right side = meters + peak
    auto faderArea = bounds.removeFromLeft(FADER_WIDTH + SPACING);
    auto metersArea = bounds;

    // Paint fader section
    paintMasterFader(g, faderArea);

    // Paint level meters
    paintLevelMeters(g, metersArea.removeFromLeft(METER_WIDTH + SPACING));

    // Paint peak indicators
    paintPeakIndicators(g, metersArea);
}

void MasterOutputComponent::resized()
{
    // All drawing handled in paint()
}

void MasterOutputComponent::timerCallback()
{
    updateAnimatedLevels();
    // Calculate fader position (0 dB = middle, -inf = top, +12 = bottom)
    // Range: -60 to +12 dB
    float normalizedGain = (masterGaindB_ + 60.0f) / 72.0f;
    normalizedGain = juce::jlimit(0.0f, 1.0f, normalizedGain);

    int faderY = faderBounds.getY() + (int)(faderBounds.getHeight() * (1.0f - normalizedGain));

    // DESIGN SYSTEM: Draw fader thumb with accentPrimary gradient
    auto thumbBounds = juce::Rectangle<int>(faderBounds.getX(), faderY - 6, faderBounds.getWidth(), 12);
    juce::ColourGradient gradient(
        juce::Colour(ZenithLookAndFeel::Colors::accentPrimaryHover),  // Light top
        (float)thumbBounds.getX(), (float)thumbBounds.getY(),
        juce::Colour(ZenithLookAndFeel::Colors::accentPrimary),       // Accent bottom
        (float)thumbBounds.getX(), (float)thumbBounds.getBottom(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(thumbBounds.toFloat(), ZenithLookAndFeel::Radius::xs);

    // DESIGN SYSTEM: Thumb border highlight using accentPrimaryHover
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimaryHover).withAlpha(0.8f));
    g.drawRoundedRectangle(thumbBounds.toFloat().expanded(1.0f), ZenithLookAndFeel::Radius::xs, 1.0f);

    // DESIGN SYSTEM: Display dB value using textPrimary
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.setFont(ZenithLookAndFeel::Typography::getSmallBold());
    juce::String dbText = juce::String(masterGaindB_, 1) + " dB";
    g.drawText(dbText, faderBounds.withHeight(20), juce::Justification::centred, true);
}

void MasterOutputComponent::paintLevelMeters(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    auto meterBounds = bounds.reduced(ZenithLookAndFeel::Spacing::xs, ZenithLookAndFeel::Spacing::l);

    // DESIGN SYSTEM: Background using elevation
    g.setColour(juce::Colour(ZenithLookAndFeel::Elevation::dp1));
    g.fillRoundedRectangle(meterBounds.toFloat(), ZenithLookAndFeel::Radius::s);

    // DESIGN SYSTEM: Border using borderMedium
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderMedium));
    g.drawRoundedRectangle(meterBounds.toFloat(), ZenithLookAndFeel::Radius::s, 1.0f);

    // Draw level meter bars (smoothly animated)
    float meterHeight = (float)meterBounds.getHeight();

    // DESIGN SYSTEM: Current level bar with proper meter colors
    float currentBarHeight = animatedCurrentLevel_ * meterHeight;
    auto currentBarBounds = meterBounds
        .withY(meterBounds.getBottom() - (int)currentBarHeight)
        .withHeight((int)currentBarHeight);

    if (currentBarHeight > 0.5f) {
        // DESIGN SYSTEM: Use meter colors based on level
        // Green: 0-60% (-18dB to -6dB safe zone)
        // Amber: 60-90% (-6dB to 0dB caution)
        // Red: 90%+ (0dB+ clipping)
        juce::Colour barColor;
        if (animatedCurrentLevel_ < 0.6f) {
            barColor = juce::Colour(ZenithLookAndFeel::Colors::meterGreen);
        } else if (animatedCurrentLevel_ < 0.9f) {
            barColor = juce::Colour(ZenithLookAndFeel::Colors::meterAmber);
        } else {
            barColor = juce::Colour(ZenithLookAndFeel::Colors::meterRed);
        }
        
        g.setColour(barColor);
        g.fillRect(currentBarBounds);
    }

    // DESIGN SYSTEM: Peak level indicator line using meterRed
    float peakLineY = meterBounds.getBottom() - (animatedPeakLevel_ * meterHeight);
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::meterRed));
    g.drawHorizontalLine((int)peakLineY, (float)meterBounds.getX(), (float)meterBounds.getRight());

    // DESIGN SYSTEM: dB scale labels using textDisabled
    g.setFont(ZenithLookAndFeel::Typography::getTiny());
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textDisabled));
    g.drawText("-12", meterBounds.withHeight(12), juce::Justification::centred, true);
    g.drawText("0", meterBounds.withTop(meterBounds.getCentreY() - 6).withHeight(12),
               juce::Justification::centred, true);
}

void MasterOutputComponent::paintPeakIndicators(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Show headroom and peak values
    auto textBounds = bounds.reduced(ZenithLookAndFeel::Spacing::xs, ZenithLookAndFeel::Spacing::l);

    g.setFont(ZenithLookAndFeel::Typography::getTiny());

    // Headroom indicator
    float headroom = 1.0f - animatedPeakLevel_;
    headroom = juce::jlimit(0.0f, 1.0f, headroom);

    juce::String headroomText = "HM: " + juce::String(gainToDb(headroom), 1) + " dB";
    
    // DESIGN SYSTEM: Headroom color - meterRed if low, success if good
    g.setColour(headroom < 0.1f 
        ? juce::Colour(ZenithLookAndFeel::Colors::meterRed) 
        : juce::Colour(ZenithLookAndFeel::Colors::success));
    g.drawText(headroomText, textBounds.removeFromTop(16), juce::Justification::centredLeft, true);

    // DESIGN SYSTEM: Peak value using meterRed if clipping, textPrimary if safe
    juce::String peakText = "P: " + juce::String(gainToDb(animatedPeakLevel_), 1) + " dB";
    g.setColour(animatedPeakLevel_ > 0.95f 
        ? juce::Colour(ZenithLookAndFeel::Colors::meterRed) 
        : juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.drawText(peakText, textBounds, juce::Justification::centredLeft, true);
}

//==============================================================================
void MasterOutputComponent::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds().removeFromLeft(FADER_WIDTH + SPACING).reduced(ZenithLookAndFeel::Spacing::s, ZenithLookAndFeel::Spacing::l);

    if (bounds.contains(event.getPosition())) {
        isDraggingFader_ = true;
        faderStartY_ = event.getPosition().y;
        faderStartValue_ = masterGaindB_;
    }
}

void MasterOutputComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingFader_) {
        int deltaY = event.getPosition().y - faderStartY_;
        float deltaDb = -(deltaY / 2.0f);  // -60 to +12 range

        setMasterGaindB(faderStartValue_ + deltaDb);
    }
}

void MasterOutputComponent::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isDraggingFader_ = false;
}

//==============================================================================
void MasterOutputComponent::setMasterGaindB(float gaindB)
{
    masterGaindB_ = juce::jlimit(-60.0f, 12.0f, gaindB);
    repaint();
}

void MasterOutputComponent::resetPeaks()
{
    peakHoldFrames_ = 0;
    animatedPeakLevel_ = 0.0f;
    engine_.resetPeakMeters();
}

//==============================================================================
void MasterOutputComponent::updateAnimatedLevels()
{
    // Get current and peak levels from engine
    float currentLevel = engine_.getMasterLevel();
    float peakLevel = engine_.getMasterPeakLevel();

    // Smooth animation towards current level (decay when signal drops)
    const float RELEASE = 0.95f;  // Smooth release

    animatedCurrentLevel_ = (currentLevel > animatedCurrentLevel_)
        ? currentLevel
        : animatedCurrentLevel_ * RELEASE;

    // Peak level with hold time
    if (peakLevel > animatedPeakLevel_) {
        animatedPeakLevel_ = peakLevel;
        peakHoldFrames_ = PEAK_HOLD_TIME_FRAMES;
    } else if (peakHoldFrames_ > 0) {
        peakHoldFrames_--;
    } else {
        animatedPeakLevel_ *= 0.98f;  // Slow decay
    }

    // Clamp to valid range
    animatedCurrentLevel_ = juce::jlimit(0.0f, 2.0f, animatedCurrentLevel_);
    animatedPeakLevel_ = juce::jlimit(0.0f, 2.0f, animatedPeakLevel_);
}

//==============================================================================
float MasterOutputComponent::dbToGain(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

float MasterOutputComponent::gainToDb(float gain)
{
    return 20.0f * std::log10(juce::jlimit(0.0001f, 100.0f, gain));
}

}  // namespace zenith

#ifdef ZENITH_USE_SKIA
void MasterOutputComponent::paintToSkia(SkCanvas* canvas, SkRect bounds)
{
    auto& theme = zenith::SkiaTheme::getInstance();
    canvas->clear(theme.getColors().bg1);

    SkPaint p;
    p.setColor(theme.getColors().textStrong);
    // TODO: Implement custom Skia rendering for MasterOutputComponent
}
#endif