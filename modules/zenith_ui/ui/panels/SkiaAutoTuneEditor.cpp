/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "SkiaAutoTuneEditor.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace ui {
namespace panels {

//==============================================================================
// Preset button data
static const char* kPresetNames[5] = {
    "Natural",
    "Transparent",
    "Tight",
    "Robot (T-Pain)",
    "Subtle"
};

static const effects::PresetType kPresetTypes[5] = {
    effects::PresetType::Natural,
    effects::PresetType::Transparent,
    effects::PresetType::Tight,
    effects::PresetType::Robot,
    effects::PresetType::Subtle
};

//==============================================================================
SkiaAutoTuneEditor::SkiaAutoTuneEditor(
    effects::ZenithUltraLowLatencyAutoTune& processor)
    : SkiaComponent(processor)
{
    // Get reference to processor
    autoTune_ = processor;

    // Initialize preset buttons
    for (int i = 0; i < 5; ++i)
    {
        presetButtons_[i].name = kPresetNames[i];
        presetButtons_[i].type = kPresetTypes[i];
        presetButtons_[i].isActive = (i == 0);  // Default to Natural
    }

    // Start with Low latency mode
    currentLatencyMode_ = effects::LatencyMode::Low;
}

SkiaAutoTuneEditor::~SkiaAutoTuneEditor()
{
}

//==============================================================================
void SkiaAutoTuneEditor::drawSkia(SkCanvas* canvas)
{
    auto& context = canvas->getContext();
    SkPaint paint(context);
    SkCanvas* c = paint.getCanvas();

    // Background
    c->clear(SK_ColorBLACK);

    // Get dimensions
    SkScalar w = SkScalar::Float(getWidth());
    SkScalar h = SkScalar::Float(getHeight());

    // Layout sections (vertical stack)
    float yPos = 10.0f;
    float xPos = 10.0f;
    float sectionWidth = w - 20.0f;

    // 1. HEADER - Title + Algorithm Selector
    drawSection(c, xPos, yPos, sectionWidth, LayoutSection::Header);
    yPos = sectionBounds_[static_cast<int>(LayoutSection::Header)].bottom + 15.0f;

    // 2. PITCH VISUALIZATION
    drawSection(c, xPos, yPos, sectionWidth, LayoutSection::Visualization);
    yPos = sectionBounds_[static_cast<int>(LayoutSection::Visualization)].bottom + 15.0f;

    // 3. LATENCY + CONTROLS ROW
    drawSection(c, xPos, yPos, sectionWidth, LayoutSection::Latency);
    yPos = sectionBounds_[static_cast<int>(LayoutSection::Latency)].bottom + 15.0f;

    // 4. KEY/SCALE ROW
    drawSection(c, xPos, yPos, sectionWidth, LayoutSection::ScaleKey);
    yPos = sectionBounds_[static_cast<int>(LayoutSection::ScaleKey)].bottom + 15.0f;

    // 5. FORMANT ROW
    drawSection(c, xPos, yPos, sectionWidth, LayoutSection::Formant);
    yPos = sectionBounds_[static_cast<int>(LayoutSection::Formant)].bottom + 15.0f;

    // 6. PRESETS
    drawSection(c, xPos, yPos, sectionWidth, LayoutSection::Presets);
    yPos = sectionBounds_[static_cast<int>(LayoutSection::Presets)].bottom + 15.0f;

    // 7. ADVANCED TOGGLES
    if (showAdvanced_)
    {
        drawSection(c, xPos, yPos, sectionWidth, LayoutSection::Advanced);
    }
    yPos = sectionBounds_[static_cast<int>(LayoutSection::Advanced)].bottom + 15.0f;

    // 8. FEEDBACK DISPLAY
    drawSection(c, xPos, yPos, sectionWidth, LayoutSection::Feedback);
}

//==============================================================================
void SkiaAutoTuneEditor::resized()
{
    // Recalculate layout based on new size
    float w = SkScalar::Float(getWidth());
    float h = SkScalar::Float(getHeight());

    float yPos = 10.0f;
    float xPos = 10.0f;
    float sectionWidth = w - 20.0f;
    float sectionHeight = (h - 40.0f) / 9.0f;  // Account for margins

    // Header section
    sectionBounds_[static_cast<int>(LayoutSection::Header)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.4f);
    yPos += sectionHeight;

    // Visualization section
    sectionBounds_[static_cast<int>(LayoutSection::Visualization)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.25f);
    yPos += sectionHeight;

    // Latency + Main Controls
    sectionBounds_[static_cast<int>(LayoutSection::Latency)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.2f);
    yPos += sectionHeight;

    // Scale/Key
    sectionBounds_[static_cast<int>(LayoutSection::ScaleKey)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.15f);
    yPos += sectionHeight;

    // Formant
    sectionBounds_[static_cast<int>(LayoutSection::Formant)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.08f);
    yPos += sectionHeight;

    // Presets
    sectionBounds_[static_cast<int>(LayoutSection::Presets)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.12f);
    yPos += sectionHeight;

    // Advanced
    sectionBounds_[static_cast<int>(LayoutSection::Advanced)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.06f);
    yPos += sectionHeight;

    // Feedback
    sectionBounds_[static_cast<int>(LayoutSection::Feedback)] = SkRect::MakeXYWH(xPos, yPos, sectionWidth, sectionHeight * 0.08f);
}

//==============================================================================
void SkiaAutoTuneEditor::mouseDown(const MouseEvent& e)
{
    // Check preset buttons
    for (auto& btn : presetButtons_)
    {
        if (btn.bounds.contains(e.position))
        {
            // Deselect all
            for (auto& b : presetButtons_)
                b.isActive = false;

            // Select this one
            btn.isActive = true;

            // Load preset
            autoTune_.loadPreset(btn.type);
            repaint();
            return;
        }
    }

    // Check advanced toggle
    SkRect advancedBounds = sectionBounds_[static_cast<int>(LayoutSection::Advanced)];
    if (advancedBounds.contains(e.position))
    {
        showAdvanced_ = !showAdvanced_;
        repaint();
    }
}

void SkiaAutoTuneEditor::mouseUp(const MouseEvent& e)
{
    (void)e;
}

void SkiaAutoTuneEditor::mouseDrag(const MouseEvent& e)
{
}

void SkiaAutoTuneEditor::mouseExit(const MouseEvent& e)
{
    (void)e;
}

//==============================================================================
void SkiaAutoTuneEditor::drawSection(
    SkCanvas* canvas, LayoutSection section, const char* title)
{
    // Calculate bounds
    int idx = static_cast<int>(section);
    SkRect bounds = sectionBounds_[idx];

    // Draw section background
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);
    c->clear(SK_ColorBLACK);

    // Section background with subtle gradient
    SkColor bgStart = SkColorSetARGB(40, 40, 45, 255);
    SkColor bgEnd = SkColorSetARGB(30, 30, 35, 255);
    SkPoint points[4] = {bounds.x(), bounds.y(), bounds.right(), bounds.bottom()};
    SkPaint gradient(bgStart, bgEnd, points);

    SkRect bgRect = bounds;
    bgRect.inset(2, 2);
    c->drawRect(bgRect, paint);

    // Section title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setAntiAlias(true);
    titlePaint.setTextSize(18.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style);

    SkString titleStr(title);
    float titleWidth = titlePaint.measureText(titleStr.c_str());
    float titleX = bounds.x() + (bounds.width() - titleWidth) * 0.5f;
    c->drawString(titleStr.c_str(), titleX, bounds.y() + 8, titlePaint);

    // Draw content based on section type
    drawSectionContent(c, section, bounds);
}

//==============================================================================
void SkiaAutoTuneEditor::drawSectionContent(
    SkCanvas* canvas, LayoutSection section, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    switch (section)
    {
    case LayoutSection::Header:
        drawHeaderContent(c, bounds);
        break;

    case LayoutSection::Visualization:
        drawVisualizationContent(c, bounds);
        break;

    case LayoutSection::Latency:
        drawLatencyContent(c, bounds);
        break;

    case LayoutSection::ScaleKey:
        drawScaleKeyContent(c, bounds);
        break;

    case LayoutSection::Formant:
        drawFormantContent(c, bounds);
        break;

    case LayoutSection::Presets:
        drawPresetsContent(c, bounds);
        break;

    case LayoutSection::Advanced:
        drawAdvancedContent(c, bounds);
        break;

    case LayoutSection::Feedback:
        drawFeedbackContent(c, bounds);
        break;
    }
}

//==============================================================================
void SkiaAutoTuneEditor::drawHeaderContent(SkCanvas* canvas, const SkRect& bounds)
{
    // Get current algorithm
    juce::String algoName = autoTune_.getAlgorithmUsed();

    // Title "Zenith Auto-Tune"
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(22.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    SkString title("Zenith Auto-Tune");
    float titleY = bounds.y() + bounds.height() * 0.15f;
    c->drawString(title.c_str(), bounds.x() + 10, titleY, titlePaint);

    // Algorithm selector (simplified - just show current)
    SkPaint algoPaint;
    algoPaint.setColor(SK_ColorLTGRAY);
    algoPaint.setTextSize(14.0f);
    algoPaint.setTypeface(SkTypeface::kSerif_Style);

    SkString algoText("Algorithm: ");
    algoText.append(algoName);
    c->drawString(algoText.c_str(), bounds.x() + 12, titleY + 40, algoPaint);
}

//==============================================================================
void SkiaAutoTuneEditor::drawVisualizationContent(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    // Draw pitch visualization using embedded pitch editor
    // Note: This is a placeholder - the actual pitch editor would draw curves
    SkRect vizBounds = bounds;
    vizBounds.inset(4, 4);
    vizBounds.inset(4, 4);

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(20, 20, 30, 200));
    c->drawRect(vizBounds, bgPaint);

    // Title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(14.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    c->drawString("PITCH VISUALIZATION", vizBounds.x() + 10, vizBounds.y() + 8, titlePaint);

    // Get pitch data
    float detectedPitch = autoTune_.getDetectedPitch();
    float targetPitch = autoTune_.getTargetPitch();
    float confidence = autoTune_.getConfidence();

    // Draw pitch info
    SkPaint infoPaint;
    infoPaint.setColor(SK_ColorLTGRAY);
    infoPaint.setTextSize(12.0f);
    infoPaint.setTypeface(SkTypeface::kSerif_Style);

    char pitchText[64];
    if (detectedPitch > 0.0f)
        snprintf(pitchText, sizeof(pitchText), "Detected: %.1f Hz", detectedPitch);
    else
        snprintf(pitchText, sizeof(pitchText), "Detected: -- Hz");

    float infoY = vizBounds.y() + 30;
    c->drawString(pitchText, vizBounds.x() + 15, infoY, infoPaint);

    if (targetPitch > 0.0f)
        snprintf(pitchText, sizeof(pitchText), "Target: %.1f Hz", targetPitch);
    else
        snprintf(pitchText, sizeof(pitchText), "Target: -- Hz");
    c->drawString(pitchText, vizBounds.x() + 15, infoY + 20, infoPaint);

    char confText[64];
    snprintf(confText, sizeof(confText), "Confidence: %.0f%%", confidence * 100.0f);
    c->drawString(confText, vizBounds.x() + 15, infoY + 35, infoPaint);

    // Note about pitch editor integration
    SkPaint notePaint;
    notePaint.setColor(SK_ColorGRAY);
    notePaint.setTextSize(10.0f);
    c->drawString("(Full pitch editor integration pending)", vizBounds.x() + 10, infoY + 50, notePaint);
}

//==============================================================================
void SkiaAutoTuneEditor::drawLatencyContent(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    // Title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(14.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    c->drawString("LATENCY MODE", bounds.x() + 10, bounds.y() + 8, titlePaint);

    // Current mode
    SkPaint modePaint;
    modePaint.setColor(SK_ColorYELLOW);
    modePaint.setTextSize(16.0f);
    modePaint.setTypeface(SkTypeface::kSerif_Style);

    const char* modeName = "Turbo (0.36ms)";
    if (currentLatencyMode_ == effects::LatencyMode::Turbo)
        modeName = "Turbo (0.36ms)";
    else if (currentLatencyMode_ == effects::LatencyMode::Extreme)
        modeName = "Extreme (0.73ms)";
    else if (currentLatencyMode_ == effects::LatencyMode::UltraLow)
        modeName = "UltraLow (1.45ms)";
    else if (currentLatencyMode_ == effects::LatencyMode::Low)
        modeName = "Low (2.90ms)";
    else if (currentLatencyMode_ == effects::LatencyMode::Standard)
        modeName = "Standard (5.80ms)";
    else if (currentLatencyMode_ == effects::LatencyMode::HighQuality)
        modeName = "HighQuality (11.6ms)";

    c->drawString(modeName, bounds.x() + 10, bounds.y() + 30, modePaint);

    // Total latency display
    float totalMs = autoTune_.getLatencyMs();
    SkPaint totalPaint;
    totalPaint.setColor(SK_ColorWHITE);
    totalPaint.setTextSize(14.0f);
    totalPaint.setTypeface(SkTypeface::kSerif_Style);

    char latencyText[64];
    snprintf(latencyText, sizeof(latencyText), "Total: %.2f ms", totalMs);
    c->drawString(latencyText, bounds.x() + 10, bounds.y() + 50, totalPaint);
}

//==============================================================================
void SkiaAutoTuneEditor::drawScaleKeyContent(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    // Title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(14.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    c->drawString("KEY / SCALE", bounds.x() + 10, bounds.y() + 8, titlePaint);

    // Current key and scale
    int key = autoTune_.getKey();
    int scale = autoTune_.getScale();

    const char* keyNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const char* scaleNames[] = {
        "Chromatic", "Major", "Minor", "Minor Harmonic",
        "Minor Melodic", "Pentatonic Major", "Pentatonic Minor",
        "Blues", "Dorian", "Phrygian", "Lydian", "Mixolydian"
    };

    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setTextSize(16.0f);
    textPaint.setTypeface(SkTypeface::kSerif_Style);

    char keyScaleText[64];
    snprintf(keyScaleText, sizeof(keyScaleText), "%s %s", keyNames[key], scaleNames[scale]);
    c->drawString(keyScaleText, bounds.x() + 10, bounds.y() + 30, textPaint);

    // Click detection area (placeholder for interaction)
    SkRect keyArea = bounds;
    keyArea.inset(10, 30, bounds.width() - 20, bounds.height() - 50);
    SkPaint border;
    border.setColor(SK_ColorWHITE);
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    c->drawRect(keyArea, border);
}

//==============================================================================
void SkiaAutoTuneEditor::drawFormantContent(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    // Title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(14.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    c->drawString("FORMANT PRESERVATION", bounds.x() + 10, bounds.y() + 8, titlePaint);

    // Formant amount slider (0-100%)
    float amount = autoTune_.getFormantPreservation() * 100.0f;

    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setTextSize(12.0f);
    textPaint.setTypeface(SkTypeface::kSerif_Style);

    char amountText[64];
    snprintf(amountText, sizeof(amountText), "Amount: %.0f%%", amount);
    c->drawString(amountText, bounds.x() + 10, bounds.y() + 30, textPaint);

    // Visualization bar
    SkRect barBounds = bounds;
    barBounds.inset(10, 30, bounds.width() - 20, 8.0f);
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(60, 60, 80, 200));
    c->drawRect(barBounds, bgPaint);

    SkPaint fillPaint;
    fillPaint.setColor(SK_ColorWHITE);
    fillPaint.setStyle(SkPaint::kFill_Style);
    SkRect fillRect = barBounds;
    fillRect.inset(11, 30, amount / 100.0f * (bounds.width() - 40.0f), 4.0f);
    c->drawRect(fillRect, fillPaint);
}

//==============================================================================
void SkiaAutoTuneEditor::drawPresetsContent(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    // Title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(14.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    c->drawString("PRESETS", bounds.x() + 10, bounds.y() + 8, titlePaint);

    // Draw preset buttons
    float btnWidth = (bounds.width() - 20.0f) / 5.0f - 6.0f;
    float btnHeight = 30.0f;
    float btnGap = 5.0f;
    float yPos = bounds.y() + 25.0f;
    float xPos = 10.0f;

    for (int i = 0; i < 5; ++i)
    {
        PresetButton& btn = presetButtons_[i];

        // Button background
        SkRect btnBounds(xPos, yPos, btnWidth, btnHeight);
        btnBounds.inset(4, 4);

        SkPaint bgPaint;
        if (btn.isActive)
        {
            // Active preset - blue glow
            bgPaint.setColor(SkColorSetARGB(40, 120, 255, 200));
        }
        else
        {
            // Inactive preset - dark gray
            bgPaint.setColor(SkColorSetARGB(60, 60, 65, 200));
        }
        c->drawRect(btnBounds, bgPaint);

        // Button border
        SkPaint border;
        border.setColor(SK_ColorWHITE);
        border.setStyle(SkPaint::kStroke_Style);
        border.setStrokeWidth(btn.isActive ? 2.0f : 1.0f);
        c->drawRect(btnBounds, border);

        // Preset name
        SkPaint textPaint;
        textPaint.setColor(SK_ColorWHITE);
        textPaint.setTextSize(12.0f);
        textPaint.setTypeface(SkTypeface::kSerif_Style);

        SkString name(btn.name);
        float nameWidth = textPaint.measureText(name.c_str());
        float nameX = xPos + (btnWidth - nameWidth) * 0.5f;
        float nameY = yPos + (btnHeight - 12.0f) * 0.5f;
        c->drawString(name.c_str(), nameX, nameY, textPaint);

        xPos += btnWidth + btnGap;
    }
}

//==============================================================================
void SkiaAutoTuneEditor::drawAdvancedContent(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    // Title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(14.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    c->drawString("ADVANCED OPTIONS", bounds.x() + 10, bounds.y() + 8, titlePaint);

    // Downsampling toggle
    bool downsampleOn = true;  // Would read from processor

    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setTextSize(12.0f);
    textPaint.setTypeface(SkTypeface::kSerif_Style);

    const char* label = downsampleOn ? "Downsampling: ON" : "Downsampling: OFF";
    c->drawString(label, bounds.x() + 10, bounds.y() + 25, textPaint);

    // Pitch prediction toggle
    bool predictionOn = true;

    const char* predLabel = predictionOn ? "Pitch Prediction: ON" : "Pitch Prediction: OFF";
    c->drawString(predLabel, bounds.x() + 10, bounds.y() + 40, textPaint);
}

//==============================================================================
void SkiaAutoTuneEditor::drawFeedbackContent(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    SkCanvas* c = paint.init(canvas);

    // Title
    SkPaint titlePaint;
    titlePaint.setColor(SK_ColorWHITE);
    titlePaint.setTextSize(14.0f);
    titlePaint.setTypeface(SkTypeface::kSerif_Style_Bold);
    c->drawString("REAL-TIME FEEDBACK", bounds.x() + 10, bounds.y() + 8, titlePaint);

    // Is correcting?
    bool isCorrecting = autoTune_.isCorrecting();

    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setTextSize(12.0f);
    textPaint.setTypeface(SkTypeface::kSerif_Style);

    const char* status = isCorrecting ? "Status: CORRECTING" : "Status: BYPASS";
    c->drawString(status, bounds.x() + 10, bounds.y() + 25, textPaint);

    // Current correction (cents)
    float correction = autoTune_.getCurrentCorrection();

    char corrText[64];
    snprintf(corrText, sizeof(corrText), "Correction: %.1f cents", correction);
    c->drawString(corrText, bounds.x() + 10, bounds.y() + 45, textPaint);
}

} // namespace panels
} // namespace ui
} // namespace zenith
