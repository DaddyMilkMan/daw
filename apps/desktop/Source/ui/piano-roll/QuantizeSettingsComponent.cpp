/*
  ==============================================================================
    QuantizeSettingsComponent.cpp
    Created: 25 Dec 2025
    Author:  Zenith DAW
  ==============================================================================
*/

#include "QuantizeSettingsComponent.h"
#include "../design-system/ColorBridge.h"

namespace zenith {

QuantizeSettingsComponent::QuantizeSettingsComponent(PianoRollComponent& owner,
                                                     PianoRollComponent::QuantizeOptions& options)
    : owner_(owner), options_(options) {
    setSize(280, 260);
    setWantsKeyboardFocus(true);

    // Initialize grid options
    gridOptions = {"Current", "1/4", "1/8", "1/16", "1/32"};
    gridValues = {0.0, 1.0, 0.5, 0.25, 0.125};

    // Find initial grid selection
    selectedGridIndex = 0;
    for (size_t i = 1; i < gridValues.size(); ++i) {
        if (std::abs(options_.gridSize - gridValues[i]) < 0.001) {
            selectedGridIndex = (int)i;
            break;
        }
    }
}

void QuantizeSettingsComponent::drawSkia(SkCanvas* canvas) {
    using namespace design;

    auto bounds = getLocalBounds().toFloat();

    // Background with glassmorphism effect
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_DARKER);
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                          dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          bgPaint);

    // Subtle border
    SkPaint borderPaint;
    borderPaint.setColor(colors::BORDER_DEFAULT);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                          dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          borderPaint);

    // Title
    SkFont titleFont =
        getSkFont(typography::FONT_LG, FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(colors::CYAN);
    titlePaint.setAntiAlias(true);
    canvas->drawString("QUANTIZE", spacing::MD,
                       spacing::MD + typography::FONT_LG, titleFont,
                       titlePaint);

    float y = spacing::MD + typography::FONT_LG + spacing::LG;
    float labelWidth = 70.0f;
    float controlX = spacing::MD + labelWidth;
    float controlWidth = bounds.getWidth() - controlX - spacing::MD;

    // Grid selector row
    drawLabel(canvas, "Grid:", spacing::MD, y);
    gridDropdownBounds =
        juce::Rectangle<float>(controlX, y - 16, controlWidth * 0.6f, 24);
    drawDropdown(canvas, gridDropdownBounds, gridOptions[selectedGridIndex],
                 gridDropdownOpen, gridDropdownHovered);

    // Triplets toggle
    tripletsBounds = juce::Rectangle<float>(controlX + controlWidth * 0.65f,
                                            y - 16, controlWidth * 0.35f, 24);
    drawToggleButton(canvas, tripletsBounds, "Triplets", options_.useTriplets,
                     tripletsHovered);

    y += 36;

    // Swing slider row
    drawLabel(canvas, "Swing:", spacing::MD, y);
    swingSliderBounds =
        juce::Rectangle<float>(controlX, y - 10, controlWidth, 20);
    drawSlider(canvas, swingSliderBounds, options_.swingAmount, swingDragging,
               swingHovered);

    y += 36;

    // Strength slider row
    drawLabel(canvas, "Amount:", spacing::MD, y);
    strengthSliderBounds =
        juce::Rectangle<float>(controlX, y - 10, controlWidth, 20);
    drawSlider(canvas, strengthSliderBounds, options_.strength, strengthDragging,
               strengthHovered);

    y += 36;

    // Start/End toggles row
    drawLabel(canvas, "Affect:", spacing::MD, y);
    startToggleBounds =
        juce::Rectangle<float>(controlX, y - 16, controlWidth * 0.45f, 24);
    drawToggleButton(canvas, startToggleBounds, "Start", options_.quantizeStart,
                     startHovered);

    endToggleBounds = juce::Rectangle<float>(controlX + controlWidth * 0.55f,
                                             y - 16, controlWidth * 0.45f, 24);
    drawToggleButton(canvas, endToggleBounds, "End", options_.quantizeEnd,
                     endHovered);

    y += 40;

    // Apply button
    applyButtonBounds = juce::Rectangle<float>(
        spacing::MD, y, bounds.getWidth() - spacing::MD * 2, 32);
    drawButton(canvas, applyButtonBounds, "APPLY", applyHovered);

    // Dropdown menu (if open)
    if (gridDropdownOpen) {
        drawDropdownMenu(
            canvas, gridDropdownBounds.withY(gridDropdownBounds.getBottom()));
    }
}

void QuantizeSettingsComponent::drawLabel(SkCanvas* canvas, const char* text, float x, float y) {
    using namespace design;
    SkFont font = getSkFont(typography::FONT_SM);
    SkPaint paint;
    paint.setColor(colors::TEXT_SECONDARY);
    paint.setAntiAlias(true);
    canvas->drawString(text, x, y, font, paint);
}

void QuantizeSettingsComponent::drawSlider(SkCanvas* canvas, const juce::Rectangle<float>& bounds,
                                          float value, bool dragging, bool hovered) {
    using namespace design;

    // Track
    SkPaint trackPaint;
    trackPaint.setColor(colors::BG_DARK);
    trackPaint.setAntiAlias(true);
    SkRect trackRect = SkRect::MakeXYWH(bounds.getX(), bounds.getCentreY() - 3,
                                        bounds.getWidth(), 6);
    canvas->drawRoundRect(trackRect, 3, 3, trackPaint);

    // Fill
    float fillWidth = value * bounds.getWidth();
    SkPaint fillPaint;
    fillPaint.setColor(hovered || dragging ? colors::CYAN : colors::NEON_GREEN);
    fillPaint.setAntiAlias(true);
    SkRect fillRect =
        SkRect::MakeXYWH(bounds.getX(), bounds.getCentreY() - 3, fillWidth, 6);
    canvas->drawRoundRect(fillRect, 3, 3, fillPaint);

    // Thumb
    float thumbX = bounds.getX() + fillWidth;
    SkPaint thumbPaint;
    thumbPaint.setColor(dragging
                            ? SK_ColorWHITE
                            : (hovered ? colors::CYAN : colors::TEXT_PRIMARY));
    thumbPaint.setAntiAlias(true);
    canvas->drawCircle(thumbX, bounds.getCentreY(), dragging ? 8 : 6,
                       thumbPaint);

    // Value text
    SkFont font = getMonoFont(typography::FONT_XS);
    SkPaint textPaint;
    textPaint.setColor(colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);
    juce::String valueStr = juce::String((int)(value * 100)) + "%";
    canvas->drawString(valueStr.toRawUTF8(), bounds.getRight() + 4,
                       bounds.getCentreY() + 4, font, textPaint);
}

void QuantizeSettingsComponent::drawToggleButton(SkCanvas* canvas, const juce::Rectangle<float>& bounds,
                                                const char* label, bool active, bool hovered) {
    using namespace design;

    SkPaint bgPaint;
    bgPaint.setColor(active ? colors::NEON_GREEN
                            : (hovered ? colors::BG_MEDIUM : colors::BG_DARK));
    bgPaint.setAntiAlias(true);
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                   bounds.getWidth(), bounds.getHeight());
    canvas->drawRoundRect(rect, dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setColor(active ? colors::NEON_GREEN : colors::BORDER_DEFAULT);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(rect, dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          borderPaint);

    // Text
    SkFont font = getSkFont(typography::FONT_XS);
    SkPaint textPaint;
    textPaint.setColor(active ? colors::BG_DARKEST : colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    float textWidth =
        font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, bounds.getCentreX() - textWidth / 2,
                       bounds.getCentreY() + 4, font, textPaint);
}

void QuantizeSettingsComponent::drawDropdown(SkCanvas* canvas, const juce::Rectangle<float>& bounds,
                                            const juce::String& text, bool open, bool hovered) {
    using namespace design;

    SkPaint bgPaint;
    bgPaint.setColor(hovered || open ? colors::BG_MEDIUM : colors::BG_DARK);
    bgPaint.setAntiAlias(true);
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                   bounds.getWidth(), bounds.getHeight());
    canvas->drawRoundRect(rect, dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setColor(open ? colors::CYAN : colors::BORDER_DEFAULT);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(rect, dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          borderPaint);

    // Text
    SkFont font = getSkFont(typography::FONT_SM);
    SkPaint textPaint;
    textPaint.setColor(colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    canvas->drawString(text.toRawUTF8(), bounds.getX() + 8,
                       bounds.getCentreY() + 4, font, textPaint);

    // Arrow
    SkPath arrow;
    float arrowX = bounds.getRight() - 16;
    float arrowY = bounds.getCentreY();
    arrow.moveTo(arrowX - 4, arrowY - 2);
    arrow.lineTo(arrowX, arrowY + 3);
    arrow.lineTo(arrowX + 4, arrowY - 2);
    SkPaint arrowPaint;
    arrowPaint.setColor(colors::TEXT_SECONDARY);
    arrowPaint.setStyle(SkPaint::kStroke_Style);
    arrowPaint.setStrokeWidth(1.5f);
    arrowPaint.setAntiAlias(true);
    canvas->drawPath(arrow, arrowPaint);
}

void QuantizeSettingsComponent::drawDropdownMenu(SkCanvas* canvas,
                                                const juce::Rectangle<float>& bounds) {
    using namespace design;

    float itemHeight = 24;
    float menuHeight = itemHeight * gridOptions.size();

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_DARK);
    bgPaint.setAntiAlias(true);
    SkRect menuRect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                       bounds.getWidth(), menuHeight);
    canvas->drawRoundRect(menuRect, dimensions::RADIUS_SM,
                          dimensions::RADIUS_SM, bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setColor(colors::CYAN);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(menuRect, dimensions::RADIUS_SM,
                          dimensions::RADIUS_SM, borderPaint);

    // Items
    SkFont font = getSkFont(typography::FONT_SM);

    for (size_t i = 0; i < gridOptions.size(); ++i) {
        float y = bounds.getY() + i * itemHeight;
        bool isHovered = (hoveredDropdownIndex == (int)i);
        bool isSelected = (selectedGridIndex == (int)i);

        if (isHovered || isSelected) {
            SkPaint highlightPaint;
            highlightPaint.setColor(isSelected ? colors::NEON_GREEN
                                               : colors::BG_MEDIUM);
            highlightPaint.setAntiAlias(true);
            canvas->drawRect(SkRect::MakeXYWH(bounds.getX() + 2, y + 2,
                                              bounds.getWidth() - 4,
                                              itemHeight - 4),
                             highlightPaint);
        }

        SkPaint textPaint;
        textPaint.setColor(isSelected ? colors::BG_DARKEST
                                      : colors::TEXT_PRIMARY);
        textPaint.setAntiAlias(true);
        canvas->drawString(gridOptions[i].toRawUTF8(), bounds.getX() + 8,
                           y + itemHeight / 2 + 4, font, textPaint);
    }
}

void QuantizeSettingsComponent::drawButton(SkCanvas* canvas, const juce::Rectangle<float>& bounds,
                                          const char* label, bool hovered) {
    using namespace design;

    // Gradient-ish button
    SkPaint bgPaint;
    bgPaint.setColor(hovered ? colors::CYAN : colors::NEON_GREEN);
    bgPaint.setAntiAlias(true);
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                   bounds.getWidth(), bounds.getHeight());
    canvas->drawRoundRect(rect, dimensions::RADIUS_SM, dimensions::RADIUS_SM,
                          bgPaint);

    // Text
    SkFont font =
        getSkFont(typography::FONT_MD, FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(colors::BG_DARKEST);
    textPaint.setAntiAlias(true);
    float textWidth =
        font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, bounds.getCentreX() - textWidth / 2,
                       bounds.getCentreY() + 5, font, textPaint);
}

void QuantizeSettingsComponent::mouseMove(const juce::MouseEvent& e) {
    float x = e.position.x;
    float y = e.position.y;

    gridDropdownHovered = gridDropdownBounds.contains(x, y);
    tripletsHovered = tripletsBounds.contains(x, y);
    swingHovered = swingSliderBounds.expanded(0, 4).contains(x, y);
    strengthHovered = strengthSliderBounds.expanded(0, 4).contains(x, y);
    startHovered = startToggleBounds.contains(x, y);
    endHovered = endToggleBounds.contains(x, y);
    applyHovered = applyButtonBounds.contains(x, y);

    // Dropdown menu hover
    if (gridDropdownOpen) {
        float menuTop = gridDropdownBounds.getBottom();
        float itemHeight = 24;
        if (x >= gridDropdownBounds.getX() &&
            x <= gridDropdownBounds.getRight() && y >= menuTop &&
            y < menuTop + itemHeight * gridOptions.size()) {
            hoveredDropdownIndex = (int)((y - menuTop) / itemHeight);
        } else {
            hoveredDropdownIndex = -1;
        }
    }

    repaint();
}

void QuantizeSettingsComponent::mouseDown(const juce::MouseEvent& e) {
    float x = e.position.x;
    float y = e.position.y;

    // Dropdown selection
    if (gridDropdownOpen) {
        float menuTop = gridDropdownBounds.getBottom();
        float itemHeight = 24;
        if (x >= gridDropdownBounds.getX() &&
            x <= gridDropdownBounds.getRight() && y >= menuTop &&
            y < menuTop + gridOptions.size() * itemHeight) {
            int index = (int)((y - menuTop) / itemHeight);
            if (index >= 0 && index < (int)gridOptions.size()) {
                selectedGridIndex = index;
                if (selectedGridIndex > 0) {
                    options_.gridSize = gridValues[selectedGridIndex];
                }
                owner_.quantizeSelected(options_); // Live preview
            }
        }
        gridDropdownOpen = false;
        repaint();
        return;
    }

    // Toggle dropdown
    if (gridDropdownBounds.contains(x, y)) {
        gridDropdownOpen = !gridDropdownOpen;
        repaint();
        return;
    }

    // Triplets toggle
    if (tripletsBounds.contains(x, y)) {
        options_.useTriplets = !options_.useTriplets;
        owner_.quantizeSelected(options_);
        repaint();
        return;
    }

    // Start toggle
    if (startToggleBounds.contains(x, y)) {
        options_.quantizeStart = !options_.quantizeStart;
        owner_.quantizeSelected(options_);
        repaint();
        return;
    }

    // End toggle
    if (endToggleBounds.contains(x, y)) {
        options_.quantizeEnd = !options_.quantizeEnd;
        owner_.quantizeSelected(options_);
        repaint();
        return;
    }

    // Swing slider
    if (swingSliderBounds.expanded(0, 8).contains(x, y)) {
        swingDragging = true;
        updateSliderValue(swingSliderBounds, x, options_.swingAmount);
        owner_.quantizeSelected(options_);
        repaint();
        return;
    }

    // Strength slider
    if (strengthSliderBounds.expanded(0, 8).contains(x, y)) {
        strengthDragging = true;
        updateSliderValue(strengthSliderBounds, x, options_.strength);
        owner_.quantizeSelected(options_);
        repaint();
        return;
    }

    // Apply button
    if (applyButtonBounds.contains(x, y)) {
        owner_.quantizeSelected(options_);
        setVisible(false);
        if (auto* parent = getParentComponent()) {
            parent->removeChildComponent(this);
        }
        return;
    }
}

void QuantizeSettingsComponent::mouseDrag(const juce::MouseEvent& e) {
    float x = e.position.x;

    if (swingDragging) {
        updateSliderValue(swingSliderBounds, x, options_.swingAmount);
        owner_.quantizeSelected(options_);
        repaint();
    }

    if (strengthDragging) {
        updateSliderValue(strengthSliderBounds, x, options_.strength);
        owner_.quantizeSelected(options_);
        repaint();
    }
}

void QuantizeSettingsComponent::mouseUp(const juce::MouseEvent&) {
    swingDragging = false;
    strengthDragging = false;
    repaint();
}

void QuantizeSettingsComponent::updateSliderValue(const juce::Rectangle<float>& bounds, float mouseX,
                                               float& value) {
    float normalized = (mouseX - bounds.getX()) / bounds.getWidth();
    value = juce::jlimit(0.0f, 1.0f, normalized);
}

void QuantizeSettingsComponent::mouseExit(const juce::MouseEvent&) {
    gridDropdownHovered = false;
    tripletsHovered = false;
    swingHovered = false;
    strengthHovered = false;
    startHovered = false;
    endHovered = false;
    applyHovered = false;
    hoveredDropdownIndex = -1;
    repaint();
}

} // namespace zenith
