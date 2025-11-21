/**
 * @file MasterOutputComponent.cpp
 * @brief Master output component implementation with smooth animations
 */

#include "MasterOutputComponent.h"
#include "../../include/Engine.h"

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

    // Dark background with subtle border (Apple-inspired)
    g.fillAll(juce::Colour(0xff2a2a2a));
    g.setColour(juce::Colour(0xff3a3a3a));
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
    repaint();
}

//==============================================================================
void MasterOutputComponent::paintMasterFader(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    auto faderBounds = bounds.reduced(8, 20);

    // Background track
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(faderBounds.toFloat(), 4.0f);

    // Border
    g.setColour(juce::Colour(0xff4a4a4a));
    g.drawRoundedRectangle(faderBounds.toFloat(), 4.0f, 1.0f);

    // Calculate fader position (0 dB = middle, -∞ = top, +12 = bottom)
    // Range: -60 to +12 dB
    float normalizedGain = (masterGaindB_ + 60.0f) / 72.0f;
    normalizedGain = juce::jlimit(0.0f, 1.0f, normalizedGain);

    int faderY = faderBounds.getY() + (int)(faderBounds.getHeight() * (1.0f - normalizedGain));

    // Draw fader thumb with gradient (Apple-like)
    auto thumbBounds = juce::Rectangle<int>(faderBounds.getX(), faderY - 6, faderBounds.getWidth(), 12);
    juce::ColourGradient gradient(
        juce::Colour(0xff4a9eff),  // Light blue top
        thumbBounds.getX(), thumbBounds.getY(),
        juce::Colour(0xff2563eb),  // Deep blue bottom
        thumbBounds.getX(), thumbBounds.getBottom(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(thumbBounds.toFloat(), 3.0f);

    // Thumb border highlight
    g.setColour(juce::Colour(0xff7bb5ff).withAlpha(0.8f));
    g.drawRoundedRectangle(thumbBounds.toFloat().expanded(1.0f), 3.0f, 1.0f);

    // Display dB value above fader
    g.setColour(juce::Colour(0xffcccccc));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    juce::String dbText = juce::String(masterGaindB_, 1) + " dB";
    g.drawText(dbText, faderBounds.withHeight(20), juce::Justification::centred, true);
}

void MasterOutputComponent::paintLevelMeters(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    auto meterBounds = bounds.reduced(4, 20);

    // Background
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(meterBounds.toFloat(), 4.0f);

    // Border
    g.setColour(juce::Colour(0xff4a4a4a));
    g.drawRoundedRectangle(meterBounds.toFloat(), 4.0f, 1.0f);

    // Draw level meter bars (smoothly animated)
    float meterHeight = (float)meterBounds.getHeight();

    // Current level bar (green)
    float currentBarHeight = animatedCurrentLevel_ * meterHeight;
    auto currentBarBounds = meterBounds
        .withY(meterBounds.getBottom() - (int)currentBarHeight)
        .withHeight((int)currentBarHeight);

    if (currentBarHeight > 0.5f) {
        // Green-to-yellow gradient based on level
        juce::Colour barColor = animatedCurrentLevel_ > 0.85f
            ? juce::Colour(0xffff9500)  // Orange if close to peak
            : juce::Colour(0xff34c759);  // Green for normal
        g.setColour(barColor);
        g.fillRect(currentBarBounds);
    }

    // Peak level indicator line
    float peakLineY = meterBounds.getBottom() - (animatedPeakLevel_ * meterHeight);
    g.setColour(juce::Colour(0xffff453a));  // Red
    g.drawHorizontalLine((int)peakLineY, (float)meterBounds.getX(), (float)meterBounds.getRight());

    // dB scale labels
    g.setFont(juce::FontOptions(8.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("-12", meterBounds.withHeight(12), juce::Justification::centred, true);
    g.drawText("0", meterBounds.withTop(meterBounds.getCentreY() - 6).withHeight(12),
               juce::Justification::centred, true);
}

void MasterOutputComponent::paintPeakIndicators(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Show headroom and peak values
    auto textBounds = bounds.reduced(4, 20);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));

    // Headroom indicator
    float headroom = 1.0f - animatedPeakLevel_;
    headroom = juce::jlimit(0.0f, 1.0f, headroom);

    juce::String headroomText = "HM: " + juce::String(gainToDb(headroom), 1) + " dB";
    g.setColour(headroom < 0.1f ? juce::Colour(0xffff453a) : juce::Colour(0xff34c759));
    g.drawText(headroomText, textBounds.removeFromTop(16), juce::Justification::centredLeft, true);

    // Peak value
    juce::String peakText = "P: " + juce::String(gainToDb(animatedPeakLevel_), 1) + " dB";
    g.setColour(animatedPeakLevel_ > 0.95f ? juce::Colour(0xffff453a) : juce::Colour(0xffcccccc));
    g.drawText(peakText, textBounds, juce::Justification::centredLeft, true);
}

//==============================================================================
void MasterOutputComponent::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds().removeFromLeft(FADER_WIDTH + SPACING).reduced(8, 20);

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
    const // float ATTACK = 0.85f;  // Unused variable   // Fast attack
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

