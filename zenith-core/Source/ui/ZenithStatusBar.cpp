/**
 * @file ZenithStatusBar.cpp
 * @brief Unified status bar component implementation
 * 
 * DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens correctly
 */

#include "ZenithStatusBar.h"
#include "ZenithLookAndFeel.h"
#include "../../include/Engine.h"

namespace zenith {

ZenithStatusBar::ZenithStatusBar(Engine& engine)
    : engine_(engine)
{
    // Start timer for stats updates
    startTimer(500); // 2Hz update rate
    updateStats();
}

ZenithStatusBar::~ZenithStatusBar()
{
    stopTimer();
}

void ZenithStatusBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // DESIGN SYSTEM: Background using backgroundBase elevation (not backgroundDark)
    g.fillAll(juce::Colour(ZenithLookAndFeel::Elevation::dp0));

    // DESIGN SYSTEM: Top border using borderSubtle
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    g.drawRect(bounds.removeFromTop(1), 1);

    // DESIGN SYSTEM: Padding using Spacing constants
    bounds.reduce(ZenithLookAndFeel::Spacing::s, 0);

    // Draw CPU Usage (Right side)
    {
        auto cpuBounds = bounds.removeFromRight(100);
        // DESIGN SYSTEM: Text using textSecondary
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
        g.setFont(ZenithLookAndFeel::Typography::getSmall());
        
        juce::String cpuText = "CPU: " + juce::String(cpuUsage_, 1) + "%";
        g.drawText(cpuText, cpuBounds, juce::Justification::centredRight, true);
    }

    // Draw Audio Device Info (Right side, left of CPU)
    {
        auto deviceBounds = bounds.removeFromRight(300);
        // DESIGN SYSTEM: Text using textSecondary
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
        g.setFont(ZenithLookAndFeel::Typography::getSmall());
        
        juce::String deviceText = audioDeviceName_ + " | " + audioSettings_;
        g.drawText(deviceText, deviceBounds, juce::Justification::centredRight, true);
    }

    // Draw Status Message (Left side)
    if (messageAlpha_ > 0.0f)
    {
        // DESIGN SYSTEM: Error color using danger, normal using textPrimary
        g.setColour(isErrorMessage_ 
            ? juce::Colour(ZenithLookAndFeel::Colors::danger).withAlpha(messageAlpha_)
            : juce::Colour(ZenithLookAndFeel::Colors::textPrimary).withAlpha(messageAlpha_));
        g.setFont(ZenithLookAndFeel::Typography::getSmall());
        g.drawText(currentMessage_, bounds, juce::Justification::centredLeft, true);
    }
}

void ZenithStatusBar::resized()
{
    // No child components to resize
}

void ZenithStatusBar::timerCallback()
{
    updateStats();
    
    // Handle message fade out
    if (messageTimeout_ > 0)
    {
        messageTimeout_ -= 500; // Decrement by timer interval
        if (messageTimeout_ <= 0)
        {
            // Start fade out
            startTimer(30); // Switch to faster timer for animation
        }
    }
    else if (messageAlpha_ > 0.0f)
    {
        // Fade out animation
        messageAlpha_ -= 0.05f;
        if (messageAlpha_ <= 0.0f)
        {
            messageAlpha_ = 0.0f;
            startTimer(500); // Revert to slow timer
        }
        repaint();
    }
    else
    {
        repaint(); // Just repaint stats
    }
}

void ZenithStatusBar::showMessage(const juce::String& message, bool isError, int durationMs)
{
    currentMessage_ = message;
    isErrorMessage_ = isError;
    messageTimeout_ = durationMs;
    messageAlpha_ = 1.0f;
    repaint();
}

void ZenithStatusBar::updateStats()
{
    cpuUsage_ = engine_.getCpuUsage();
    
    // Parse audio device info string from engine
    // Format is usually "DeviceName | 44100Hz | 512 spls"
    juce::String fullInfo = engine_.getAudioDeviceInfo();
    
    // Split into name and settings
    int firstSeparator = fullInfo.indexOf(" | ");
    if (firstSeparator > 0)
    {
        audioDeviceName_ = fullInfo.substring(0, firstSeparator);
        audioSettings_ = fullInfo.substring(firstSeparator + 3);
    }
    else
    {
        audioDeviceName_ = fullInfo;
        audioSettings_ = "";
    }
}

} // namespace zenith
