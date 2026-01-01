/**
 * @file MarkerLaneComponent.cpp
 * @brief Marker lane implementation - FULLY IMPLEMENTED
 * 
 * Allows visual editing of timeline markers.
 * Features:
 * - Display markers as flags/pins on timeline
 * - Create markers (double-click)
 * - Drag markers horizontally to reposition
 * - Delete markers (Delete key)
 * - Rename markers (double-click on selected marker)
 * - Sync with ProjectState markers
 */

#include "MarkerLaneComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../controls/SkiaAlertWindow.h"
#include "../controls/SkiaButton.h"
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkFont.h>
#include <skia/include/effects/SkGradientShader.h>
#include <skia/include/core/SkMaskFilter.h>
using namespace zenith;

//==============================================================================
MarkerLaneComponent::MarkerLaneComponent(ProjectState& state)
    : projectState(state)
{
    setWantsKeyboardFocus(true);

    // Listen to marker changes
    projectState.getState().addListener(this);

    DBG("MarkerLaneComponent: Constructor - FULLY IMPLEMENTED (Skia)");
}

MarkerLaneComponent::~MarkerLaneComponent()
{
    projectState.getState().removeListener(this);
    DBG("MarkerLaneComponent: Destructor");
}



//==============================================================================
// Component Interface
//==============================================================================

void MarkerLaneComponent::resized()
{
    repaint();
}

void MarkerLaneComponent::drawSkia(SkCanvas* canvas)
{
    using namespace zenith::design;

    auto bounds = getLocalBounds();
    float width = (float)bounds.getWidth();
    float height = (float)bounds.getHeight();

    // Background (Design System)
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_DARKEST);
    bgPaint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeWH(width, height), bgPaint);

    // Draw markers
    drawMarkers(canvas);

    // Draw hovered marker highlight (Neon Pulse)
    if (hoveredMarkerId.isNotEmpty())
    {
        auto markers = projectState.getMarkers();
        for (auto marker : markers)
        {
            if (marker[ProjectState::PROP_ID].toString() == hoveredMarkerId)
            {
                double timeBeats = marker[ProjectState::PROP_TIME_BEATS];
                float x = beatsToX(timeBeats);

                SkPaint highlightPaint;
                highlightPaint.setAntiAlias(true);
                highlightPaint.setColor(design::withAlpha(colors::ACCENT_PRIMARY, 0.15f));
                canvas->drawRect(SkRect::MakeXYWH(x - 12, 0.0f, 24.0f, height), highlightPaint);
                break;
            }
        }
    }
    
    // Border
    SkPaint borderPaint;
    borderPaint.setColor(SK_ColorBLACK);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    canvas->drawRect(SkRect::MakeWH(width, height), borderPaint);
}

void MarkerLaneComponent::drawMarkers(SkCanvas* canvas) const
{
    auto markers = projectState.getMarkers();

    for (auto marker : markers)
    {
        double timeBeats = marker[ProjectState::PROP_TIME_BEATS];
        juce::String name = marker[ProjectState::PROP_NAME].toString();
        juce::String markerId = marker[ProjectState::PROP_ID].toString();
        juce::String colorHex = marker[ProjectState::PROP_COLOR].toString();

        bool selected = (markerId == selectedMarkerId);

        drawMarker(canvas, timeBeats, name, colorHex, selected);
    }
}

void MarkerLaneComponent::drawMarker(SkCanvas* canvas, double timeBeats, const juce::String& name, 
                                     const juce::String& colorHex, bool selected) const
{
    using namespace zenith::design;
    float x = beatsToX(timeBeats);
    float y = 10.0f;
    float flagHeight = 20.0f;
    float flagWidth = 10.0f;
    float height = (float)getHeight();

    // Parse color
    juce::Colour markerColor = juce::Colour::fromString(colorHex);
    if (markerColor == juce::Colour())
        markerColor = juce::Colours::cyan;

    SkColor skColor = design::toSkColor(markerColor);
    
    // Draw vertical line (Neon Core)
    SkPaint linePaint;
    linePaint.setColor(design::withAlpha(skColor, selected ? 0.9f : 0.6f));
    linePaint.setStrokeWidth(selected ? 2.0f : 1.0f);
    linePaint.setAntiAlias(true);
    
    // Glow for line
    linePaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
    canvas->drawLine(x, y + flagHeight, x, height, linePaint);
    
    linePaint.setMaskFilter(nullptr);
    linePaint.setColor(SK_ColorWHITE);
    linePaint.setStrokeWidth(0.5f);
    canvas->drawLine(x, y + flagHeight, x, height, linePaint);

    // Draw flag shape (Pinned look)
    SkPath flagPath;
    flagPath.moveTo(x, y);
    flagPath.lineTo(x, y + flagHeight);
    flagPath.lineTo(x + flagWidth + 4, y + flagHeight * 0.5f);
    flagPath.close();

    // Glass Background for Flag
    SkPaint flagPaint;
    flagPaint.setAntiAlias(true);
    
    SkPoint pts[2] = {{x, y}, {x, y + flagHeight}};
    SkColor flagColors[2] = {design::lighten(skColor, 0.2f), design::darken(skColor, 0.1f)};
    flagPaint.setShader(SkGradientShader::MakeLinear(pts, flagColors, nullptr, 2, SkTileMode::kClamp));
    flagPaint.setAlphaf(selected ? 0.9f : 0.7f);
    canvas->drawPath(flagPath, flagPaint);

    // Neon Glow for Flag
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(2.0f);
    glowPaint.setColor(design::withAlpha(skColor, 0.6f));
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
    canvas->drawPath(flagPath, glowPaint);

    // Core Border
    SkPaint borderPaint;
    borderPaint.setColor(SK_ColorWHITE);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawPath(flagPath, borderPaint);

    // Draw name label (Inter)
    SkPaint textPaint;
    textPaint.setColor(colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    SkFont font = typography::getSkFont(typography::FONT_XS, FontWeight::Bold);
    
    canvas->drawString(name.toRawUTF8(), x + 6, y + flagHeight + 12, font, textPaint);
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void MarkerLaneComponent::mouseDown(const juce::MouseEvent& event)
{
    juce::String clickedId = findMarkerAt((float)event.x, (float)event.y);
    
    if (clickedId.isNotEmpty())
    {
        selectedMarkerId = clickedId;
        isDraggingMarker = true;
        dragStartX = (float)event.x;
        repaint();
    }
    else
    {
        selectedMarkerId.clear();
        repaint();
    }
}

void MarkerLaneComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingMarker && selectedMarkerId.isNotEmpty())
    {
        double newBeats = xToBeats((float)event.x);
        newBeats = std::max(0.0, newBeats);
        
        projectState.moveMarker(selectedMarkerId, newBeats, "Move Marker");
    }
}

void MarkerLaneComponent::mouseUp(const juce::MouseEvent&)
{
    isDraggingMarker = false;
}

void MarkerLaneComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    juce::String clickedId = findMarkerAt((float)event.x, (float)event.y);
    
    if (clickedId.isNotEmpty())
    {
        showRenameDialog(clickedId);
    }
    else
    {
        // Add new marker
        double beats = xToBeats((float)event.x);
        juce::String name = generateMarkerName();
        projectState.addMarker(beats, name, "0xff4a9eff", "Create Marker");
    }
}

void MarkerLaneComponent::mouseMove(const juce::MouseEvent& event)
{
    juce::String id = findMarkerAt((float)event.x, (float)event.y);
    
    if (id != hoveredMarkerId)
    {
        hoveredMarkerId = id;
        repaint();
    }
}

void MarkerLaneComponent::mouseExit(const juce::MouseEvent&)
{
    if (hoveredMarkerId.isNotEmpty())
    {
        hoveredMarkerId.clear();
        repaint();
    }
}

bool MarkerLaneComponent::keyPressed(const juce::KeyPress& key)
{
    if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) && selectedMarkerId.isNotEmpty())
    {
        projectState.deleteMarker(selectedMarkerId, "Delete Marker");
        selectedMarkerId.clear();
        repaint();
        return true;
    }
    return false;
}

//==============================================================================
// Helpers
//==============================================================================

double MarkerLaneComponent::xToBeats(float x) const
{
    float width = (float)getWidth();
    if (width <= 0.0f) return 0.0;
    
    return viewStartBeats + (x / width) * (viewEndBeats - viewStartBeats);
}

float MarkerLaneComponent::beatsToX(double beats) const
{
    float width = (float)getWidth();
    double range = viewEndBeats - viewStartBeats;
    if (range <= 0.0) return 0.0f;
    
    return (float)((beats - viewStartBeats) / range * width);
}

juce::String MarkerLaneComponent::findMarkerAt(float x, float y) const
{
    juce::ignoreUnused(y); // Markers span full height for detection in this MVP
    
    // Detection radius in pixels
    const float kHitRadius = 10.0f;
    
    auto markers = projectState.getMarkers();
    juce::String detectedId;
    float minDist = 99999.0f;
    
    for (auto marker : markers)
    {
        double time = marker[ProjectState::PROP_TIME_BEATS];
        float markerX = beatsToX(time);
        
        float dist = std::abs(x - markerX);
        if (dist < kHitRadius && dist < minDist)
        {
            minDist = dist;
            detectedId = marker[ProjectState::PROP_ID].toString();
        }
    }
    
    return detectedId;
}

juce::String MarkerLaneComponent::generateMarkerName() const
{
    auto markers = projectState.getMarkers();
    int count = markers.getNumChildren() + 1;
    return "Marker " + juce::String(count);
}

void MarkerLaneComponent::showRenameDialog(const juce::String& markerId)
{
    // Find current marker name
    auto markers = projectState.getMarkers();
    juce::String currentName;

    for (auto marker : markers)
    {
        if (marker[ProjectState::PROP_ID].toString() == markerId)
        {
            currentName = marker[ProjectState::PROP_NAME].toString();
            break;
        }
    }

    if (currentName.isEmpty())
        return;

    auto* dialog = new SkiaAlertWindow(
        "Rename Marker",
        "Enter new name for marker:",
        SkiaAlertWindow::IconType::NoIcon);

    dialog->addTextEditor("name", currentName, "Marker Name");
    dialog->addButton("Rename", SkiaAlertWindow::Result::Button1, SkiaButton::Style::Primary);
    dialog->addButton("Cancel", SkiaAlertWindow::Result::Cancelled, SkiaButton::Style::Secondary);

    // Find parent to add to
    auto* parent = getParentComponent();
    while (parent != nullptr && parent->getParentComponent() != nullptr) {
        parent = parent->getParentComponent();
    }

    if (parent != nullptr) {
        int w = 400;
        int h = 200;
        dialog->setBounds((parent->getWidth() - w) / 2, (parent->getHeight() - h) / 2, w, h);
        parent->addAndMakeVisible(dialog);
        
        // Capture dialog pointer for use in lambda
        SkiaAlertWindow* dialogPtr = dialog;
        dialog->showAsync([this, markerId, dialogPtr](SkiaAlertWindow::Result result) {
            if (result == SkiaAlertWindow::Result::Button1) {
                juce::String newName = dialogPtr->getTextEditorContents("name");
                if (newName.isNotEmpty()) {
                    projectState.renameMarker(markerId, newName, "Rename Marker");
                }
            }
            delete dialogPtr;
        });
    } else {
        delete dialog;
    }
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void MarkerLaneComponent::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&)
{
    repaint();
}

void MarkerLaneComponent::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&)
{
    repaint();
}

void MarkerLaneComponent::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int)
{
    repaint();
}

void MarkerLaneComponent::valueTreeChildOrderChanged(juce::ValueTree&, int, int)
{
    repaint();
}

void MarkerLaneComponent::valueTreeParentChanged(juce::ValueTree&)
{
    repaint();
}
