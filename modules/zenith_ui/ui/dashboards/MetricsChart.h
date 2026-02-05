/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    MetricsChart.h
    Chart component for metrics visualization - Neon Noir Edition
  ==============================================================================
*/


#include <vector>
#include <map>
#include <string>
#include <utility>
#include <cmath>
#include <limits>

namespace zenith {
namespace ui {

// Chart component for metrics visualization - Neon Noir Edition
class MetricsChart : public juce::Component,
                     public juce::Timer {
public:
    enum class ChartType {
        Line,
        Bar,
        Scatter,
        Area  // New: filled area chart with gradient
    };
    
    // --- Right Path Refinements ---
    struct AnimatedRange {
        float current = 0.0f;
        float target = 0.0f;
        float velocity = 0.0f;
        void setTarget(float value, bool immediate = false) {
            target = value;
            if (immediate) current = value;
        }
        bool update(float spring = 0.18f, float damp = 0.75f) {
            float force = (target - current) * spring;
            velocity = (velocity + force) * damp;
            current += velocity;
            return std::abs(target - current) > 0.0001f || std::abs(velocity) > 0.0001f;
        }
    };

    // Data series for multi-series support
    struct DataSeries {
        juce::String name;
        std::vector<std::pair<float, float>> points;
        std::vector<std::pair<float, float>> animatedPoints;  // For smooth transitions
        juce::Colour color;
        float lineWidth = 2.5f;
        bool showPoints = true;
        bool showArea = false;
        float animationProgress = 1.0f;
    };
    
    MetricsChart(ChartType type = ChartType::Line);
    ~MetricsChart() override;
    
    // Data management
    void addDataPoint(float x, float y);
    void setDataPoints(const std::vector<std::pair<float, float>>& points);
    void clearData();
    
    // Multi-series support
    void addSeries(const juce::String& name, const juce::Colour& color);
    void addDataPointToSeries(const juce::String& name, float x, float y);
    void setSeriesData(const juce::String& name, const std::vector<std::pair<float, float>>& points);
    void clearSeries(const juce::String& name);
    void clearAllSeries();
    int getSeriesCount() const { return static_cast<int>(series_.size()); }
    int getSeriesPointCount(const juce::String& name) const {
        auto it = series_.find(name);
        return (it != series_.end()) ? static_cast<int>(it->second.points.size()) : -1;
    }
    
    // Appearance
    void setChartType(ChartType type);
    void setLineColor(const juce::Colour& color);
    void setBackgroundColor(const juce::Colour& color);
    void setShowGrid(bool show);
    void setShowLabels(bool show);
    void setShowGlow(bool show);
    void setShowGradientFill(bool show);
    void setAnimated(bool animated);
    void setShowLegend(bool show);
    
    // Axes
    void setXRange(float min, float max);
    void setYRange(float min, float max);
    void setXLabel(const juce::String& label);
    void setYLabel(const juce::String& label);
    void setAutoScale(bool autoScale);
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    // Mouse events for tooltip
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
private:
    ChartType chartType;
    std::vector<std::pair<float, float>> dataPoints;      // Primary series
    std::vector<std::pair<float, float>> animatedPoints;  // Interpolated for animation
    std::vector<std::pair<float, float>> previousPoints;  // Previous state for lerp
    std::map<juce::String, DataSeries> series_;           // Named multi-series
    
    // Neon Noir Color Palette
    juce::Colour lineColor = juce::Colour(0xFF00F0FF);        // CYAN - Primary
    juce::Colour backgroundColor = juce::Colour(0xFF121212);  // BG_01
    juce::Colour gridColor = juce::Colour(0x15FFFFFF);        // Subtle grid
    juce::Colour axisColor = juce::Colour(0xFF3A3A3F);        // Axis lines
    juce::Colour labelColor = juce::Colour(0xFFA1A1AA);       // TEXT_SECONDARY
    juce::Colour glowColor = juce::Colour(0xFF00F0FF);        // CYAN glow
    juce::Colour gradientTopColor = juce::Colour(0x4000F0FF); // Gradient fill top
    juce::Colour gradientBottomColor = juce::Colour(0x0000F0FF); // Gradient fill bottom
    
    // Modern color palette for multi-series
    std::vector<juce::Colour> seriesColors = {
        juce::Colour(0xFF00F0FF),  // Cyan
        juce::Colour(0xFFFF00D4),  // Magenta
        juce::Colour(0xFF10B981),  // Green
        juce::Colour(0xFFFF8800),  // Orange
        juce::Colour(0xFF7000FF),  // Violet
        juce::Colour(0xFFEC4899)   // Pink
    };
    
    bool showGrid = true;
    bool showLabels = true;
    bool showGlow_ = true;
    bool showGradientFill_ = true;
    bool animated_ = true;
    bool autoScale_ = true;
    bool showLegend_ = true;  // Legend visibility
    
    float xMin = 0.0f, xMax = 100.0f;
    float yMin = 0.0f, yMax = 1.0f;
    juce::String xLabel, yLabel;
    
    // Animation state
    float animationProgress_ = 1.0f;
    static constexpr float animationSpeed_ = 0.12f;  // Per frame interpolation
    static constexpr int animationFPS_ = 60;
    
    // Chart margins for proper label placement
    int marginLeft_ = 55;
    int marginRight_ = 25;
    int marginTop_ = 20;
    int marginBottom_ = 45;

    // --- Right Path State ---
    AnimatedRange xMinAnim, xMaxAnim, yMinAnim, yMaxAnim;
    float tooltipAlphaAnim = 0.0f;
    juce::Point<float> tooltipPosAnim;
    
    // Nice ticks logic
    struct Tick { float value; juce::String label; };
    std::vector<Tick> xTicks, yTicks;
    void updateNiceTicks();
    static std::vector<Tick> calculateNiceTicks(float min, float max, int maxTicks);
    
    // Drawing methods
    void drawBackground(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawAxes(juce::Graphics& g);
    void drawData(juce::Graphics& g);
    void drawLabels(juce::Graphics& g);
    void drawGlow(juce::Graphics& g, const juce::Path& path, const juce::Colour& color, float radius);
    void drawGradientFill(juce::Graphics& g, const juce::Path& linePath);
    void drawDataPoints(juce::Graphics& g);
    void drawValueLabels(juce::Graphics& g);
    void drawSeries(juce::Graphics& g, const std::vector<std::pair<float, float>>& points, const juce::Colour& color);
    void drawAllSeries(juce::Graphics& g);
    void drawLegend(juce::Graphics& g);
    
    // Animation
    void startAnimation();
    void updateAnimation();
    std::pair<float, float> interpolatePoint(const std::pair<float, float>& from,
                                              const std::pair<float, float>& to,
                                              float t) const;
    
    // Coordinate conversion
    float xToScreen(float x) const;
    float yToScreen(float y) const;
    float screenToX(float screenX) const;
    float screenToY(float screenY) const;
    juce::Rectangle<int> getChartArea() const;
    
    // Auto-scaling
    void calculateAutoScale();
    
    // Easing functions for smooth animations
    static float easeOutCubic(float t) {
        return 1.0f - std::pow(1.0f - t, 3.0f);
    }
    static float easeOutExpo(float t) {
        return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
    }
    
    // Tooltip state
    bool isHovering = false;
    juce::Point<float> mousePosition;
    int hoveredPointIndex = -1;
    
    // Tooltip drawing
    void drawTooltip(juce::Graphics& g);
    int findNearestPoint(juce::Point<float> pos, float maxDistance = 20.0f) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetricsChart)
};

} // namespace ui
} // namespace zenith
