/*
  ==============================================================================

    ExportDialog.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ExportDialog.h"
#include "../../../include/CommandAPI.h"

namespace zenith {

ExportDialog::ExportDialog(CommandAPI& commandAPI)
    : commandAPI_(commandAPI)
{
    // Format Buttons
    btnWav_ = std::make_unique<SkiaButton>("WAV");
    btnWav_->setToggleable(true);
    btnWav_->onClick = [this]() { selectedFormat_ = "wav"; updateButtonStates(); };
    addAndMakeVisible(btnWav_.get());

    btnFlac_ = std::make_unique<SkiaButton>("FLAC");
    btnFlac_->setToggleable(true);
    btnFlac_->onClick = [this]() { selectedFormat_ = "flac"; updateButtonStates(); };
    addAndMakeVisible(btnFlac_.get());

    btnOgg_ = std::make_unique<SkiaButton>("OGG");
    btnOgg_->setToggleable(true);
    btnOgg_->onClick = [this]() { selectedFormat_ = "ogg"; updateButtonStates(); };
    addAndMakeVisible(btnOgg_.get());

    // Bit Depth Buttons
    btn8Bit_ = std::make_unique<SkiaButton>("8-bit");
    btn8Bit_->setToggleable(true);
    btn8Bit_->onClick = [this]() { selectedBitDepth_ = 8; updateButtonStates(); };
    addAndMakeVisible(btn8Bit_.get());

    btn16Bit_ = std::make_unique<SkiaButton>("16-bit");
    btn16Bit_->setToggleable(true);
    btn16Bit_->onClick = [this]() { selectedBitDepth_ = 16; updateButtonStates(); };
    addAndMakeVisible(btn16Bit_.get());

    btn24Bit_ = std::make_unique<SkiaButton>("24-bit");
    btn24Bit_->setToggleable(true);
    btn24Bit_->onClick = [this]() { selectedBitDepth_ = 24; updateButtonStates(); };
    addAndMakeVisible(btn24Bit_.get());

    btn32Bit_ = std::make_unique<SkiaButton>("32-bit Float");
    btn32Bit_->setToggleable(true);
    btn32Bit_->onClick = [this]() { selectedBitDepth_ = 32; updateButtonStates(); };
    addAndMakeVisible(btn32Bit_.get());

    // Options
    toggleDither_ = std::make_unique<SkiaButton>("Dither");
    toggleDither_->setToggleable(true);
    toggleDither_->setToggleState(true); // Default on
    addAndMakeVisible(toggleDither_.get());

    toggleNormalize_ = std::make_unique<SkiaButton>("Normalize");
    toggleNormalize_->setToggleable(true);
    addAndMakeVisible(toggleNormalize_.get());

    toggleAIEnhance_ = std::make_unique<SkiaButton>("AI Enhance");
    toggleAIEnhance_->setToggleable(true);
    toggleAIEnhance_->setStyle(SkiaButton::Style::Success); // Highlight AI
    addAndMakeVisible(toggleAIEnhance_.get());

    // Actions
    btnExport_ = std::make_unique<SkiaButton>("EXPORT");
    btnExport_->setStyle(SkiaButton::Style::Primary);
    btnExport_->onClick = [this]() { triggerExport(); };
    addAndMakeVisible(btnExport_.get());

    btnCancel_ = std::make_unique<SkiaButton>("Cancel");
    btnCancel_->setStyle(SkiaButton::Style::Ghost);
    btnCancel_->onClick = [this]() { setVisible(false); }; // Just hide for now
    addAndMakeVisible(btnCancel_.get());

    updateButtonStates();
    setSize(500, 450);
}

ExportDialog::~ExportDialog() {}

void ExportDialog::updateButtonStates()
{
    btnWav_->setToggleState(selectedFormat_ == "wav");
    btnFlac_->setToggleState(selectedFormat_ == "flac");
    btnOgg_->setToggleState(selectedFormat_ == "ogg");

    btn8Bit_->setToggleState(selectedBitDepth_ == 8);
    btn16Bit_->setToggleState(selectedBitDepth_ == 16);
    btn24Bit_->setToggleState(selectedBitDepth_ == 24);
    btn32Bit_->setToggleState(selectedBitDepth_ == 32);
}

void ExportDialog::resized()
{
    auto area = getLocalBounds().reduced(30);
    int buttonHeight = 32;
    int gap = 10;

    // Title area (handled in drawSkia)
    area.removeFromTop(40);

    // Format
    area.removeFromTop(20); // Label space
    auto formatRow = area.removeFromTop(buttonHeight);
    int w = (formatRow.getWidth() - 2 * gap) / 3;
    btnWav_->setBounds(formatRow.removeFromLeft(w));
    formatRow.removeFromLeft(gap);
    btnFlac_->setBounds(formatRow.removeFromLeft(w));
    formatRow.removeFromLeft(gap);
    btnOgg_->setBounds(formatRow);

    area.removeFromTop(gap * 2);

    // Bit Depth
    area.removeFromTop(20); // Label space
    auto depthRow = area.removeFromTop(buttonHeight);
    w = (depthRow.getWidth() - 3 * gap) / 4;
    btn8Bit_->setBounds(depthRow.removeFromLeft(w));
    depthRow.removeFromLeft(gap);
    btn16Bit_->setBounds(depthRow.removeFromLeft(w));
    depthRow.removeFromLeft(gap);
    btn24Bit_->setBounds(depthRow.removeFromLeft(w));
    depthRow.removeFromLeft(gap);
    btn32Bit_->setBounds(depthRow);

    area.removeFromTop(gap * 2);

    // Options
    area.removeFromTop(20); // Label space
    toggleDither_->setBounds(area.removeFromTop(buttonHeight));
    area.removeFromTop(gap);
    toggleNormalize_->setBounds(area.removeFromTop(buttonHeight));
    area.removeFromTop(gap);
    toggleAIEnhance_->setBounds(area.removeFromTop(buttonHeight));

    // Actions (Bottom)
    auto actionRow = getLocalBounds().reduced(30).removeFromBottom(40);
    btnCancel_->setBounds(actionRow.removeFromLeft(100));
    btnExport_->setBounds(actionRow.removeFromRight(120));
}

void ExportDialog::drawSkia(SkCanvas* canvas)
{
    // Draw Glass Background
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    SkRRect rrect = SkRRect::MakeRectXY(rect, 16.0f, 16.0f);

    SkPaint paint;
    paint.setAntiAlias(true);

    // Dark Glass
    paint.setColor(SkColorSetARGB(245, 15, 15, 20));
    canvas->drawRRect(rrect, paint);

    // Border
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetARGB(50, 255, 255, 255));
    canvas->drawRRect(rrect, paint);

    // Title
    SkFont font;
    font.setSize(24.0f);
    font.setSubpixel(true);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SK_ColorWHITE);
    
    const char* title = "Export Project";
    canvas->drawString(title, 30.0f, 45.0f, font, paint);

    // Section Labels
    font.setSize(14.0f);
    paint.setColor(SkColorSetARGB(150, 255, 255, 255));
    
    // Manually positioned to match resized()
    canvas->drawString("Format", 30.0f, 85.0f, font, paint);
    canvas->drawString("Bit Depth", 30.0f, 165.0f, font, paint);
    canvas->drawString("Options", 30.0f, 245.0f, font, paint);

    // Draw Children
    drawChildren(canvas);
}

void ExportDialog::triggerExport()
{
    // Construct JSON parameters
    juce::DynamicObject* params = new juce::DynamicObject();
    params->setProperty("output_path", "C:\\zenith\\exports\\project_export"); // Default path
    params->setProperty("format", selectedFormat_);
    params->setProperty("bit_depth", selectedBitDepth_);
    params->setProperty("enable_dither", toggleDither_->getToggleState());
    params->setProperty("normalize", toggleNormalize_->getToggleState());
    params->setProperty("ai_mastering", toggleAIEnhance_->getToggleState());
    params->setProperty("duration", 0.0); // Full project

    juce::var args(params);
    
    juce::DynamicObject* request = new juce::DynamicObject();
    request->setProperty("id", "ExportProjectAdvanced");
    request->setProperty("params", args);
    commandAPI_.executeCommand(juce::var(request));
    
    // Close dialog
    setVisible(false);
}

} // namespace zenith
