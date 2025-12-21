/*
  ==============================================================================
    QuantizeSettingsComponent.h
    Created: 7 Dec 2025
    Description: Pure Skia-rendered UI for configuring quantization settings.

    This is a custom Skia component - NO JUCE widgets used.
    All controls are rendered and interacted with via Skia.
  ==============================================================================
*/

#pragma once
#include "skia/SkiaComponent.h"
#include "skia/ZenithDesignSystem.h"
#include "ui/PianoRollComponent.h"
#include <JuceHeader.h>

class QuantizeSettingsComponent : public zenith::SkiaComponent {
public:
  QuantizeSettingsComponent(PianoRollComponent &owner,
                            PianoRollComponent::QuantizeOptions &options)
      : owner(owner), options(options) {
    setSize(280, 260);
    setWantsKeyboardFocus(true);

    // Initialize grid options
    gridOptions = {"Current", "1/4", "1/8", "1/16", "1/32"};
    gridValues = {0.0, 1.0, 0.5, 0.25, 0.125};

    // Find initial grid selection
    selectedGridIndex = 0;
    for (size_t i = 1; i < gridValues.size(); ++i) {
      if (std::abs(options.gridSize - gridValues[i]) < 0.001) {
        selectedGridIndex = (int)i;
        break;
      }
    }
  }

  void drawSkia(SkCanvas *canvas) override {
    using namespace zenith::design;

    auto bounds = getLocalBounds().toFloat();

    // Background with glassmorphism effect
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_DARKER);
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                          dimensions::RADIUS_MD, dimensions::RADIUS_MD,
                          bgPaint);

    // Subtle border
    SkPaint borderPaint;
    borderPaint.setColor(colors::BORDER_DEFAULT);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                          dimensions::RADIUS_MD, dimensions::RADIUS_MD,
                          borderPaint);

    // Title
    SkFont titleFont =
        design::getSkFont(typography::FONT_LG, design::FontWeight::Bold);
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
    drawToggleButton(canvas, tripletsBounds, "Triplets", options.useTriplets,
                     tripletsHovered);

    y += 36;

    // Swing slider row
    drawLabel(canvas, "Swing:", spacing::MD, y);
    swingSliderBounds =
        juce::Rectangle<float>(controlX, y - 10, controlWidth, 20);
    drawSlider(canvas, swingSliderBounds, options.swingAmount, swingDragging,
               swingHovered);

    y += 36;

    // Strength slider row
    drawLabel(canvas, "Amount:", spacing::MD, y);
    strengthSliderBounds =
        juce::Rectangle<float>(controlX, y - 10, controlWidth, 20);
    drawSlider(canvas, strengthSliderBounds, options.strength, strengthDragging,
               strengthHovered);

    y += 36;

    // Start/End toggles row
    drawLabel(canvas, "Affect:", spacing::MD, y);
    startToggleBounds =
        juce::Rectangle<float>(controlX, y - 16, controlWidth * 0.45f, 24);
    drawToggleButton(canvas, startToggleBounds, "Start", options.quantizeStart,
                     startHovered);

    endToggleBounds = juce::Rectangle<float>(controlX + controlWidth * 0.55f,
                                             y - 16, controlWidth * 0.45f, 24);
    drawToggleButton(canvas, endToggleBounds, "End", options.quantizeEnd,
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

private:
  PianoRollComponent &owner;
  PianoRollComponent::QuantizeOptions &options;

  // Control bounds
  juce::Rectangle<float> gridDropdownBounds;
  juce::Rectangle<float> tripletsBounds;
  juce::Rectangle<float> swingSliderBounds;
  juce::Rectangle<float> strengthSliderBounds;
  juce::Rectangle<float> startToggleBounds;
  juce::Rectangle<float> endToggleBounds;
  juce::Rectangle<float> applyButtonBounds;

  // State
  std::vector<juce::String> gridOptions;
  std::vector<double> gridValues;
  int selectedGridIndex = 0;
  bool gridDropdownOpen = false;
  bool gridDropdownHovered = false;
  bool tripletsHovered = false;
  bool swingHovered = false;
  bool swingDragging = false;
  bool strengthHovered = false;
  bool strengthDragging = false;
  bool startHovered = false;
  bool endHovered = false;
  bool applyHovered = false;

  // Drawing helpers
  void drawLabel(SkCanvas *canvas, const char *text, float x, float y) {
    using namespace zenith::design;
    SkFont font = design::getSkFont(typography::FONT_SM);
    SkPaint paint;
    paint.setColor(colors::TEXT_SECONDARY);
    paint.setAntiAlias(true);
    canvas->drawString(text, x, y, font, paint);
  }

  void drawSlider(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                  float value, bool dragging, bool hovered) {
    using namespace zenith::design;

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
    SkFont font = design::getMonoFont(typography::FONT_XS);
    SkPaint textPaint;
    textPaint.setColor(colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);
    juce::String valueStr = juce::String((int)(value * 100)) + "%";
    canvas->drawString(valueStr.toRawUTF8(), bounds.getRight() + 4,
                       bounds.getCentreY() + 4, font, textPaint);
  }

  void drawToggleButton(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                        const char *label, bool active, bool hovered) {
    using namespace zenith::design;

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
    SkFont font = design::getSkFont(typography::FONT_XS);
    SkPaint textPaint;
    textPaint.setColor(active ? colors::BG_DARKEST : colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    float textWidth =
        font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, bounds.getCentreX() - textWidth / 2,
                       bounds.getCentreY() + 4, font, textPaint);
  }

  void drawDropdown(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                    const juce::String &text, bool open, bool hovered) {
    using namespace zenith::design;

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
    SkFont font = design::getSkFont(typography::FONT_SM);
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

  void drawDropdownMenu(SkCanvas *canvas,
                        const juce::Rectangle<float> &bounds) {
    using namespace zenith::design;

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
    SkFont font = design::getSkFont(typography::FONT_SM);

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

  void drawButton(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                  const char *label, bool hovered) {
    using namespace zenith::design;

    // Gradient-ish button
    SkPaint bgPaint;
    bgPaint.setColor(hovered ? colors::CYAN : colors::NEON_GREEN);
    bgPaint.setAntiAlias(true);
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                   bounds.getWidth(), bounds.getHeight());
    canvas->drawRoundRect(rect, dimensions::RADIUS_MD, dimensions::RADIUS_MD,
                          bgPaint);

    // Text
    SkFont font =
        design::getSkFont(typography::FONT_MD, design::FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(colors::BG_DARKEST);
    textPaint.setAntiAlias(true);
    float textWidth =
        font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, bounds.getCentreX() - textWidth / 2,
                       bounds.getCentreY() + 5, font, textPaint);
  }

  int hoveredDropdownIndex = -1;

  // Mouse handling
  void mouseMove(const juce::MouseEvent &e) override {
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

  void mouseDown(const juce::MouseEvent &e) override {
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
            options.gridSize = gridValues[selectedGridIndex];
          }
          owner.quantizeSelected(options); // Live preview
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
      options.useTriplets = !options.useTriplets;
      owner.quantizeSelected(options);
      repaint();
      return;
    }

    // Start toggle
    if (startToggleBounds.contains(x, y)) {
      options.quantizeStart = !options.quantizeStart;
      owner.quantizeSelected(options);
      repaint();
      return;
    }

    // End toggle
    if (endToggleBounds.contains(x, y)) {
      options.quantizeEnd = !options.quantizeEnd;
      owner.quantizeSelected(options);
      repaint();
      return;
    }

    // Swing slider
    if (swingSliderBounds.expanded(0, 8).contains(x, y)) {
      swingDragging = true;
      updateSliderValue(swingSliderBounds, x, options.swingAmount);
      owner.quantizeSelected(options);
      repaint();
      return;
    }

    // Strength slider
    if (strengthSliderBounds.expanded(0, 8).contains(x, y)) {
      strengthDragging = true;
      updateSliderValue(strengthSliderBounds, x, options.strength);
      owner.quantizeSelected(options);
      repaint();
      return;
    }

    // Apply button
    if (applyButtonBounds.contains(x, y)) {
      owner.quantizeSelected(options);
      if (auto *callout = findParentComponentOfClass<juce::CallOutBox>())
        callout->dismiss();
      return;
    }
  }

  void mouseDrag(const juce::MouseEvent &e) override {
    float x = e.position.x;

    if (swingDragging) {
      updateSliderValue(swingSliderBounds, x, options.swingAmount);
      owner.quantizeSelected(options);
      repaint();
    }

    if (strengthDragging) {
      updateSliderValue(strengthSliderBounds, x, options.strength);
      owner.quantizeSelected(options);
      repaint();
    }
  }

  void mouseUp(const juce::MouseEvent &) override {
    swingDragging = false;
    strengthDragging = false;
    repaint();
  }

  void updateSliderValue(const juce::Rectangle<float> &bounds, float mouseX,
                         float &value) {
    float normalized = (mouseX - bounds.getX()) / bounds.getWidth();
    value = juce::jlimit(0.0f, 1.0f, normalized);
  }

  void mouseExit(const juce::MouseEvent &) override {
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
};
