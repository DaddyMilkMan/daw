/**
 * @file ZenithTransportBar.cpp
 * @brief Unified transport bar implementation
 */

#include "ZenithTransportBar.h"
#include "ZenithLookAndFeel.h"
#include "../../include/Engine.h"

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

ZenithTransportBar::ZenithTransportBar(Engine& engine)
    : engine_(engine)
{
    setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(false);
    startTimer(60); // 60 Hz for smooth display updates
}

ZenithTransportBar::~ZenithTransportBar()
{
    stopTimer();
}

//==============================================================================
// Component Overrides
//==============================================================================

void ZenithTransportBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Background with subtle gradient
    juce::ColourGradient gradient(
        juce::Colour(ZenithLookAndFeel::Colors::backgroundMid),
        0.0f, 0.0f,
        juce::Colour(ZenithLookAndFeel::Colors::backgroundPanel),
        0.0f, (float)getHeight(),
        false
    );
    g.setGradientFill(gradient);
    g.fillAll();
    
    // Top border
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::border));
    g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 1.0f);
    
    // Bottom border (stronger)
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderLight));
    g.drawLine(0.0f, (float)getHeight(), (float)getWidth(), (float)getHeight(), 1.0f);
    
    // Left section: Project name
    auto leftSection = bounds.removeFromLeft(200).reduced(ZenithLookAndFeel::Metrics::spacingM, 0);
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.setFont(ZenithLookAndFeel::getFontHeading());
    g.drawText("Zenith DAW", leftSection, juce::Justification::centredLeft, true);
    
    // Right section: Status displays
    auto rightSection = bounds.removeFromRight(400).reduced(ZenithLookAndFeel::Metrics::spacingM, 0);
    
    // Track count (rightmost)
    auto trackCountArea = rightSection.removeFromRight(100);
    g.setFont(ZenithLookAndFeel::getFontSmall());
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    g.drawText(juce::String(trackCount_) + " tracks", trackCountArea, juce::Justification::centredRight, true);
    
    // CPU usage
    auto cpuArea = rightSection.removeFromRight(80);
    g.drawText(cpuDisplay_, cpuArea, juce::Justification::centredRight, true);
    
    // Tempo
    auto tempoArea = rightSection.removeFromRight(80);
    g.setFont(ZenithLookAndFeel::getFontBody());
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.drawText(tempoDisplay_, tempoArea, juce::Justification::centredRight, true);
    
    // Time display
    auto timeArea = rightSection.removeFromRight(120);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText(timeDisplay_, timeArea, juce::Justification::centredRight, true);
    
    // Center section: Transport controls
    // Draw play button
    bool isPlaying = engine_.isPlaying();
    drawTransportButton(g, getPlayButtonBounds(), 
                       u8"\u25B6", // Play triangle
                       juce::Colour(ZenithLookAndFeel::Colors::playGreen),
                       isPlaying,
                       playHovered_,
                       playPressed_);
    
    // Stop button
    drawTransportButton(g, getStopButtonBounds(),
                       u8"\u25A0", // Stop square
                       juce::Colour(ZenithLookAndFeel::Colors::stopGrey),
                       !isPlaying,
                       stopHovered_,
                       stopPressed_);
    
    // Record button
    bool isRecording = engine_.isRecording();
    drawTransportButton(g, getRecordButtonBounds(),
                       u8"\u25CF", // Record circle
                       juce::Colour(ZenithLookAndFeel::Colors::recordRed),
                       isRecording,
                       recordHovered_,
                       recordPressed_);
    
    // Loop button
    bool isLooping = engine_.isLooping();
    drawTransportButton(g, getLoopButtonBounds(),
                       u8"\u27F3", // Loop arrow
                       juce::Colour(ZenithLookAndFeel::Colors::accentSecondary),
                       isLooping,
                       loopHovered_,
                       loopPressed_);
}

void ZenithTransportBar::resized()
{
    // Layout is handled in paint() for simplicity
}

void ZenithTransportBar::timerCallback()
{
    // Update time display
    double playheadPosition = engine_.getPlaybackPositionBeats();
    timeDisplay_ = formatTime(playheadPosition * (60.0 / engine_.getTempoMap().getTempoAt(playheadPosition))); // Convert beats to seconds approx or use getPlayheadSamples / SampleRate
    // Better: Use getPlayheadSamples / SampleRate
    double currentSeconds = (double)engine_.getPlayheadSamples() / engine_.getSampleRate();
    timeDisplay_ = formatTime(currentSeconds);
    
    // Update tempo
    double tempo = engine_.getTempoMap().getTempoAt(engine_.getPlaybackPositionBeats());
    tempoDisplay_ = formatTempo(tempo);
    
    // Update CPU
    double cpuUsage = engine_.getCpuUsage();
    cpuDisplay_ = juce::String(cpuUsage, 1) + "%";
    
    // Update track count
    trackCount_ = engine_.getNumTracks();
    
    repaint();
}

//==============================================================================
// Mouse Handling
//==============================================================================

void ZenithTransportBar::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    
    playPressed_ = getPlayButtonBounds().contains(pos);
    stopPressed_ = getStopButtonBounds().contains(pos);
    recordPressed_ = getRecordButtonBounds().contains(pos);
    loopPressed_ = getLoopButtonBounds().contains(pos);
    
    repaint();
}

void ZenithTransportBar::mouseUp(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    
    // Play button
    if (playPressed_ && getPlayButtonBounds().contains(pos))
    {
        if (engine_.isPlaying())
            engine_.stop(); // Toggle behavior if desired, or just play
        else
            engine_.play();
        DBG("Transport: Play/Stop");
    }
    
    // Stop button
    if (stopPressed_ && getStopButtonBounds().contains(pos))
    {
        engine_.stop();
        engine_.setPlayheadSamples(0); // Return to zero on stop
        DBG("Transport: Stop");
    }
    
    // Record button
    if (recordPressed_ && getRecordButtonBounds().contains(pos))
    {
        engine_.toggleRecording();
        DBG("Transport: Record toggled");
    }
    
    // Loop button
    if (loopPressed_ && getLoopButtonBounds().contains(pos))
    {
        engine_.setLooping(!engine_.isLooping());
        DBG("Transport: Loop toggled");
    }
    
    playPressed_ = false;
    stopPressed_ = false;
    recordPressed_ = false;
    loopPressed_ = false;
    
    repaint();
}

void ZenithTransportBar::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    
    bool newPlayHovered = getPlayButtonBounds().contains(pos);
    bool newStopHovered = getStopButtonBounds().contains(pos);
    bool newRecordHovered = getRecordButtonBounds().contains(pos);
    bool newLoopHovered = getLoopButtonBounds().contains(pos);
    
    if (newPlayHovered != playHovered_ || newStopHovered != stopHovered_ ||
        newRecordHovered != recordHovered_ || newLoopHovered != loopHovered_)
    {
        playHovered_ = newPlayHovered;
        stopHovered_ = newStopHovered;
        recordHovered_ = newRecordHovered;
        loopHovered_ = newLoopHovered;
        repaint();
    }
}

void ZenithTransportBar::mouseEnter(const juce::MouseEvent& event)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ZenithTransportBar::mouseExit(const juce::MouseEvent& event)
{
    playHovered_ = false;
    stopHovered_ = false;
    recordHovered_ = false;
    loopHovered_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

//==============================================================================
// Drawing Helpers
//==============================================================================

void ZenithTransportBar::drawTransportButton(juce::Graphics& g,
                                            const juce::Rectangle<int>& bounds,
                                            const juce::String& symbol,
                                            const juce::Colour& color,
                                            bool isActive,
                                            bool isHovered,
                                            bool isPressed)
{
    auto buttonBounds = bounds.toFloat().reduced(2.0f);
    
    // Background circle
    juce::Colour bgColor;
    if (isPressed)
        bgColor = color.darker(0.3f);
    else if (isActive)
        bgColor = color;
    else if (isHovered)
        bgColor = juce::Colour(ZenithLookAndFeel::Colors::backgroundLight);
    else
        bgColor = juce::Colour(ZenithLookAndFeel::Colors::backgroundPanel);
    
    g.setColour(bgColor);
    g.fillEllipse(buttonBounds);
    
    // Border
    if (!isActive)
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::border));
        g.drawEllipse(buttonBounds, 1.0f);
    }
    
    // Symbol
    juce::Colour symbolColor = isActive 
        ? juce::Colour(0xff000000) // Black on active background
        : color.brighter(0.3f);
    
    g.setColour(symbolColor);
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText(symbol, buttonBounds.toNearestInt(), juce::Justification::centred, true);
}

//==============================================================================
// Button Bounds
//==============================================================================

juce::Rectangle<int> ZenithTransportBar::getPlayButtonBounds() const
{
    int buttonSize = 40;
    int spacing = 8;
    int totalWidth = buttonSize * 4 + spacing * 3;
    int startX = (getWidth() - totalWidth) / 2;
    int y = (getHeight() - buttonSize) / 2;
    
    return juce::Rectangle<int>(startX, y, buttonSize, buttonSize);
}

juce::Rectangle<int> ZenithTransportBar::getStopButtonBounds() const
{
    int buttonSize = 40;
    int spacing = 8;
    int totalWidth = buttonSize * 4 + spacing * 3;
    int startX = (getWidth() - totalWidth) / 2;
    int y = (getHeight() - buttonSize) / 2;
    
    return juce::Rectangle<int>(startX + buttonSize + spacing, y, buttonSize, buttonSize);
}

juce::Rectangle<int> ZenithTransportBar::getRecordButtonBounds() const
{
    int buttonSize = 40;
    int spacing = 8;
    int totalWidth = buttonSize * 4 + spacing * 3;
    int startX = (getWidth() - totalWidth) / 2;
    int y = (getHeight() - buttonSize) / 2;
    
    return juce::Rectangle<int>(startX + (buttonSize + spacing) * 2, y, buttonSize, buttonSize);
}

juce::Rectangle<int> ZenithTransportBar::getLoopButtonBounds() const
{
    int buttonSize = 40;
    int spacing = 8;
    int totalWidth = buttonSize * 4 + spacing * 3;
    int startX = (getWidth() - totalWidth) / 2;
    int y = (getHeight() - buttonSize) / 2;
    
    return juce::Rectangle<int>(startX + (buttonSize + spacing) * 3, y, buttonSize, buttonSize);
}

//==============================================================================
// Display Formatting
//==============================================================================

juce::String ZenithTransportBar::formatTime(double seconds)
{
    int totalSeconds = (int)seconds;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;
    int ms = (int)((seconds - totalSeconds) * 100);
    
    if (hours > 0)
        return juce::String::formatted("%02d:%02d:%02d.%02d", hours, minutes, secs, ms);
    else
        return juce::String::formatted("%02d:%02d.%02d", minutes, secs, ms);
}

juce::String ZenithTransportBar::formatTempo(double bpm)
{
    return juce::String(bpm, 1) + " BPM";
}

} // namespace zenith

