/**
 * @file SkiaTransportControlComponent.cpp
 * @brief Implementation of Logic Pro style transport control
 */

#include "SkiaTransportControlComponent.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA

#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>

namespace zenith {

// Helper to create ARGB color
static constexpr SkColor ARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

SkiaTransportControlComponent::SkiaTransportControlComponent()
    : isPlaying_(false), isRecording_(false), isLooping_(false), tempo_(120.0f), timelinePosition_(0.0)
{
    setSize(800, 60);  // Logic Pro transport bar height
}

void SkiaTransportControlComponent::setIsPlaying(bool playing)
{
    if (isPlaying_ != playing)
    {
        isPlaying_ = playing;
        repaint();
    }
}

void SkiaTransportControlComponent::setIsRecording(bool recording)
{
    if (isRecording_ != recording)
    {
        isRecording_ = recording;
        repaint();
    }
}

void SkiaTransportControlComponent::setTempo(float bpm)
{
    if (tempo_ != bpm)
    {
        tempo_ = bpm;
        repaint();
    }
}

void SkiaTransportControlComponent::setTimelinePosition(double seconds)
{
    if (timelinePosition_ != seconds)
    {
        timelinePosition_ = seconds;
        repaint();
    }
}

void SkiaTransportControlComponent::paint(juce::Graphics& g)
{
    auto& theme = SkiaTheme::getInstance();
    auto colors = theme.getColors();
    
    // Fill with Logic Pro transport background (#1C1C1C)
    g.fillAll(juce::Colour(0xff1C1C1C));
    
    auto bounds = getLocalBounds();
    
    // === LCD Display Area (Center) ===
    int lcdWidth = 300;
    int lcdHeight = 40;
    int lcdX = (bounds.getWidth() - lcdWidth) / 2;
    int lcdY = (bounds.getHeight() - lcdHeight) / 2;
    
    // LCD Bezel (dark grey surround)
    juce::Rectangle<int> lcdBezel(lcdX - 6, lcdY - 6, lcdWidth + 12, lcdHeight + 12);
    g.setColour(juce::Colour(0xff333333));
    g.fillRoundedRectangle(lcdBezel.toFloat(), 4.0f);
    
    // LCD Background (pure black)
    juce::Rectangle<int> lcdBg(lcdX, lcdY, lcdWidth, lcdHeight);
    g.setColour(juce::Colour(0xff000000));
    g.fillRoundedRectangle(lcdBg.toFloat(), 2.0f);
    
    // LCD Text - Tempo (cyan, large)
    g.setColour(juce::Colour(0xffAADDFF));  // Light blue/cyan for LCD
    juce::Font tempoFont(24.0f, juce::Font::bold);
    g.setFont(tempoFont);
    juce::String tempoText = juce::String(tempo_, 3);  // "120.000"
    g.drawText(tempoText, lcdX + 10, lcdY + 4, 120, 32, juce::Justification::centredLeft);
    
    // LCD Text - Position (Bar.Beat.Tick format)
    int bar = 1 + (int)(timelinePosition_ / 4.0);  // Assume 4/4
    int beat = 1 + (int)fmod(timelinePosition_, 4.0);
    int tick = (int)((fmod(timelinePosition_, 1.0)) * 960.0);  // 960 ticks per beat
    
    g.setColour(juce::Colour(0xffAADDFF));
    juce::Font posFont(18.0f, juce::Font::bold);
    g.setFont(posFont);
    juce::String posText = juce::String(bar) + "." + juce::String(beat) + "." + juce::String(tick).paddedLeft('0', 3);
    g.drawText(posText, lcdX + 150, lcdY + 4, 140, 32, juce::Justification::centredLeft);
    
    // === Transport Buttons (Left of LCD) ===
    int buttonSize = 36;
    int buttonY = (bounds.getHeight() - buttonSize) / 2;
    int buttonSpacing = 44;
    int startX = lcdX - (buttonSpacing * 3) - 20;
    
    // Play Button (green when active)
    juce::Rectangle<float> playBounds(startX, buttonY, buttonSize, buttonSize);
    if (isPlaying_)
    {
        g.setColour(juce::Colour(0xff00FF00));  // Bright green
        g.fillRoundedRectangle(playBounds, 6.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff3E3E3E));
        g.fillRoundedRectangle(playBounds, 6.0f);
    }
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(playBounds, 6.0f, 1.0f);
    
    // Play triangle icon
    juce::Path playIcon;
    playIcon.addTriangle(playBounds.getCentreX() - 6, playBounds.getCentreY() - 8,
                        playBounds.getCentreX() - 6, playBounds.getCentreY() + 8,
                        playBounds.getCentreX() + 8, playBounds.getCentreY());
    g.setColour(isPlaying_ ? juce::Colour(0xff000000) : juce::Colour(0xffDFDFDF));
    g.fillPath(playIcon);
    
    // Stop Button
    juce::Rectangle<float> stopBounds(startX + buttonSpacing, buttonY, buttonSize, buttonSize);
    g.setColour(juce::Colour(0xff3E3E3E));
    g.fillRoundedRectangle(stopBounds, 6.0f);
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(stopBounds, 6.0f, 1.0f);
    
    // Stop square icon
    juce::Rectangle<float> stopIcon(stopBounds.getCentreX() - 7, stopBounds.getCentreY() - 7, 14, 14);
    g.setColour(juce::Colour(0xffDFDFDF));
    g.fillRect(stopIcon);
    
    // Record Button (red when active)
    juce::Rectangle<float> recBounds(startX + buttonSpacing * 2, buttonY, buttonSize, buttonSize);
    if (isRecording_)
    {
        g.setColour(juce::Colour(0xffFF0000));  // Pure red
        g.fillRoundedRectangle(recBounds, 6.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff3E3E3E));
        g.fillRoundedRectangle(recBounds, 6.0f);
    }
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(recBounds, 6.0f, 1.0f);
    
    // Record circle icon
    g.setColour(isRecording_ ? juce::Colour(0xff000000) : juce::Colour(0xffDFDFDF));
    g.fillEllipse(recBounds.getCentreX() - 8, recBounds.getCentreY() - 8, 16, 16);
    
    // === Cycle/Loop Button (Right of LCD) ===
    juce::Rectangle<float> cycleBounds(lcdX + lcdWidth + 20, buttonY, buttonSize, buttonSize);
    if (isLooping_)
    {
        g.setColour(juce::Colour(0xff00FF00));  // Green when active
        g.fillRoundedRectangle(cycleBounds, 6.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff3E3E3E));
        g.fillRoundedRectangle(cycleBounds, 6.0f);
    }
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(cycleBounds, 6.0f, 1.0f);
    
    // Loop icon (circular arrows)
    g.setColour(isLooping_ ? juce::Colour(0xff000000) : juce::Colour(0xffDFDFDF));
    g.setFont(juce::Font(16.0f));
    g.drawText("⟲", cycleBounds.toNearestInt(), juce::Justification::centred);
    
    // === CPU Meter (far right) ===
    int cpuMeterX = bounds.getWidth() - 80;
    int cpuMeterY = buttonY;
    
    // Simulate CPU usage (would be real in production)
    float cpuUsage = 0.45f;  // 45% CPU
    
    // CPU label
    g.setColour(juce::Colour(0xff9A9A9A));
    g.setFont(juce::Font(9.0f));
    g.drawText("CPU", cpuMeterX, cpuMeterY - 12, 40, 10, juce::Justification::centred);
    
    // CPU meter bar
    juce::Rectangle<float> cpuBounds(cpuMeterX, cpuMeterY, 40, 8);
    g.setColour(juce::Colour(0xff1A1A1A));
    g.fillRoundedRectangle(cpuBounds, 2.0f);
    
    // CPU fill (green <80%, orange >=80%)
    float cpuFillWidth = cpuBounds.getWidth() * cpuUsage;
    juce::Rectangle<float> cpuFill(cpuBounds.getX(), cpuBounds.getY(), cpuFillWidth, cpuBounds.getHeight());
    
    if (cpuUsage < 0.8f)
        g.setColour(juce::Colour(0xff00FF00));  // Green
    else
        g.setColour(juce::Colour(0xffFF9900));  // Orange warning
    
    g.fillRoundedRectangle(cpuFill, 2.0f);
    
    // CPU percentage text
    g.setColour(juce::Colour(0xffDFDFDF));
    g.setFont(juce::Font(10.0f));
    g.drawText(juce::String((int)(cpuUsage * 100)) + "%", cpuMeterX, cpuMeterY + 10, 40, 12, 
              juce::Justification::centred);
    
    // === MIDI Activity Indicator (left side) ===
    int midiX = 20;
    int midiY = buttonY;
    
    g.setColour(juce::Colour(0xff9A9A9A));
    g.setFont(juce::Font(9.0f));
    g.drawText("MIDI", midiX, midiY - 12, 30, 10, juce::Justification::centred);
    
    // MIDI in/out dots (green when active)
    bool midiInActive = isPlaying_;  // Simulate activity
    bool midiOutActive = isRecordEnabled_;
    
    g.setColour(midiInActive ? juce::Colour(0xff00FF00) : juce::Colour(0xff333333));
    g.fillEllipse(midiX + 5, midiY + 2, 6, 6);  // IN dot
    
    g.setColour(midiOutActive ? juce::Colour(0xff00FF00) : juce::Colour(0xff333333));
    g.fillEllipse(midiX + 15, midiY + 2, 6, 6);  // OUT dot
    
    // === Master Volume Slider (right of cycle) ===
    int masterVolX = lcdX + lcdWidth + 70;
    int masterVolY = buttonY + 4;
    int masterVolWidth = 60;
    
    g.setColour(juce::Colour(0xff9A9A9A));
    g.setFont(juce::Font(9.0f));
    g.drawText("MASTER", masterVolX, masterVolY - 16, masterVolWidth, 10, juce::Justification::centred);
    
    // Master volume slider (horizontal mini)
    juce::Rectangle<float> masterVolBounds(masterVolX, masterVolY, masterVolWidth, 12);
    g.setColour(juce::Colour(0xff1A1A1A));
    g.fillRoundedRectangle(masterVolBounds, 2.0f);
    
    // Fill (assume 0dB = 100%)
    float masterVol = 0.85f;  // 85% = -1.5dB
    float masterFillWidth = masterVolBounds.getWidth() * masterVol;
    juce::Rectangle<float> masterFill(masterVolBounds.getX(), masterVolBounds.getY(), 
                                      masterFillWidth, masterVolBounds.getHeight());
    
    g.setColour(juce::Colour(0xff006FFF));  // Logic Blue
    g.fillRoundedRectangle(masterFill, 2.0f);
    
    // === Subtle Noise Texture Overlay (1% opacity for professional feel) ===
    // Create subtle noise pattern for that professional "not too perfect" look
    juce::Random random(12345);  // Fixed seed for consistent pattern
    for (int i = 0; i < 500; ++i)  // Sparse noise
    {
        int x = random.nextInt(bounds.getWidth());
        int y = random.nextInt(bounds.getHeight());
        int brightness = random.nextInt(40) - 20;  // -20 to +20
        
        juce::Colour noiseColor = juce::Colour(0xff1C1C1C).brighter(brightness * 0.001f);
        g.setColour(noiseColor.withAlpha(0.01f));  // 1% opacity
        g.fillRect(x, y, 1, 1);
    }
}

void SkiaTransportControlComponent::resized()
{
    // Layout is handled in paint based on dynamic bounds
}

void SkiaTransportControlComponent::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds();
    int lcdWidth = 300;
    int lcdX = (bounds.getWidth() - lcdWidth) / 2;
    int buttonSize = 36;
    int buttonY = (bounds.getHeight() - buttonSize) / 2;
    int buttonSpacing = 44;
    int startX = lcdX - (buttonSpacing * 3) - 20;
    
    // Check play button
    juce::Rectangle<int> playBounds(startX, buttonY, buttonSize, buttonSize);
    if (playBounds.contains(event.getPosition()))
    {
        setIsPlaying(!isPlaying_);
        return;
    }
    
    // Check stop button
    juce::Rectangle<int> stopBounds(startX + buttonSpacing, buttonY, buttonSize, buttonSize);
    if (stopBounds.contains(event.getPosition()))
    {
        setIsPlaying(false);
        setIsRecording(false);
        return;
    }
    
    // Check record button
    juce::Rectangle<int> recBounds(startX + buttonSpacing * 2, buttonY, buttonSize, buttonSize);
    if (recBounds.contains(event.getPosition()))
    {
        setIsRecording(!isRecording_);
        if (isRecording_)
            setIsPlaying(true);  // Auto-start playback when recording
        return;
    }
    
    // Check cycle button
    juce::Rectangle<int> cycleBounds(lcdX + lcdWidth + 20, buttonY, buttonSize, buttonSize);
    if (cycleBounds.contains(event.getPosition()))
    {
        isLooping_ = !isLooping_;
        repaint();
        return;
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
