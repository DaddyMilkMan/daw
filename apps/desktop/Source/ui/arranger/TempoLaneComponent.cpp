/**
 * @file TempoLaneComponent.cpp
 * @brief Tempo lane implementation - FULLY IMPLEMENTED
 * 
 * Allows visual editing of tempo automation.
 */

#include "TempoLaneComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkFont.h>
#include <effects/SkGradientShader.h>

using namespace zenith;

//==============================================================================
TempoLaneComponent::TempoLaneComponent(ProjectState& state)
    : projectState(state)
{
    setWantsKeyboardFocus(true);
    projectState.getState().addListener(this);
}

TempoLaneComponent::~TempoLaneComponent()
{
    projectState.getState().removeListener(this);
}

//==============================================================================
// Component Interface
//==============================================================================

void TempoLaneComponent::resized()
{
    repaint();
}

void TempoLaneComponent::drawSkia(SkCanvas* canvas)
{
    using namespace zenith::design;

    auto bounds = getLocalBounds();
    float width = (float)bounds.getWidth();
    float height = (float)bounds.getHeight();

    // Background - use design system BG_02 for panels
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_02);
    canvas->drawRect(SkRect::MakeWH(width, height), bgPaint);

    // Grid
    drawGrid(canvas);

    // Curve
    drawTempoCurve(canvas);

    // Points
    drawTempoPoints(canvas);
    
    // Border
    SkPaint borderPaint;
    borderPaint.setColor(SK_ColorBLACK);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    canvas->drawRect(SkRect::MakeWH(width, height), borderPaint);
}

void TempoLaneComponent::drawGrid(SkCanvas* canvas) const
{
    using namespace zenith::design;
    
    float width = (float)getWidth();
    SkPaint gridPaint;
    gridPaint.setColor(withAlpha(colors::TEXT_PRIMARY, opacity::BORDER_SUBTLE));
    gridPaint.setStrokeWidth(1.0f);

    // Draw horizontal lines for key BPMs
    std::vector<double> gridBpms = {60, 80, 100, 120, 140, 160, 180, 200};
    
    for (double bpm : gridBpms)
    {
        if (bpm >= minBpm && bpm <= maxBpm)
        {
            float y = bpmToY(bpm);
            canvas->drawLine(0, y, width, y, gridPaint);
            
            // Label
            SkPaint textPaint;
            textPaint.setColor(colors::TEXT_TERTIARY);
            SkFont font = zenith::design::typography::getMonoFont(10.0f);
            canvas->drawString(juce::String(bpm).toStdString().c_str(), 5, y - 2, font, textPaint);
        }
    }
}

void TempoLaneComponent::drawTempoCurve(SkCanvas* canvas) const
{
    using namespace zenith::design;
    auto points = projectState.getTempoMap(); // Assumes sorted by time
    if (points.getNumChildren() == 0) return;

    SkPath curvePath;
    bool first = true;

    // Iterate through points to build path
    for (auto point : points)
    {
        double time = point[ProjectState::PROP_TIME_BEATS];
        double bpm = point[ProjectState::PROP_BPM];
        
        float x = beatsToX(time);
        float y = bpmToY(bpm);

        if (first)
        {
            curvePath.moveTo(0, y); // Start at t=0 with first bpm
            curvePath.lineTo(x, y);
            first = false;
        }
        else
        {
            // Step change for now (or linear ramp if we support it later)
            // Logic: Line to new X, old Y (hold value), then jump to new Y? 
            // Standard DAW automation is usually points connected linearly or hold.
            // Let's assume linear ramp for tempo curves
            SkPoint lastPt;
            if (curvePath.getLastPt(&lastPt)) {
                curvePath.lineTo(x, y); 
            }
        }
    }
    
    // Extend to end
    SkPoint lastPt;
    if (curvePath.getLastPt(&lastPt)) {
        curvePath.lineTo((float)getWidth(), lastPt.fY);
    }

    SkPaint curvePaint;
    curvePaint.setColor(colors::CYAN);  // Use design system accent for tempo curve
    curvePaint.setStyle(SkPaint::kStroke_Style);
    curvePaint.setStrokeWidth(2.0f);
    curvePaint.setAntiAlias(true);
    
    canvas->drawPath(curvePath, curvePaint);
    
    // Fill below curve
    curvePath.lineTo((float)getWidth(), (float)getHeight());
    curvePath.lineTo(0, (float)getHeight());
    curvePath.close();
    
    // Use stack-allocated arrays to avoid memory leak (was using new[] without delete)
    SkPoint gradientPoints[2] = {SkPoint::Make(0, 0), SkPoint::Make(0, (float)getHeight())};
    SkColor gradientColors[2] = {withAlpha(colors::CYAN, opacity::GLOW_SUBTLE), withAlpha(colors::CYAN, 0.04f)};
    
    SkPaint fillPaint;
    fillPaint.setShader(SkGradientShader::MakeLinear(
        gradientPoints,
        gradientColors,
        nullptr, 2, SkTileMode::kClamp));
    fillPaint.setStyle(SkPaint::kFill_Style);
    
    canvas->drawPath(curvePath, fillPaint);
}

void TempoLaneComponent::drawTempoPoints(SkCanvas* canvas) const
{
    auto points = projectState.getTempoMap();

    for (auto point : points)
    {
        double time = point[ProjectState::PROP_TIME_BEATS];
        double bpm = point[ProjectState::PROP_BPM];
        juce::String id = point[ProjectState::PROP_ID].toString();
        
        bool selected = (id == selectedPointId);
        drawTempoPoint(canvas, time, bpm, selected);
    }
}

void TempoLaneComponent::drawTempoPoint(SkCanvas* canvas, double timeBeats, double bpm, bool selected) const
{
    using namespace zenith::design;
    
    float x = beatsToX(timeBeats);
    float y = bpmToY(bpm);
    float radius = 4.0f;

    SkPaint pointPaint;
    pointPaint.setColor(selected ? colors::TEXT_PRIMARY : colors::CYAN);
    pointPaint.setStyle(SkPaint::kFill_Style);
    pointPaint.setAntiAlias(true);
    
    canvas->drawCircle(x, y, radius, pointPaint);
    
    if (selected)
    {
        SkPaint ringPaint;
        ringPaint.setColor(colors::TEXT_PRIMARY);
        ringPaint.setStyle(SkPaint::kStroke_Style);
        ringPaint.setStrokeWidth(1.0f);
        ringPaint.setAntiAlias(true);
        canvas->drawCircle(x, y, radius + 2.0f, ringPaint);
    }
}

//==============================================================================
// Interaction
//==============================================================================

void TempoLaneComponent::mouseDown(const juce::MouseEvent& event)
{
    juce::String id = findPointAt((float)event.x, (float)event.y);
    
    if (id.isNotEmpty())
    {
        selectedPointId = id;
        isDraggingPoint = true;
        dragStartX = (float)event.x;
        dragStartY = (float)event.y;
    }
    else
    {
        selectedPointId.clear();
    }
    repaint();
}

void TempoLaneComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingPoint && selectedPointId.isNotEmpty())
    {
        double newBeats = xToBeats((float)event.x);
        double newBpm = yToBpm((float)event.y);
        
        newBeats = std::max(0.0, newBeats);
        newBpm = juce::jlimit(minBpm, maxBpm, newBpm);
        
        projectState.moveTempoChange(selectedPointId, newBeats, newBpm, "Move Tempo Change");
    }
}

void TempoLaneComponent::mouseUp(const juce::MouseEvent&)
{
    isDraggingPoint = false;
}

void TempoLaneComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Add point
    double beats = xToBeats((float)event.x);
    double bpm = yToBpm((float)event.y);
    
    // Snap bpm
    bpm = std::round(bpm);
    
    projectState.addTempoChange(beats, bpm, "Add Tempo Change");
}

void TempoLaneComponent::mouseMove(const juce::MouseEvent& event)
{
    juce::String id = findPointAt((float)event.x, (float)event.y);
    if (id != hoveredPointId)
    {
        hoveredPointId = id;
        repaint();
    }
}

void TempoLaneComponent::mouseExit(const juce::MouseEvent&)
{
    if (hoveredPointId.isNotEmpty())
    {
        hoveredPointId.clear();
        repaint();
    }
}

bool TempoLaneComponent::keyPressed(const juce::KeyPress& key)
{
    if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) && selectedPointId.isNotEmpty())
    {
        projectState.deleteTempoChange(selectedPointId, "Delete Tempo Change");
        selectedPointId.clear();
        repaint();
        return true;
    }
    return false;
}

//==============================================================================
// Helpers
//==============================================================================

double TempoLaneComponent::xToBeats(float x) const
{
    float width = (float)getWidth();
    if (width <= 0.0f) return 0.0;
    return viewStartBeats + (x / width) * (viewEndBeats - viewStartBeats);
}

float TempoLaneComponent::beatsToX(double beats) const
{
    float width = (float)getWidth();
    double range = viewEndBeats - viewStartBeats;
    if (range <= 0.0) return 0.0f;
    return (float)((beats - viewStartBeats) / range * width);
}

double TempoLaneComponent::yToBpm(float y) const
{
    float height = (float)getHeight();
    if (height <= 0.0f) return minBpm;
    
    // Inverted Y: 0 is maxBpm, height is minBpm
    double normalized = y / height;
    return maxBpm - normalized * (maxBpm - minBpm);
}

float TempoLaneComponent::bpmToY(double bpm) const
{
    float height = (float)getHeight();
    double range = maxBpm - minBpm;
    if (range <= 0.0) return 0.0f;
    
    // Inverted Y
    return (float)((maxBpm - bpm) / range * height);
}

juce::String TempoLaneComponent::findPointAt(float x, float y) const
{
    const float kHitRadius = 6.0f;
    
    auto points = projectState.getTempoMap();
    juce::String detectedId;
    float minDist = 99999.0f;
    
    for (auto point : points)
    {
        double time = point[ProjectState::PROP_TIME_BEATS];
        double bpm = point[ProjectState::PROP_BPM];
        
        float px = beatsToX(time);
        float py = bpmToY(bpm);
        
        float dist = std::sqrt(std::pow(x - px, 2) + std::pow(y - py, 2));
        
        if (dist < kHitRadius && dist < minDist)
        {
            minDist = dist;
            detectedId = point[ProjectState::PROP_ID].toString();
        }
    }
    
    return detectedId;
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void TempoLaneComponent::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&)
{
    repaint();
}

void TempoLaneComponent::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&)
{
    repaint();
}

void TempoLaneComponent::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int)
{
    repaint();
}

void TempoLaneComponent::valueTreeChildOrderChanged(juce::ValueTree&, int, int)
{
    repaint();
}

void TempoLaneComponent::valueTreeParentChanged(juce::ValueTree&)
{
    repaint();
}