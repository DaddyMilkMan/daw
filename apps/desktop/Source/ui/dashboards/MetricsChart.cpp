/*
  ==============================================================================
    MetricsChart.cpp
    Chart component for metrics visualization - premium dark theme
  ==============================================================================
*/

#include "MetricsChart.h"
#include "../../ui/design-system/ZenithDesignSystem.h"
#include "../../ui/design-system/ColorBridge.h"

namespace zenith {
namespace ui {

// MetricsChart Implementation
MetricsChart::MetricsChart(ChartType type) : chartType(type) {
    setOpaque(false);  // Required for layered overlays and smooth composition
    
    // Initialize animated ranges with defaults
    xMinAnim.setTarget(0.0f, true);
    xMaxAnim.setTarget(100.0f, true);
    yMinAnim.setTarget(0.0f, true);
    yMaxAnim.setTarget(1.0f, true);
}

MetricsChart::~MetricsChart() {
    stopTimer();
}

void MetricsChart::addDataPoint(float x, float y) {
    dataPoints.push_back({x, y});
    
    // Keep only last 1000 points
    if (dataPoints.size() > 1000) {
        dataPoints.erase(dataPoints.begin());
    }
    
    repaint();
}

void MetricsChart::setDataPoints(const std::vector<std::pair<float, float>>& points) {
    dataPoints = points;
    repaint();
}

void MetricsChart::clearData() {
    dataPoints.clear();
    repaint();
}

void MetricsChart::setChartType(ChartType type) {
    chartType = type;
    repaint();
}

void MetricsChart::setLineColor(const juce::Colour& color) {
    lineColor = color;
    repaint();
}

void MetricsChart::setBackgroundColor(const juce::Colour& color) {
    backgroundColor = color;
    repaint();
}

void MetricsChart::setShowGrid(bool show) {
    showGrid = show;
    repaint();
}

void MetricsChart::setShowLabels(bool show) {
    showLabels = show;
    repaint();
}

void MetricsChart::setShowLegend(bool show) {
    showLegend_ = show;
    repaint();
}

void MetricsChart::setXRange(float min, float max) {
    xMin = min;
    xMax = max;
    repaint();
}

void MetricsChart::setYRange(float min, float max) {
    yMin = min;
    yMax = max;
    repaint();
}

void MetricsChart::setXLabel(const juce::String& label) {
    xLabel = label;
    repaint();
}

void MetricsChart::setYLabel(const juce::String& label) {
    yLabel = label;
    repaint();
}

void MetricsChart::paint(juce::Graphics& g) {
    drawBackground(g);
    
    if (showGrid) {
        drawGrid(g);
    }
    
    drawAxes(g);
    drawData(g);
    
    // Draw multi-series data
    drawAllSeries(g);
    
    if (showLabels) {
        drawLabels(g);
        drawValueLabels(g);
    }
    
    // Draw legend for multi-series
    if (showLegend_ && !series_.empty()) {
        drawLegend(g);
    }
    
    // Draw tooltip on top of everything
    if (isHovering && hoveredPointIndex >= 0) {
        drawTooltip(g);
    }
}

void MetricsChart::resized() {
    // Chart will be redrawn on paint
}

void MetricsChart::drawGrid(juce::Graphics& g) {
    auto chartArea = getChartArea();
    g.setColour(gridColor);
    
    // Vertical grid lines (at X ticks)
    for (const auto& tick : xTicks) {
        float x = xToScreen(tick.value);
        if (x < chartArea.getX() || x > chartArea.getRight()) continue;
        
        float dash[] = { 4.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>(x, (float)chartArea.getY(), x, (float)chartArea.getBottom()), dash, 2, 1.0f);
    }
    
    // Horizontal grid lines (at Y ticks)
    for (const auto& tick : yTicks) {
        float y = yToScreen(tick.value);
        if (y < chartArea.getY() || y > chartArea.getBottom()) continue;
        
        float dash[] = { 4.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>((float)chartArea.getX(), y, (float)chartArea.getRight(), y), dash, 2, 1.0f);
    }
}

void MetricsChart::drawAxes(juce::Graphics& g) {
    auto chartArea = getChartArea();
    g.setColour(axisColor);
    
    // Y-axis
    g.drawLine((float)chartArea.getX(), (float)chartArea.getY(), 
               (float)chartArea.getX(), (float)chartArea.getBottom(), 1.5f);
               
    // X-axis
    g.drawLine((float)chartArea.getX(), (float)chartArea.getBottom(), 
               (float)chartArea.getRight(), (float)chartArea.getBottom(), 1.5f);
}

void MetricsChart::drawData(juce::Graphics& g) {
    if (!dataPoints.empty()) {
        const auto& points = animated_ ? animatedPoints : dataPoints;
        if (!points.empty()) {
            drawSeries(g, points, lineColor);
        }
    }
}

void MetricsChart::drawAllSeries(juce::Graphics& g) {
    for (const auto& [name, series] : series_) {
        if (!series.points.empty()) {
            const auto& points = animated_ ? series.animatedPoints : series.points;
            if (!points.empty()) {
                drawSeries(g, points, series.color);
            }
        }
    }
}

void MetricsChart::drawSeries(juce::Graphics& g, const std::vector<std::pair<float, float>>& points, const juce::Colour& color) {
    if (points.empty()) return;
    
    g.setColour(color);
    
    auto bounds = getLocalBounds().reduced(marginLeft_, marginTop_)
                         .withTrimmedRight(marginRight_ - marginLeft_)
                         .withTrimmedBottom(marginBottom_ - marginTop_);
    
    switch (chartType) {
        case ChartType::Line:
            {
                juce::Path path;
                bool started = false;
                
                for (const auto& point : points) {
                    float x = xToScreen(point.first);
                    float y = yToScreen(point.second);
                    
                    if (!started) {
                        path.startNewSubPath(x, y);
                        started = true;
                    } else {
                        path.lineTo(x, y);
                    }
                }
                
                // Draw gradient fill if enabled
                if (showGradientFill_) {
                    drawGradientFill(g, path);
                }
                
                // Draw glow if enabled
                if (showGlow_) {
                    drawGlow(g, path, color, 10.0f);
                    // Re-set color after glow
                    g.setColour(color);
                }
                
                g.strokePath(path, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            break;
            
        case ChartType::Bar:
            {
                float barWidth = bounds.getWidth() / static_cast<float>(points.size());
                float spacer = barWidth * 0.2f;
                barWidth -= spacer;
                
                for (size_t i = 0; i < points.size(); ++i) {
                    float x = bounds.getX() + i * (barWidth + spacer) + spacer * 0.5f;
                    float y = yToScreen(points[i].second);
                    float zeroY = yToScreen(0.0f); // Base bars at y=0
                    float height = zeroY - y;
                    
                    juce::Rectangle<float> barRect(x, y, barWidth, height);
                    g.fillRect(barRect);
                    
                    if (showGlow_) {
                         // Simple outer glow for bars? Maybe overkill for now.
                    }
                }
            }
            break;
            
        case ChartType::Scatter:
            {
                for (const auto& point : points) {
                    float x = xToScreen(point.first);
                    float y = yToScreen(point.second);
                    
                    float radius = 4.0f;
                    g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);
                    
                    if (showGlow_) {
                        juce::Path p;
                        p.addEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);
                        drawGlow(g, p, color, 8.0f);
                        g.setColour(color); // Restore
                        g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);
                    }
                }
            }
            break;
            
        case ChartType::Area:
            // Area chart implementation (similar to Line but closed)
             {
                juce::Path path;
                bool started = false;
                
                for (const auto& point : points) {
                    float x = xToScreen(point.first);
                    float y = yToScreen(point.second);
                    
                    if (!started) {
                        path.startNewSubPath(x, y);
                        started = true;
                    } else {
                        path.lineTo(x, y);
                    }
                }
                
                // Create area path
                juce::Path areaPath = path;
                areaPath.lineTo(path.getCurrentPosition().getX(), bounds.getBottom());
                areaPath.lineTo(path.getBounds().getX(), bounds.getBottom());
                areaPath.closeSubPath();
                
                g.setGradientFill(juce::ColourGradient(color.withAlpha(0.6f), 0, bounds.getY(),
                                                      color.withAlpha(0.0f), 0, bounds.getBottom(), false));
                g.fillPath(areaPath);
                
                g.setColour(color);
                g.strokePath(path, juce::PathStrokeType(2.5f));
            }
            break;
    }
}

void MetricsChart::drawBackground(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Subtle gradient background for the premium dark theme
    g.setGradientFill(juce::ColourGradient(backgroundColor.brighter(0.02f), 0, 0,
                                          backgroundColor, 0, bounds.getBottom(), false));
    g.fillAll();
    
    // Subtle inner border
    g.setColour(juce::Colour(0x0AFFFFFF));
    g.drawRect(bounds, 1.0f);
}

void MetricsChart::drawGlow(juce::Graphics& g, const juce::Path& path, const juce::Colour& color, float radius) {
    if (path.isEmpty()) return;
    
    // Right Path: Physically accurate glow falloff (5-layer exponential)
    for (int i = 0; i < 5; ++i) {
        float ratio = (i + 1) / 5.0f;
        float opacity = 0.25f * std::exp(-3.0f * ratio);
        float width = radius * ratio * 2.5f + 1.0f;
        
        g.setColour(color.withAlpha(opacity));
        g.strokePath(path, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void MetricsChart::drawGradientFill(juce::Graphics& g, const juce::Path& linePath) {
    if (linePath.isEmpty()) return;
    
    auto bounds = getChartArea();
    juce::Path areaPath = linePath;
    
    // Close the path to the bottom of the chart
    auto boundsPath = linePath.getBounds();
    areaPath.lineTo(boundsPath.getRight(), bounds.getBottom());
    areaPath.lineTo(boundsPath.getX(), bounds.getBottom());
    areaPath.closeSubPath();
    
    juce::ColourGradient gradient(gradientTopColor, 0, bounds.getY(),
                                 gradientBottomColor, 0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillPath(areaPath);
}

void MetricsChart::drawDataPoints(juce::Graphics& g) {
    // Implementation for explicit data points drawing if needed
    // Currently handled inside drawSeries for Scatter type or implicitly
}

void MetricsChart::drawValueLabels(juce::Graphics& g) {
    auto chartArea = getChartArea();
    g.setColour(labelColor.withAlpha(0.8f));
    g.setFont(zenith::design::typography::getJuceFont(zenith::design::typography::FONT_XS));
    
    // Y-axis value labels (at Nice Tick positions)
    for (const auto& tick : yTicks) {
        float y = yToScreen(tick.value);
        if (y < chartArea.getY() - 5 || y > chartArea.getBottom() + 5) continue;
        
        g.drawText(tick.label, -5, (int)y - 8, marginLeft_, 16,
                   juce::Justification::centredRight);
        
        // Minor tick mark
        g.setColour(axisColor.withAlpha(0.4f));
        g.drawLine((float)chartArea.getX() - 3.0f, y, (float)chartArea.getX(), y, 1.0f);
        g.setColour(labelColor.withAlpha(0.8f));
    }
    
    // X-axis value labels (at Nice Tick positions)
    for (const auto& tick : xTicks) {
        float x = xToScreen(tick.value);
        if (x < chartArea.getX() - 5 || x > chartArea.getRight() + 5) continue;
        
        g.drawText(tick.label, (int)x - 25, chartArea.getBottom() + 4, 50, 16,
                   juce::Justification::centred);
                   
        // Minor tick mark
        g.setColour(axisColor.withAlpha(0.4f));
        g.drawLine(x, (float)chartArea.getBottom(), x, (float)chartArea.getBottom() + 3.0f, 1.0f);
        g.setColour(labelColor.withAlpha(0.8f));
    }
}

void MetricsChart::drawLabels(juce::Graphics& g) {
    g.setColour(labelColor);
    g.setFont(zenith::design::typography::getJuceFont(zenith::design::typography::FONT_SM));
    
    auto chartArea = getChartArea();
    
    // X axis label (centered below chart)
    if (!xLabel.isEmpty()) {
        g.drawText(xLabel, chartArea.getX(), chartArea.getBottom() + 25,
                   chartArea.getWidth(), 15, juce::Justification::centred);
    }
    
    // Y axis label (rotated on left side)
    if (!yLabel.isEmpty()) {
        g.setFont(zenith::design::typography::getJuceFont(zenith::design::typography::FONT_XS));
        juce::AffineTransform rotation = juce::AffineTransform::rotation(
            -juce::MathConstants<float>::halfPi,
            marginLeft_ / 2.0f, getHeight() / 2.0f
        );
        g.addTransform(rotation);
        g.drawText(yLabel, -getHeight() / 2.0f - chartArea.getHeight() / 2.0f,
                   marginLeft_ / 2 - 6, chartArea.getHeight(), 12, 
                   juce::Justification::centred);
        g.addTransform(rotation.inverted());
    }
}

// Coordinate conversion (Right Path versions)
float MetricsChart::xToScreen(float x) const {
    auto chartArea = getChartArea();
    if (xMaxAnim.current <= xMinAnim.current) return chartArea.getX();
    float normalized = (x - xMinAnim.current) / (xMaxAnim.current - xMinAnim.current);
    return chartArea.getX() + normalized * chartArea.getWidth();
}

float MetricsChart::yToScreen(float y) const {
    auto chartArea = getChartArea();
    if (yMaxAnim.current <= yMinAnim.current) return chartArea.getBottom();
    float normalized = (y - yMinAnim.current) / (yMaxAnim.current - yMinAnim.current);
    return chartArea.getBottom() - normalized * chartArea.getHeight();
}

float MetricsChart::screenToX(float screenX) const {
    auto chartArea = getChartArea();
    if (chartArea.getWidth() == 0) return xMinAnim.current;
    float normalized = (screenX - chartArea.getX()) / (float)chartArea.getWidth();
    return xMinAnim.current + normalized * (xMaxAnim.current - xMinAnim.current);
}

float MetricsChart::screenToY(float screenY) const {
    auto chartArea = getChartArea();
    if (chartArea.getHeight() == 0) return yMinAnim.current;
    float normalized = 1.0f - (screenY - chartArea.getY()) / (float)chartArea.getHeight();
    return yMinAnim.current + normalized * (yMaxAnim.current - yMinAnim.current);
}

void MetricsChart::mouseMove(const juce::MouseEvent& event) {
    mousePosition = event.position;
    int nearestPoint = findNearestPoint(mousePosition);
    
    if (nearestPoint != hoveredPointIndex) {
        hoveredPointIndex = nearestPoint;
        isHovering = (nearestPoint >= 0);
        repaint();
    } else if (isHovering) {
        // Still hovering, update position for tooltip
        repaint();
    }
}

void MetricsChart::mouseExit(const juce::MouseEvent& event) {
    isHovering = false;
    hoveredPointIndex = -1;
    repaint();
}

int MetricsChart::findNearestPoint(juce::Point<float> pos, float maxDistance) const {
    const auto& points = animated_ ? animatedPoints : dataPoints;
    if (points.empty()) return -1;
    
    int nearestIndex = -1;
    float nearestDistanceSq = maxDistance * maxDistance;
    
    for (size_t i = 0; i < points.size(); ++i) {
        float screenX = xToScreen(points[i].first);
        float screenY = yToScreen(points[i].second);
        
        float dx = pos.x - screenX;
        float dy = pos.y - screenY;
        float distSq = dx * dx + dy * dy;
        
        if (distSq < nearestDistanceSq) {
            nearestDistanceSq = distSq;
            nearestIndex = static_cast<int>(i);
        }
    }
    
    return nearestIndex;
}

void MetricsChart::drawTooltip(juce::Graphics& g) {
    if (tooltipAlphaAnim < 0.01f) return;

    const auto& points = animated_ ? animatedPoints : dataPoints;
    if (hoveredPointIndex < 0 || hoveredPointIndex >= static_cast<int>(points.size())) 
        return;
    
    const auto& point = points[hoveredPointIndex];
    float screenX = xToScreen(point.first);
    float screenY = yToScreen(point.second);
    
    // Use the animated alpha for the whole tooltip group
    g.beginTransparencyLayer(tooltipAlphaAnim);
    
    // Draw highlight ring on the hovered point
    g.setColour(lineColor.withAlpha(0.6f));
    g.drawEllipse(screenX - 10, screenY - 10, 20, 20, 2.0f);
    
    // Format tooltip text
    juce::String tooltipText;
    tooltipText += (xLabel.isEmpty() ? "X: " : xLabel + ": ") + juce::String(point.first, 1) + "\n";
    tooltipText += (yLabel.isEmpty() ? "Y: " : yLabel + ": ") + juce::String(point.second, 4);
    
    // Premium Typography
    auto tooltipFont = zenith::design::typography::getJuceFont(zenith::design::typography::FONT_XS);
    g.setFont(tooltipFont);
    
    auto getLayoutWidth = [&](const juce::String& t) {
        juce::AttributedString as;
        as.setText(t);
        as.setFont(tooltipFont);
        juce::TextLayout tl;
        tl.createLayout(as, 500.0f); // max width for measurement
        return tl.getWidth();
    };

    float textWidth = getLayoutWidth(tooltipText.upToFirstOccurrenceOf("\n", false, false));
    float secondLineWidth = getLayoutWidth(tooltipText.fromLastOccurrenceOf("\n", false, false));
    textWidth = juce::jmax(textWidth, secondLineWidth);
    
    float tooltipWidth = textWidth + 24.0f;
    float tooltipHeight = 44.0f;
    
    // Use animated position
    float tooltipX = tooltipPosAnim.x + 15.0f;
    float tooltipY = tooltipPosAnim.y - tooltipHeight - 10.0f;
    
    auto bounds = getLocalBounds().toFloat();
    if (tooltipX + tooltipWidth > bounds.getRight()) tooltipX = tooltipPosAnim.x - tooltipWidth - 15.0f;
    if (tooltipY < bounds.getY()) tooltipY = tooltipPosAnim.y + 20.0f;
    
    juce::Rectangle<float> tooltipBounds(tooltipX, tooltipY, tooltipWidth, tooltipHeight);
    
    // Modern glassmorphic style
    g.setColour(juce::Colour(0xF0181818));  // Deep slightly transparent back
    g.fillRoundedRectangle(tooltipBounds, zenith::design::dimensions::RADIUS_SM);
    
    // Accent border
    g.setColour(lineColor.withAlpha(0.7f));
    g.drawRoundedRectangle(tooltipBounds, zenith::design::dimensions::RADIUS_SM, 1.5f);
    
    // Inner highlight
    g.setColour(juce::Colour(0x10FFFFFF));
    g.drawRoundedRectangle(tooltipBounds.reduced(1.0f), zenith::design::dimensions::RADIUS_SM - 1.0f, 1.0f);
    
    // Text
    g.setColour(zenith::design::toJuceColour(zenith::design::colors::TEXT_PRIMARY));
    g.drawFittedText(tooltipText, tooltipBounds.reduced(12.0f, 8.0f).toNearestInt(),
                     juce::Justification::centredLeft, 2);

    g.endTransparencyLayer();
}

// ==================== Multi-Series API Implementation ====================

void MetricsChart::addSeries(const juce::String& name, const juce::Colour& color) {
    DataSeries newSeries;
    newSeries.name = name;
    newSeries.color = color;
    series_[name] = newSeries;
    repaint();
}

void MetricsChart::addDataPointToSeries(const juce::String& name, float x, float y) {
    auto it = series_.find(name);
    if (it != series_.end()) {
        it->second.points.push_back({x, y});
        
        // Keep only last 1000 points per series
        if (it->second.points.size() > 1000) {
            it->second.points.erase(it->second.points.begin());
        }
        
        // Auto-scale if enabled
        if (autoScale_) {
            calculateAutoScale();
        }
        
        repaint();
    }
}

void MetricsChart::setSeriesData(const juce::String& name, const std::vector<std::pair<float, float>>& points) {
    auto it = series_.find(name);
    if (it != series_.end()) {
        it->second.points = points;
        
        // Update animated points for smooth transition
        if (animated_) {
            it->second.animatedPoints = it->second.points;
        }
        
        if (autoScale_) {
            calculateAutoScale();
        }
        
        repaint();
    }
}

void MetricsChart::clearSeries(const juce::String& name) {
    auto it = series_.find(name);
    if (it != series_.end()) {
        it->second.points.clear();
        it->second.animatedPoints.clear();
        repaint();
    }
}

void MetricsChart::clearAllSeries() {
    series_.clear();
    repaint();
}

// ==================== Additional Appearance Methods ====================

void MetricsChart::setShowGlow(bool show) {
    showGlow_ = show;
    repaint();
}

void MetricsChart::setShowGradientFill(bool show) {
    showGradientFill_ = show;
    repaint();
}

void MetricsChart::setAnimated(bool animated) {
    animated_ = animated;
    if (animated) {
        startTimer(1000 / animationFPS_);
    } else {
        stopTimer();
    }
    repaint();
}

void MetricsChart::setAutoScale(bool autoScale) {
    autoScale_ = autoScale;
    if (autoScale) {
        calculateAutoScale();
    }
    repaint();
}


// ==================== Legend Drawing ====================

void MetricsChart::drawLegend(juce::Graphics& g) {
    if (series_.empty()) return;
    
    auto chartArea = getChartArea();
    
    const float legendPadding = 12.0f;
    const float swatchSize = 10.0f;
    const float rowHeight = 20.0f;
    const float textPadding = 8.0f;
    
    auto legendFont = zenith::design::typography::getJuceFont(zenith::design::typography::FONT_XS);
    g.setFont(legendFont);
    
    float maxTextWidth = 0.0f;
    for (const auto& [name, series] : series_) {
        juce::AttributedString as;
        as.setText(name);
        as.setFont(legendFont);
        juce::TextLayout tl;
        tl.createLayout(as, 500.0f); // max width for measurement
        maxTextWidth = juce::jmax(maxTextWidth, tl.getWidth());
    }
    
    float legendWidth = legendPadding * 2 + swatchSize + textPadding + maxTextWidth;
    float legendHeight = legendPadding * 2 + series_.size() * rowHeight;
    
    float legendX = chartArea.getRight() - legendWidth - 12.0f;
    float legendY = chartArea.getY() + 12.0f;
    
    juce::Rectangle<float> legendBounds(legendX, legendY, legendWidth, legendHeight);
    
    g.setColour(juce::Colour(0xCC121212));
    g.fillRoundedRectangle(legendBounds, zenith::design::dimensions::RADIUS_SM);
    g.setColour(juce::Colour(0x30FFFFFF));
    g.drawRoundedRectangle(legendBounds, zenith::design::dimensions::RADIUS_SM, 1.0f);
    
    float currentY = legendY + legendPadding;
    for (const auto& [name, series] : series_) {
        juce::Rectangle<float> swatchBounds(legendX + legendPadding, 
                                            currentY + (rowHeight - swatchSize) / 2.0f,
                                            swatchSize, swatchSize);
        g.setColour(series.color);
        g.fillRoundedRectangle(swatchBounds, 2.0f);
        
        g.setColour(labelColor);
        g.drawText(name, swatchBounds.getRight() + textPadding, currentY, maxTextWidth, rowHeight, juce::Justification::centredLeft);
        currentY += rowHeight;
    }
}

// ==================== Animation Helpers ====================

void MetricsChart::startAnimation() {
    animationProgress_ = 0.0f;
    if (animated_) {
        startTimer(1000 / animationFPS_);
    }
}

void MetricsChart::updateAnimation() {
    if (animationProgress_ < 1.0f) {
        animationProgress_ += animationSpeed_;
        if (animationProgress_ > 1.0f) {
            animationProgress_ = 1.0f;
        }
        
        // Interpolate animated points
        float t = easeOutCubic(animationProgress_);
        
        // Interpolate primary data points
        if (previousPoints.size() == dataPoints.size()) {
            animatedPoints.resize(dataPoints.size());
            for (size_t i = 0; i < dataPoints.size(); ++i) {
                animatedPoints[i] = interpolatePoint(previousPoints[i], dataPoints[i], t);
            }
        } else {
            animatedPoints = dataPoints;
        }
        
        // Interpolate series points
        for (auto& [name, series] : series_) {
            if (series.animatedPoints.size() != series.points.size()) {
                series.animatedPoints = series.points;
            }
        }
    }
}

std::pair<float, float> MetricsChart::interpolatePoint(const std::pair<float, float>& from,
                                                        const std::pair<float, float>& to,
                                                        float t) const {
    return {
        from.first + (to.first - from.first) * t,
        from.second + (to.second - from.second) * t
    };
}

// ==================== Chart Area Helper ====================

juce::Rectangle<int> MetricsChart::getChartArea() const {
    return getLocalBounds().reduced(marginLeft_, marginTop_)
                          .withTrimmedRight(marginRight_ - marginLeft_)
                          .withTrimmedBottom(marginBottom_ - marginTop_);
}

// ==================== Auto-Scale Calculation ====================

void MetricsChart::timerCallback() {
    bool needsRepaint = false;
    
    // Update axis animations (Smooth spring movement)
    needsRepaint |= xMinAnim.update();
    needsRepaint |= xMaxAnim.update();
    needsRepaint |= yMinAnim.update();
    needsRepaint |= yMaxAnim.update();
    
    // Update tooltip appearance transitions
    float targetAlpha = isHovering ? 1.0f : 0.0f;
    if (std::abs(tooltipAlphaAnim - targetAlpha) > 0.005f) {
        tooltipAlphaAnim += (targetAlpha - tooltipAlphaAnim) * 0.15f;
        needsRepaint = true;
    }
    
    if (isHovering || tooltipAlphaAnim > 0.01f) {
        tooltipPosAnim += (mousePosition - tooltipPosAnim) * 0.25f;
        needsRepaint = true;
    }

    // Update data animations
    updateAnimation();
    
    if (needsRepaint) {
        updateNiceTicks();
        repaint();
    }
}

// Nice Ticks Algorithm (Right Path)
std::vector<MetricsChart::Tick> MetricsChart::calculateNiceTicks(float min, float max, int maxTicks) {
    std::vector<Tick> ticks;
    if (max <= min || maxTicks <= 1) return ticks;

    float range = max - min;
    float roughStep = range / (maxTicks - 1);
    float magnitude = std::pow(10.0f, std::floor(std::log10(roughStep)));
    float normalizedStep = roughStep / magnitude;

    float niceStep;
    if (normalizedStep < 1.5f) niceStep = 1.0f;
    else if (normalizedStep < 3.0f) niceStep = 2.0f;
    else if (normalizedStep < 7.0f) niceStep = 5.0f;
    else niceStep = 10.0f;

    niceStep *= magnitude;

    float startValue = std::ceil(min / niceStep) * niceStep;
    for (float v = startValue; v <= max + (niceStep * 0.01f); v += niceStep) {
        Tick t;
        t.value = v;
        if (std::abs(v) < 0.0001f) t.label = "0";
        else if (niceStep >= 1.0f) t.label = juce::String(std::round(v));
        else if (niceStep >= 0.1f) t.label = juce::String(v, 1);
        else t.label = juce::String(v, 2);
        ticks.push_back(t);
    }
    return ticks;
}

void MetricsChart::updateNiceTicks() {
    xTicks = calculateNiceTicks(xMinAnim.current, xMaxAnim.current, 6);
    yTicks = calculateNiceTicks(yMinAnim.current, yMaxAnim.current, 5);
}

// ==================== Auto-Scale (Animated Version) ====================

void MetricsChart::calculateAutoScale() {
    if (dataPoints.empty() && series_.empty()) return;
    
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    bool hasData = false;
    
    auto process = [&](const std::vector<std::pair<float, float>>& pts) {
        for (const auto& p : pts) {
            minX = std::min(minX, p.first); maxX = std::max(maxX, p.first);
            minY = std::min(minY, p.second); maxY = std::max(maxY, p.second);
            hasData = true;
        }
    };

    process(dataPoints);
    for (const auto& [name, series] : series_) process(series.points);
    
    if (hasData) {
        float dx = (maxX - minX) * 0.05f; if (dx < 0.1f) dx = 1.0f;
        float dy = (maxY - minY) * 0.15f; if (dy < 0.1f) dy = 0.1f;
        
        xMinAnim.setTarget(minX - dx);
        xMaxAnim.setTarget(maxX + dx);
        yMinAnim.setTarget(std::max(0.0f, minY - dy));
        yMaxAnim.setTarget(maxY + dy);
    }
}

} // namespace ui
} // namespace zenith
