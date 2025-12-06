/**
 * @file AutomationLaneComponent.cpp
 * @brief Implementation of AutomationLaneComponent
 *
 * Phase U5: Automation Lanes UI
 * POLISH: Skia rendering with smooth curves, themed colors, and hover tooltips
 */

#include "../../include/ui/AutomationLaneComponent.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkRect.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkRRect.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkTextBlob.h>
    #include <include/core/SkSurface.h>
    #include <include/core/SkImageInfo.h>
#endif

//==============================================================================
// Constructor / Destructor
//==============================================================================

AutomationLaneComponent::AutomationLaneComponent(zenith::ProjectState& state,
                                                 const juce::String& trackId_,
                                                 const juce::String& paramId_)
    : projectState(state),
      trackId(trackId_),
      paramId(paramId_)
{
    // Get envelope node (creates if doesn't exist)
    envelopeNode = projectState.getOrCreateAutomationEnvelope(trackId, paramId);
    envelopeNode.addListener(this);

    // Set default parameter info based on parameter type
    if (paramId == "volume")
        paramInfo = getDefaultVolumeInfo();
    else if (paramId == "pan")
    {
        paramInfo.displayName = "Pan";
        paramInfo.minValue = -1.0;
        paramInfo.maxValue = 1.0;
        paramInfo.units = "";
        paramInfo.valueToString = [](double v) { return juce::String(v, 2); };
    }
    else if (paramId == "mute")
    {
        paramInfo.displayName = "Mute";
        paramInfo.minValue = 0.0;
        paramInfo.maxValue = 1.0;
        paramInfo.units = "";
        paramInfo.valueToString = [](double v) { return v >= 0.5 ? "ON" : "OFF"; };
    }
}

AutomationLaneComponent::~AutomationLaneComponent()
{
    if (envelopeNode.isValid())
        envelopeNode.removeListener(this);
}

//==============================================================================
// Visual Settings
//==============================================================================

void AutomationLaneComponent::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat = juce::jmax(1.0, ppb);
    repaint();
}

void AutomationLaneComponent::setScrollOffsetBeats(double offset)
{
    scrollOffsetBeats = offset;
    repaint();
}

void AutomationLaneComponent::setGridSnapEnabled(bool enabled, double gridBeats_)
{
    gridSnapEnabled = enabled;
    gridBeats = juce::jmax(0.25, gridBeats_);
}

//==============================================================================
// Parameter Info
//==============================================================================

void AutomationLaneComponent::setParameterInfo(const ParamInfo& info)
{
    paramInfo = info;
    repaint();
}

AutomationLaneComponent::ParamInfo AutomationLaneComponent::getDefaultVolumeInfo()
{
    ParamInfo info;
    info.displayName = "Volume";
    info.minValue = 0.0;
    info.maxValue = 1.0;
    info.units = "";
    info.valueToString = [](double v)
    {
        // Convert to percentage
        return juce::String(static_cast<int>(v * 100)) + "%";
    };
    return info;
}

//==============================================================================
// Component Overrides - Rendering
//==============================================================================

void AutomationLaneComponent::paint(juce::Graphics& g)
{
#ifdef ZENITH_USE_SKIA
    // Use Skia rendering for automation lanes
    auto& theme = zenith::SkiaTheme::getInstance();
    const auto& colors = theme.getColors();
    const auto& typo = theme.getTypography();

    // Get Skia canvas by wrapping JUCE Graphics in a temporary surface
    juce::Image tempImage(juce::Image::ARGB, std::max(1, getWidth()), std::max(1, getHeight()), true);
    {
        juce::Image::BitmapData bitmapData(tempImage, juce::Image::BitmapData::readWrite);
        SkImageInfo info = SkImageInfo::MakeN32Premul(tempImage.getWidth(), tempImage.getHeight());
        auto skSurface = SkSurfaces::WrapPixels(info, bitmapData.getLinePointer(0), bitmapData.lineStride);

        if (skSurface)
        {
            SkCanvas& canvas = *skSurface->getCanvas();
            canvas.clear(SK_ColorTRANSPARENT);

            // Background with subtle tint
            SkPaint bgPaint;
            bgPaint.setColor(colors.bg2);
            bgPaint.setAntiAlias(true);
            canvas.drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);

            // Draw components
            drawGrid(g);  // Keep JUCE grid for now (lighter weight)

            // Draw envelope curve with Skia (smooth)
            rebuildPointHandles();

            if (!pointHandles.empty())
            {
                SkPath path;
                std::vector<PointHandle> sortedHandles = pointHandles;
                std::sort(sortedHandles.begin(), sortedHandles.end(),
                    [](const PointHandle& a, const PointHandle& b) { return a.screenPos.x < b.screenPos.x; });

                // Start from left edge
                path.moveTo(0, sortedHandles[0].screenPos.y);
                path.lineTo(sortedHandles[0].screenPos.x, sortedHandles[0].screenPos.y);

                // Draw smooth curve through points
                for (size_t i = 0; i < sortedHandles.size(); ++i)
                {
                    path.lineTo(sortedHandles[i].screenPos.x, sortedHandles[i].screenPos.y);
                }

                // Extend to right edge
                if (!sortedHandles.empty())
                    path.lineTo(getWidth(), sortedHandles.back().screenPos.y);

                // Draw curve
                SkPaint curvePaint;
                curvePaint.setColor(colors.accentMain);
                curvePaint.setStyle(SkPaint::kStroke_Style);
                curvePaint.setStrokeWidth(2.5f);
                curvePaint.setAntiAlias(true);
                canvas.drawPath(path, curvePaint);
            }

            // Draw control points with selection highlighting
            SkColor selectionColor = colors.primary;
            for (const auto& handle : pointHandles)
            {
                bool isSelected = (handle.pointId == draggedPointId);
                bool isHovered = (handle.pointId == hoveredPointId);

                // Selection background (if selected)
                if (isSelected)
                {
                    SkPaint selBgPaint;
                    selBgPaint.setColor(selectionColor);
                    selBgPaint.setAntiAlias(true);
                    float expandedRadius = handle.radius + 4.0f;
                    canvas.drawCircle(handle.screenPos.x, handle.screenPos.y, expandedRadius, selBgPaint);
                }

                // Control point fill
                SkPaint fillPaint;
                fillPaint.setColor(isSelected ? colors.accentMain :
                                   isHovered ? colors.accentMain :
                                   colors.accentMain);
                fillPaint.setAntiAlias(true);
                canvas.drawCircle(handle.screenPos.x, handle.screenPos.y, handle.radius, fillPaint);

                // Control point border
                SkPaint borderPaint;
                borderPaint.setColor(isSelected ? selectionColor : colors.textStrong);
                borderPaint.setStyle(SkPaint::kStroke_Style);
                borderPaint.setStrokeWidth(isSelected ? 2.5f : 1.5f);
                borderPaint.setAntiAlias(true);
                canvas.drawCircle(handle.screenPos.x, handle.screenPos.y, handle.radius, borderPaint);

                // Hover glow
                if (isHovered && !isSelected)
                {
                    SkPaint glowPaint;
                    glowPaint.setColor(SkColorSetARGB(40, 0, 212, 170));
                    glowPaint.setAntiAlias(true);
                    canvas.drawCircle(handle.screenPos.x, handle.screenPos.y, handle.radius + 3.0f, glowPaint);
                }
            }

            // Draw hover tooltip
            if (hoveredPointId.isNotEmpty())
            {
                juce::String tooltipText = paramInfo.valueToString(hoveredPointValue) +
                                          " @ " + juce::String(hoveredPointTime, 2) + " beats";

                SkFont font;
                font.setSize(typo.tiny.size);

                auto textStr = tooltipText.toStdString();
                SkRect textBounds;
                font.measureText(textStr.c_str(), textStr.length(), SkTextEncoding::kUTF8, &textBounds);

                float tooltipWidth = textBounds.width() + 12.0f;
                float tooltipHeight = 20.0f;
                float tooltipX = hoveredPointScreenPos.x - tooltipWidth / 2.0f;
                float tooltipY = hoveredPointScreenPos.y - tooltipHeight - 10.0f;

                // Clamp to screen
                tooltipX = juce::jlimit(4.0f, (float)getWidth() - tooltipWidth - 4.0f, tooltipX);
                tooltipY = juce::jmax(4.0f, tooltipY);

                // Tooltip background
                SkPaint tooltipBg;
                tooltipBg.setColor(colors.bg3);
                tooltipBg.setAntiAlias(true);
                SkRRect tooltipRect = SkRRect::MakeRectXY(
                    SkRect::MakeXYWH(tooltipX, tooltipY, tooltipWidth, tooltipHeight), 4.0f, 4.0f);
                canvas.drawRRect(tooltipRect, tooltipBg);

                // Tooltip border
                SkPaint tooltipBorder;
                tooltipBorder.setColor(colors.borderSubtle);
                tooltipBorder.setStyle(SkPaint::kStroke_Style);
                tooltipBorder.setStrokeWidth(1.0f);
                tooltipBorder.setAntiAlias(true);
                canvas.drawRRect(tooltipRect, tooltipBorder);

                // Tooltip text
                SkPaint textPaint;
                textPaint.setColor(colors.textStrong);
                textPaint.setAntiAlias(true);
                auto blob = SkTextBlob::MakeFromString(textStr.c_str(), font);
                canvas.drawTextBlob(blob, tooltipX + 6.0f, tooltipY + 14.0f, textPaint);
            }

            // Draw parameter name
            SkFont nameFont;
            nameFont.setSize(typo.body.size);

            SkPaint namePaint;
            namePaint.setColor(colors.textMuted);
            namePaint.setAntiAlias(true);

            auto nameStr = paramInfo.displayName.toStdString();
            auto nameBlob = SkTextBlob::MakeFromString(nameStr.c_str(), nameFont);
            canvas.drawTextBlob(nameBlob, 8.0f, 18.0f, namePaint);
        }
    }

    // Draw the rendered image
    g.drawImageAt(tempImage, 0, 0);
#else
    // Fallback: Original JUCE rendering
    g.fillAll(juce::Colour(0xff2a2a2a));
    drawGrid(g);
    drawEnvelopeCurve(g);
    rebuildPointHandles();
    drawControlPoints(g);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(14.0f);
    g.drawText(paramInfo.displayName, 5, 5, 100, 20, juce::Justification::centredLeft);
#endif
}

void AutomationLaneComponent::resized()
{
    repaint();
}

//==============================================================================
// Drawing Methods
//==============================================================================

void AutomationLaneComponent::drawGrid(juce::Graphics& g)
{
    const int width = getWidth();
    const int height = getHeight();

    g.setColour(juce::Colour(0xff444444));

    // Horizontal lines (value divisions)
    const int numHLines = 5;
    for (int i = 0; i <= numHLines; ++i)
    {
        float y = i * height / static_cast<float>(numHLines);
        g.drawLine(0.0f, y, static_cast<float>(width), y, 0.5f);
    }

    // Vertical lines (beat grid)
    double beatStart = std::floor(scrollOffsetBeats);
    double beatEnd = scrollOffsetBeats + (width / pixelsPerBeat);

    for (double beat = beatStart; beat <= beatEnd; beat += 1.0)
    {
        float x = beatsToPixels(beat);
        if (x >= 0.0f && x <= width)
        {
            // Stronger line every 4 beats (measure)
            if (static_cast<int>(beat) % 4 == 0)
                g.setColour(juce::Colour(0xff666666));
            else
                g.setColour(juce::Colour(0xff444444));

            g.drawLine(x, 0.0f, x, static_cast<float>(height), 0.5f);
        }
    }
}

void AutomationLaneComponent::drawEnvelopeCurve(juce::Graphics& g)
{
    if (!envelopeNode.isValid())
        return;

    const int numPoints = envelopeNode.getNumChildren();
    if (numPoints == 0)
        return;

    // Build path for automation curve
    juce::Path path;
    bool firstPoint = true;

    // Gather all points
    struct PointData
    {
        double timeBeats;
        double value;
        float x;
        float y;
    };
    std::vector<PointData> points;

    for (int i = 0; i < numPoints; ++i)
    {
        auto pointNode = envelopeNode.getChild(i);
        double timeBeats = pointNode.getProperty(zenith::ProjectState::PROP_TIME_BEATS, 0.0);
        double value = pointNode.getProperty(zenith::ProjectState::PROP_VALUE, 0.0);

        PointData pt;
        pt.timeBeats = timeBeats;
        pt.value = value;
        pt.x = beatsToPixels(timeBeats);
        pt.y = valueToPixelY(value);
        points.push_back(pt);
    }

    // Sort by time (should already be sorted, but just in case)
    std::sort(points.begin(), points.end(),
              [](const PointData& a, const PointData& b) { return a.timeBeats < b.timeBeats; });

    // Draw line from left edge to first point
    if (!points.empty())
    {
        path.startNewSubPath(0.0f, points[0].y);
        path.lineTo(points[0].x, points[0].y);
    }

    // Draw lines between points
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (i == 0)
            path.lineTo(points[i].x, points[i].y);
        else
            path.lineTo(points[i].x, points[i].y);
    }

    // Draw line from last point to right edge (hold value)
    if (!points.empty())
    {
        path.lineTo(static_cast<float>(getWidth()), points.back().y);
    }

    // Draw the path
    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void AutomationLaneComponent::drawControlPoints(juce::Graphics& g)
{
    for (const auto& handle : pointHandles)
    {
        // Check if this point is being dragged
        bool isSelected = (handle.pointId == draggedPointId);

        // Draw circle
        g.setColour(isSelected ? juce::Colour(0xffff9944) : juce::Colour(0xff4a9eff));
        g.fillEllipse(handle.screenPos.x - handle.radius,
                     handle.screenPos.y - handle.radius,
                     handle.radius * 2.0f,
                     handle.radius * 2.0f);

        // Draw outline
        g.setColour(juce::Colours::white);
        g.drawEllipse(handle.screenPos.x - handle.radius,
                     handle.screenPos.y - handle.radius,
                     handle.radius * 2.0f,
                     handle.radius * 2.0f,
                     1.5f);
    }
}

void AutomationLaneComponent::rebuildPointHandles()
{
    pointHandles.clear();

    if (!envelopeNode.isValid())
        return;

    const int numPoints = envelopeNode.getNumChildren();
    for (int i = 0; i < numPoints; ++i)
    {
        auto pointNode = envelopeNode.getChild(i);
        juce::String pointId = pointNode.getProperty(zenith::ProjectState::PROP_ID, "");
        double timeBeats = pointNode.getProperty(zenith::ProjectState::PROP_TIME_BEATS, 0.0);
        double value = pointNode.getProperty(zenith::ProjectState::PROP_VALUE, 0.0);

        PointHandle handle;
        handle.pointId = pointId;
        handle.screenPos = juce::Point<float>(beatsToPixels(timeBeats), valueToPixelY(value));
        pointHandles.push_back(handle);
    }
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void AutomationLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    auto mousePos = e.position;

    // Check if clicking on existing point
    juce::String hitPointId = findPointAtPosition(mousePos);

    if (hitPointId.isNotEmpty())
    {
        // Start dragging existing point
        draggedPointId = hitPointId;
        dragStartMousePos = mousePos;
        isDragging = true;

        // Store original position for potential undo
        if (envelopeNode.isValid())
        {
            for (int i = 0; i < envelopeNode.getNumChildren(); ++i)
            {
                auto pointNode = envelopeNode.getChild(i);
                juce::String pointId = pointNode.getProperty(zenith::ProjectState::PROP_ID, "");
                if (pointId == draggedPointId)
                {
                    dragStartTimeBeats = pointNode.getProperty(zenith::ProjectState::PROP_TIME_BEATS, 0.0);
                    dragStartValue = pointNode.getProperty(zenith::ProjectState::PROP_VALUE, 0.0);
                    break;
                }
            }
        }

        repaint();
    }
    else
    {
        // Add new point at click position
        addPointAt(mousePos);
    }
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || draggedPointId.isEmpty())
        return;

    // Move the dragged point
    movePoint(draggedPointId, e.position);
}

void AutomationLaneComponent::mouseUp(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        isDragging = false;
        draggedPointId = "";
        repaint();
    }
}

void AutomationLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double-click to delete point
    juce::String hitPointId = findPointAtPosition(e.position);
    if (hitPointId.isNotEmpty())
    {
        deletePoint(hitPointId);
    }
}

void AutomationLaneComponent::mouseMove(const juce::MouseEvent& e)
{
    // Update hover state
    juce::String newHoveredPointId = findPointAtPosition(e.position);

    if (newHoveredPointId != hoveredPointId)
    {
        hoveredPointId = newHoveredPointId;

        // Update hover info for tooltip
        if (hoveredPointId.isNotEmpty() && envelopeNode.isValid())
        {
            for (int i = 0; i < envelopeNode.getNumChildren(); ++i)
            {
                auto pointNode = envelopeNode.getChild(i);
                juce::String pointId = pointNode.getProperty(zenith::ProjectState::PROP_ID, "");
                if (pointId == hoveredPointId)
                {
                    hoveredPointTime = pointNode.getProperty(zenith::ProjectState::PROP_TIME_BEATS, 0.0);
                    hoveredPointValue = pointNode.getProperty(zenith::ProjectState::PROP_VALUE, 0.0);
                    hoveredPointScreenPos = juce::Point<float>(beatsToPixels(hoveredPointTime),
                                                                valueToPixelY(hoveredPointValue));
                    break;
                }
            }
        }

        repaint();
    }
}

void AutomationLaneComponent::mouseExit(const juce::MouseEvent& e)
{
    // Clear hover state
    if (hoveredPointId.isNotEmpty())
    {
        hoveredPointId = "";
        repaint();
    }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

float AutomationLaneComponent::beatsToPixels(double timeBeats) const
{
    return static_cast<float>((timeBeats - scrollOffsetBeats) * pixelsPerBeat);
}

double AutomationLaneComponent::pixelsToBeats(float pixelX) const
{
    return (pixelX / pixelsPerBeat) + scrollOffsetBeats;
}

float AutomationLaneComponent::valueToPixelY(double value) const
{
    const float height = static_cast<float>(getHeight());
    // Invert Y axis: value 1.0 (max) should be at top (y=0)
    double normalized = (value - paramInfo.minValue) / (paramInfo.maxValue - paramInfo.minValue);
    normalized = juce::jlimit(0.0, 1.0, normalized);
    return height * (1.0f - static_cast<float>(normalized));
}

double AutomationLaneComponent::pixelYToValue(float pixelY) const
{
    const float height = static_cast<float>(getHeight());
    // Invert Y axis: y=0 (top) should be max value
    float normalized = 1.0f - (pixelY / height);
    normalized = juce::jlimit(0.0f, 1.0f, normalized);

    double value = paramInfo.minValue + normalized * (paramInfo.maxValue - paramInfo.minValue);
    return juce::jlimit(paramInfo.minValue, paramInfo.maxValue, value);
}

//==============================================================================
// Hit Testing
//==============================================================================

juce::String AutomationLaneComponent::findPointAtPosition(juce::Point<float> pos) const
{
    for (const auto& handle : pointHandles)
    {
        if (handle.hitTest(pos))
            return handle.pointId;
    }
    return {};
}

//==============================================================================
// Editing Operations
//==============================================================================

void AutomationLaneComponent::addPointAt(juce::Point<float> pos)
{
    // Convert screen position to automation coordinates
    double timeBeats = pixelsToBeats(pos.x);
    double value = pixelYToValue(pos.y);

    // Snap to grid if enabled
    if (gridSnapEnabled)
        timeBeats = quantizeToGrid(timeBeats);

    // Clamp time to positive values
    timeBeats = juce::jmax(0.0, timeBeats);

    // Add point via ProjectState (with undo)
    projectState.addAutomationPoint(trackId, paramId, timeBeats, value, "Add Automation Point");

    // Repaint will happen via ValueTree listener
}

void AutomationLaneComponent::deletePoint(const juce::String& pointId)
{
    if (pointId.isEmpty())
        return;

    // Delete point via ProjectState (with undo)
    projectState.deleteAutomationPoint(trackId, paramId, pointId, "Delete Automation Point");

    // Repaint will happen via ValueTree listener
}

void AutomationLaneComponent::movePoint(const juce::String& pointId, juce::Point<float> newPos)
{
    if (pointId.isEmpty())
        return;

    // Convert screen position to automation coordinates
    double newTimeBeats = pixelsToBeats(newPos.x);
    double newValue = pixelYToValue(newPos.y);

    // Snap to grid if enabled
    if (gridSnapEnabled)
        newTimeBeats = quantizeToGrid(newTimeBeats);

    // Clamp time to positive values
    newTimeBeats = juce::jmax(0.0, newTimeBeats);

    // Move point via ProjectState (with undo)
    projectState.moveAutomationPoint(trackId, paramId, pointId, newTimeBeats, newValue, "Move Automation Point");

    // Repaint will happen via ValueTree listener
}

//==============================================================================
// Grid Snapping
//==============================================================================

double AutomationLaneComponent::quantizeToGrid(double timeBeats) const
{
    if (!gridSnapEnabled || gridBeats <= 0.0)
        return timeBeats;

    return std::round(timeBeats / gridBeats) * gridBeats;
}

//==============================================================================
// ValueTree Listener
//==============================================================================

void AutomationLaneComponent::valueTreePropertyChanged(juce::ValueTree& tree,
                                                       const juce::Identifier& property)
{
    // Any property change in the envelope or its points -> repaint
    repaint();
}

void AutomationLaneComponent::valueTreeChildAdded(juce::ValueTree& parent,
                                                  juce::ValueTree& child)
{
    // Point added -> repaint
    repaint();
}

void AutomationLaneComponent::valueTreeChildRemoved(juce::ValueTree& parent,
                                                    juce::ValueTree& child,
                                                    int index)
{
    // Point removed -> repaint
    repaint();
}

void AutomationLaneComponent::valueTreeChildOrderChanged(juce::ValueTree& parent,
                                                         int oldIndex,
                                                         int newIndex)
{
    // Point order changed (shouldn't happen, but handle it) -> repaint
    repaint();
}
