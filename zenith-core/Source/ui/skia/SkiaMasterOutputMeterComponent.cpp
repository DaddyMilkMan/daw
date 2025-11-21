/**
 * @file SkiaMasterOutputMeterComponent.cpp
 */

#include "SkiaMasterOutputMeterComponent.h"

#ifdef ZENITH_USE_SKIA

#include <cmath>

namespace zenith {

SkiaMasterOutputMeterComponent::SkiaMasterOutputMeterComponent()
{
    setSize(60, 300);
    startTimer(16);
}

SkiaMasterOutputMeterComponent::~SkiaMasterOutputMeterComponent()
{
    stopTimer();
}

void SkiaMasterOutputMeterComponent::setLevel(float leftLevel, float rightLevel)
{
    leftLevel_ = juce::jlimit(0.0f, 1.0f, leftLevel);
    rightLevel_ = juce::jlimit(0.0f, 1.0f, rightLevel);
}

void SkiaMasterOutputMeterComponent::setPeakLevel(float leftPeak, float rightPeak)
{
    leftPeak_ = juce::jlimit(0.0f, 1.0f, leftPeak);
    rightPeak_ = juce::jlimit(0.0f, 1.0f, rightPeak);
}

void SkiaMasterOutputMeterComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    const int meterWidth = 12;
    const int meterHeight = getHeight() - 20;
    const int leftMeterX = 10;
    const int rightMeterX = getWidth() - meterWidth - 10;
    const int meterY = 10;

    // Draw left meter background
    g.setColour(juce::Colours::black);
    g.fillRect(leftMeterX, meterY, meterWidth, meterHeight);

    // Draw left meter level
    int leftLevelHeight = static_cast<int>(meterHeight * leftSmoothed_);
    g.setColour(leftSmoothed_ > 0.8f ? juce::Colours::red :
                leftSmoothed_ > 0.6f  ? juce::Colours::yellow :
                                        juce::Colours::green);
    g.fillRect(leftMeterX, meterY + meterHeight - leftLevelHeight, meterWidth, leftLevelHeight);

    // Draw left peak indicator
    int leftPeakY = meterY + meterHeight - static_cast<int>(meterHeight * leftPeak_);
    g.setColour(juce::Colours::white);
    g.drawHorizontalLine(leftPeakY, leftMeterX - 2, leftMeterX + meterWidth + 2);

    // Draw right meter background
    g.setColour(juce::Colours::black);
    g.fillRect(rightMeterX, meterY, meterWidth, meterHeight);

    // Draw right meter level
    int rightLevelHeight = static_cast<int>(meterHeight * rightSmoothed_);
    g.setColour(rightSmoothed_ > 0.8f ? juce::Colours::red :
                rightSmoothed_ > 0.6f  ? juce::Colours::yellow :
                                         juce::Colours::green);
    g.fillRect(rightMeterX, meterY + meterHeight - rightLevelHeight, meterWidth, rightLevelHeight);

    // Draw right peak indicator
    int rightPeakY = meterY + meterHeight - static_cast<int>(meterHeight * rightPeak_);
    g.setColour(juce::Colours::white);
    g.drawHorizontalLine(rightPeakY, rightMeterX - 2, rightMeterX + meterWidth + 2);

    // Draw labels
    g.setColour(juce::Colours::white);
    g.setFont(10);
    g.drawText("L", leftMeterX, getHeight() - 15, meterWidth, 15,
               juce::Justification::centred);
    g.drawText("R", rightMeterX, getHeight() - 15, meterWidth, 15,
               juce::Justification::centred);
}

void SkiaMasterOutputMeterComponent::resized()
{
    // Layout
}

void SkiaMasterOutputMeterComponent::timerCallback()
{
    // Smooth level meters
    const float smoothingFactor = 0.2f;
    leftSmoothed_ += (leftLevel_ - leftSmoothed_) * smoothingFactor;
    rightSmoothed_ += (rightLevel_ - rightSmoothed_) * smoothingFactor;

    repaint();
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
