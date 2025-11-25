/**
 * @file ZenithTransportBar.cpp
 * @brief Unified transport bar implementation
 * 
 * DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens correctly
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
    
    // DESIGN SYSTEM: Background with subtle gradient using elevation tokens
    juce::ColourGradient gradient(
        juce::Colour(ZenithLookAndFeel::Elevation::dp1),
        0.0f, 0.0f,
        juce::Colour(ZenithLookAndFeel::Elevation::dp2),
        0.0f, (float)getHeight(),
        false
    );
    g.setGradientFill(gradient);
    g.fillAll();
    
    // DESIGN SYSTEM: Top border using borderSubtle
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 1.0f);
    
    // DESIGN SYSTEM: Bottom border using borderMedium (stronger)
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderMedium));
    g.drawLine(0.0f, (float)getHeight(), (float)getWidth(), (float)getHeight(), 1.0f);
    
    // DESIGN SYSTEM: Left section: Project name using textPrimary
    auto leftSection = bounds.removeFromLeft(200).reduced(ZenithLookAndFeel::Spacing::m, 0);
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.setFont(ZenithLookAndFeel::Typography::getH4());
    g.drawText("Zenith DAW", leftSection, juce::Justification::centredLeft, true);
    
    // Right section: Status displays
    auto rightSection = bounds.removeFromRight(400).reduced(ZenithLookAndFeel::Spacing::m, 0);
    
    // DESIGN SYSTEM: Track count using textSecondary (rightmost)
    auto trackCountArea = rightSection.removeFromRight(100);
    g.setFont(ZenithLookAndFeel::Typography::getSmall());
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    g.drawText(juce::String(trackCount_) + " tracks", trackCountArea, juce::Justification::centredRight, true);
    
    // CPU usage
    auto cpuArea = rightSection.removeFromRight(80);
    g.drawText(cpuDisplay_, cpuArea, juce::Justification::centredRight, true);
    
    // DESIGN SYSTEM: Tempo using textPrimary
    auto tempoArea = rightSection.removeFromRight(80);
    g.setFont(ZenithLookAndFeel::Typography::getBody());
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.drawText(tempoDisplay_, tempoArea, juce::Justification::centredRight, true);
    
    // Time display
    auto timeArea = rightSection.removeFromRight(120);
    g.setFont(ZenithLookAndFeel::Typography::getH4());
    g.drawText(timeDisplay_, timeArea, juce::Justification::centredRight, true);
    
    // Center section: Transport controls
    // DESIGN SYSTEM: Draw play button with playGreen semantic color
    bool isPlaying = engine_.isPlaying();
    drawTransportButton(g, getPlayButtonBounds(), 
                       juce::CharPointer_UTF8("\xe2\x96\xb6"), // Play triangle
                       juce::Colour(ZenithLookAndFeel::Colors::playGreen),
                       isPlaying,
                       playHovered_,
                       playPressed_);
    
    // DESIGN SYSTEM: Stop button with stopGrey color
    drawTransportButton(g, getStopButtonBounds(),
                       juce::CharPointer_UTF8("\xe2\x96\xa0"), // Stop square
                       juce::Colour(ZenithLookAndFeel::Colors::stopGrey),
                       !isPlaying,
                       stopHovered_,
                       stopPressed_);
    
    // DESIGN SYSTEM: Record button with recordRed color
    bool isRecording = engine_.isRecording();
    drawTransportButton(g, getRecordButtonBounds(),
                       juce::CharPointer_UTF8("\xe2\x97\x8f"), // Record circle
                       juce::Colour(ZenithLookAndFeel::Colors::recordRed),
                       isRecording,
                       recordHovered_,
                       recordPressed_);
    
    // DESIGN SYSTEM: Loop button with accentSecondary color
    bool isLooping = engine_.isLooping();
    drawTransportButton(g, getLoopButtonBounds(),
                       juce::CharPointer_UTF8("\xe2\x9f\xb3"), // Loop arrow
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
            engine_.stop();
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
    juce::ignoreUnused(event);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ZenithTransportBar::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
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
    
    // DESIGN SYSTEM: Background circle using elevation tokens
    juce::Colour bgColor;
    if (isPressed)
        bgColor = color.darker(0.3f);
    else if (isActive)
        bgColor = color;
    else if (isHovered)
        bgColor = juce::Colour(ZenithLookAndFeel::Elevation::dp8);  // Hover uses elevated surface
    else
        bgColor = juce::Colour(ZenithLookAndFeel::Elevation::dp2);  // Default uses panel surface
    
    g.setColour(bgColor);
    g.fillEllipse(buttonBounds);
    
    // DESIGN SYSTEM: Border using borderSubtle when not active
    if (!isActive)
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
        g.drawEllipse(buttonBounds, 1.0f);
    }
    
    // DESIGN SYSTEM: Symbol color - textOnAccent when active, color.brighter when not
    juce::Colour symbolColor = isActive 
        ? juce::Colour(ZenithLookAndFeel::Colors::textOnAccent) // Black on active background
        : color.brighter(0.3f);
    
    g.setColour(symbolColor);
    g.setFont(ZenithLookAndFeel::Typography::getH2());
    g.drawText(symbol, buttonBounds.toNearestInt(), juce::Justification::centred, true);
}

//==============================================================================
// Button Bounds
//==============================================================================

juce::Rectangle<int> ZenithTransportBar::getPlayButtonBounds() const
{
    int buttonSize = 40;
    int spacing = ZenithLookAndFeel::Spacing::s;
    int totalWidth = buttonSize * 4 + spacing * 3;
    int startX = (getWidth() - totalWidth) / 2;
    int y = (getHeight() - buttonSize) / 2;
    
    return juce::Rectangle<int>(startX, y, buttonSize, buttonSize);
}

juce::Rectangle<int> ZenithTransportBar::getStopButtonBounds() const
{
    int buttonSize = 40;
    int spacing = ZenithLookAndFeel::Spacing::s;
    int totalWidth = buttonSize * 4 + spacing * 3;
    int startX = (getWidth() - totalWidth) / 2;
    int y = (getHeight() - buttonSize) / 2;
    
    return juce::Rectangle<int>(startX + buttonSize + spacing, y, buttonSize, buttonSize);
}

juce::Rectangle<int> ZenithTransportBar::getRecordButtonBounds() const
{
    int buttonSize = 40;
    int spacing = ZenithLookAndFeel::Spacing::s;
    int totalWidth = buttonSize * 4 + spacing * 3;
    int startX = (getWidth() - totalWidth) / 2;
    int y = (getHeight() - buttonSize) / 2;
    
    return juce::Rectangle<int>(startX + (buttonSize + spacing) * 2, y, buttonSize, buttonSize);
}

juce::Rectangle<int> ZenithTransportBar::getLoopButtonBounds() const
{
    int buttonSize = 40;
    int spacing = ZenithLookAndFeel::Spacing::s;
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
