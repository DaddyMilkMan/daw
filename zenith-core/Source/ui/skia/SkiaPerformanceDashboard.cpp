/**
 * @file SkiaPerformanceDashboard.cpp
 * @brief Implementation of Skia performance dashboard
 */

#include "SkiaPerformanceDashboard.h"

#ifdef ZENITH_USE_SKIA

#include <algorithm>
#include <numeric>

namespace zenith {

SkiaPerformanceDashboard::SkiaPerformanceDashboard()
    : memoryUsage_(0.0f), componentCount_(0), averageFrameTime_(16.67f),
      peakFrameTime_(16.67f), currentFPS_(60.0f), smoothedFrameTime_(16.67f)
{
    setSize(400, 300);
    startTimer(1000); // Update every second
}

SkiaPerformanceDashboard::~SkiaPerformanceDashboard()
{
    stopTimer();
}

void SkiaPerformanceDashboard::recordFrameTime(float milliseconds)
{
    frameTimeHistory_.push_back(milliseconds);
    if (frameTimeHistory_.size() > MAX_HISTORY)
    {
        frameTimeHistory_.pop_front();
    }

    // Update smoothed frame time
    const float smoothingFactor = 0.2f;
    smoothedFrameTime_ += (milliseconds - smoothedFrameTime_) * smoothingFactor;

    // Calculate average
    if (!frameTimeHistory_.empty())
    {
        float sum = std::accumulate(frameTimeHistory_.begin(),
                                    frameTimeHistory_.end(), 0.0f);
        averageFrameTime_ = sum / frameTimeHistory_.size();

        // Calculate peak
        peakFrameTime_ = *std::max_element(frameTimeHistory_.begin(),
                                           frameTimeHistory_.end());

        // Calculate current FPS
        currentFPS_ = 1000.0f / smoothedFrameTime_;
    }

    repaint();
}

void SkiaPerformanceDashboard::setMemoryUsage(float megabytes)
{
    memoryUsage_ = megabytes;
    repaint();
}

void SkiaPerformanceDashboard::setComponentCount(int count)
{
    componentCount_ = count;
    repaint();
}

float SkiaPerformanceDashboard::getAverageFrameTime() const
{
    return averageFrameTime_;
}

float SkiaPerformanceDashboard::getPeakFrameTime() const
{
    return peakFrameTime_;
}

float SkiaPerformanceDashboard::getCurrentFPS() const
{
    return currentFPS_;
}

void SkiaPerformanceDashboard::paint(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Background
    g.fillAll(juce::Colour(
        colors.backgroundSecondary >> 16 & 0xFF,
        colors.backgroundSecondary >> 8 & 0xFF,
        colors.backgroundSecondary & 0xFF
    ));

    // Border
    g.setColour(juce::Colour(
        colors.border >> 16 & 0xFF,
        colors.border >> 8 & 0xFF,
        colors.border & 0xFF
    ));
    g.drawRect(getLocalBounds(), 2);

    // Render sections
    renderMetrics(g);
    renderFrameTimeGraph(g);
    renderMemoryUsage(g);
}

void SkiaPerformanceDashboard::resized()
{
    // Layout sections
}

void SkiaPerformanceDashboard::timerCallback()
{
    // Periodic update (already repainting on frame time changes)
}

void SkiaPerformanceDashboard::renderMetrics(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    g.setColour(juce::Colour(
        colors.textPrimary >> 16 & 0xFF,
        colors.textPrimary >> 8 & 0xFF,
        colors.textPrimary & 0xFF
    ));

    g.setFont(14.0f);

    // Title
    g.drawText("Performance Dashboard", 10, 10, 200, 20,
               juce::Justification::left);

    // FPS display
    g.setFont(12.0f);
    juce::String fpsText = juce::String(currentFPS_, 1) + " FPS";
    g.drawText(fpsText, 10, 40, 100, 20, juce::Justification::left);

    // Frame time display
    juce::String frameTimeText =
        "Frame: " + juce::String(averageFrameTime_, 2) + "ms";
    g.drawText(frameTimeText, 10, 65, 150, 20, juce::Justification::left);

    // Peak frame time
    juce::String peakText = "Peak: " + juce::String(peakFrameTime_, 2) + "ms";
    g.drawText(peakText, 10, 90, 150, 20, juce::Justification::left);

    // Component count
    juce::String componentText = "Components: " + juce::String(componentCount_);
    g.drawText(componentText, 10, 115, 150, 20, juce::Justification::left);

    // Memory usage
    juce::String memoryText = "Memory: " + juce::String(memoryUsage_, 1) + " MB";
    g.drawText(memoryText, 10, 140, 150, 20, juce::Justification::left);
}

void SkiaPerformanceDashboard::renderFrameTimeGraph(juce::Graphics& g)
{
    if (frameTimeHistory_.empty())
        return;

    const int graphX = 220;
    const int graphY = 40;
    const int graphWidth = 170;
    const int graphHeight = 80;

    // Draw graph background
    const auto& colors = SkiaTheme::getInstance().getColors();
    g.setColour(juce::Colour(
        colors.backgroundTertiary >> 16 & 0xFF,
        colors.backgroundTertiary >> 8 & 0xFF,
        colors.backgroundTertiary & 0xFF
    ));
    g.fillRect(graphX, graphY, graphWidth, graphHeight);

    // Draw graph border
    g.setColour(juce::Colour(
        colors.border >> 16 & 0xFF,
        colors.border >> 8 & 0xFF,
        colors.border & 0xFF
    ));
    g.drawRect(graphX, graphY, graphWidth, graphHeight);

    // Draw baseline (16.67ms = 60 FPS)
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    int baselineY = graphY + graphHeight - static_cast<int>(16.67f);
    g.drawHorizontalLine(baselineY, graphX, graphX + graphWidth);

    // Draw frame time points
    g.setColour(juce::Colours::cyan);
    float maxFrameTime = std::max(33.0f, peakFrameTime_);
    int pointSpacing = graphWidth / MAX_HISTORY;

    for (int i = 0; i < static_cast<int>(frameTimeHistory_.size()); i++)
    {
        float normalizedTime = frameTimeHistory_[i] / maxFrameTime;
        int pointX = graphX + i * pointSpacing;
        int pointY = graphY + graphHeight - static_cast<int>(normalizedTime * graphHeight);

        g.fillEllipse(pointX - 2, pointY - 2, 4, 4);
    }
}

void SkiaPerformanceDashboard::renderMemoryUsage(juce::Graphics& g)
{
    const int meterX = 10;
    const int meterY = 165;
    const int meterWidth = 380;
    const int meterHeight = 20;

    const auto& colors = SkiaTheme::getInstance().getColors();

    // Background
    g.setColour(juce::Colours::black);
    g.fillRect(meterX, meterY, meterWidth, meterHeight);

    // Memory level (assuming max 2GB)
    float normalizedMemory = memoryUsage_ / 2000.0f;
    int filledWidth = static_cast<int>(meterWidth * normalizedMemory);

    g.setColour(memoryUsage_ > 1500.0f ? juce::Colours::red :
                memoryUsage_ > 1000.0f  ? juce::Colours::yellow :
                                          juce::Colours::green);
    g.fillRect(meterX, meterY, filledWidth, meterHeight);

    // Border
    g.setColour(juce::Colours::white);
    g.drawRect(meterX, meterY, meterWidth, meterHeight);

    // Label
    g.setFont(10.0f);
    g.drawText("Memory", meterX + 5, meterY + 3, 100, 14,
               juce::Justification::left);
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
