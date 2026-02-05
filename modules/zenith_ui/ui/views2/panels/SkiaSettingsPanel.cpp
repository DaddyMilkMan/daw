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
    SkiaSettingsPanel.cpp
    Skia-based settings panel implementation
  ==============================================================================
*/


#include <algorithm>

namespace zenith {
namespace ui {

//==============================================================================
// Construction/Destruction
//==============================================================================

SkiaSettingsPanel::SkiaSettingsPanel(Engine& engine)
    : engine(engine), selectedCategory_(SettingCategory::Mastering) {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Initialize default settings
    initializeDefaultSettings();
    updateLayout();
}

SkiaSettingsPanel::~SkiaSettingsPanel() {
    removeKeyListener(this);
}

//==============================================================================
// Component Interface
//==============================================================================

void SkiaSettingsPanel::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Main background with subtle gradient
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_00);
    canvas->drawRect(skBounds, bgPaint);

    // Calculate layout regions
    float y = kPanelPadding;

    headerRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                   skBounds.width() - (2 * kPanelPadding), kHeaderHeight);
    y += headerRect_.height() + kTabSpacing;

    tabsRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                 skBounds.width() - (2 * kPanelPadding), kTabHeight);
    y += tabsRect_.height() + kPanelSpacing;

    // Preview panel only for certain categories
    bool showPreview = (selectedCategory_ == SettingCategory::Mastering ||
                       selectedCategory_ == SettingCategory::Analysis);

    if (showPreview) {
        previewRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                        skBounds.width() - (2 * kPanelPadding), kPreviewHeight);
        y += previewRect_.height() + kPanelSpacing;
    }

    settingsRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                    skBounds.width() - (2 * kPanelPadding),
                                    skBounds.height() - y - kFooterHeight);

    footerRect_ = SkRect::MakeXYWH(kPanelPadding, settingsRect_.bottom() + kPanelSpacing,
                                  skBounds.width() - (2 * kPanelPadding), kFooterHeight);

    // Draw sections
    drawHeader(canvas, headerRect_);
    drawTabs(canvas, tabsRect_);

    if (showPreview) {
        drawPreviewPanel(canvas, previewRect_);
    }

    drawSettingsList(canvas, settingsRect_);
    drawFooter(canvas, footerRect_);
}

void SkiaSettingsPanel::resized() {
    updateLayout();
}

//==============================================================================
// Drawing Methods
//==============================================================================

void SkiaSettingsPanel::drawHeader(SkCanvas* canvas, const SkRect& bounds) {
    // Header background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 12.0f;
    opts.useBackdropBlur = true;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Title
    SkFont titleFont = design::getDisplayFont(24.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);

    juce::String title = "Settings";
    std::string titleStr = title.toStdString();
    float titleWidth = titleFont.measureText(titleStr.c_str(), titleStr.length(), SkTextEncoding::kUTF8);
    float x = bounds.centerX() - (titleWidth / 2.0f);
    float y = bounds.centerY() + 8.0f;

    canvas->drawString(titleStr.c_str(), x, y, titleFont, titlePaint);

    // Subtitle
    SkFont subFont = design::getSkFont(14.0f);
    SkPaint subPaint;
    subPaint.setColor(design::colors::TEXT_SECONDARY);
    subPaint.setAntiAlias(true);

    juce::String subtitle = getCategoryName(selectedCategory_);
    std::string subtitleStr = subtitle.toStdString();
    float subtitleWidth = subFont.measureText(subtitleStr.c_str(), subtitleStr.length(), SkTextEncoding::kUTF8);
    x = bounds.centerX() - (subtitleWidth / 2.0f);
    y += 24.0f;

    canvas->drawString(subtitleStr.c_str(), x, y, subFont, subPaint);

    // AI suggestions indicator
    if (aiSuggestionsVisible_ && aiSuggestions_.isNotEmpty()) {
        SkFont aiFont = design::getSkFont(12.0f);
        SkPaint aiPaint;
        aiPaint.setColor(design::colors::ACCENT_PRIMARY);
        aiPaint.setAntiAlias(true);

        juce::String aiText = "💡 AI Suggestions";
        std::string aiTextStr = aiText.toStdString();
        canvas->drawString(aiTextStr.c_str(), bounds.right() - 120, bounds.centerY() + 8, aiFont, aiPaint);
    }
}

void SkiaSettingsPanel::drawTabs(SkCanvas* canvas, const SkRect& bounds) {
    // Tab background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Draw tab buttons
    for (auto& tab : tabButtons_) {
        drawTabButton(canvas, tab);
    }
}

void SkiaSettingsPanel::drawPreviewPanel(SkCanvas* canvas, const SkRect& bounds) {
    // Preview background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 12.0f;
    opts.useBackdropBlur = true;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Title
    SkFont titleFont = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);

    juce::String title = "Live Preview";
    std::string titleStr = title.toStdString();
    canvas->drawString(titleStr.c_str(), bounds.left() + 16, bounds.top() + 24, titleFont, titlePaint);

    // Before/after waveforms
    float halfWidth = bounds.width() / 2.0f - 32.0f;

    // Before waveform
    SkRect beforeRect = SkRect::MakeXYWH(bounds.left() + 16, bounds.top() + 40, halfWidth, 120);
    SkPaint beforeBg;
    beforeBg.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(beforeRect, 8.0f, 8.0f, beforeBg);

    // After waveform
    SkRect afterRect = SkRect::MakeXYWH(bounds.centerX() + 16, bounds.top() + 40, halfWidth, 120);
    SkPaint afterBg;
    afterBg.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    canvas->drawRoundRect(afterRect, 8.0f, 8.0f, afterBg);

    // Labels
    SkFont labelFont = design::getSkFont(12.0f);
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_TERTIARY);
    labelPaint.setAntiAlias(true);

    canvas->drawString("Original", beforeRect.centerX() - 20, beforeRect.bottom() + 16, labelFont, labelPaint);
    canvas->drawString("Modified", afterRect.centerX() - 20, afterRect.bottom() + 16, labelFont, labelPaint);
}

void SkiaSettingsPanel::drawSettingsList(SkCanvas* canvas, const SkRect& bounds) {
    // Settings background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Draw settings for current category
    for (auto& row : settingRows_) {
        if (row.definition.category == selectedCategory_) {
            drawSettingRow(canvas, row);
        }
    }

    // Empty state
    bool hasSettings = false;
    for (const auto& row : settingRows_) {
        if (row.definition.category == selectedCategory_) {
            hasSettings = true;
            break;
        }
    }

    if (!hasSettings) {
        SkFont font = design::getSkFont(16.0f);
        SkPaint paint;
        paint.setColor(design::colors::TEXT_TERTIARY);
        paint.setAntiAlias(true);

        juce::String emptyMsg = "No settings available";
        std::string emptyMsgStr = emptyMsg.toStdString();
        float msgWidth = font.measureText(emptyMsgStr.c_str(), emptyMsgStr.length(), SkTextEncoding::kUTF8);
        float x = bounds.centerX() - (msgWidth / 2.0f);
        float y = bounds.centerY();

        canvas->drawString(emptyMsgStr.c_str(), x, y, font, paint);
    }
}

void SkiaSettingsPanel::drawFooter(SkCanvas* canvas, const SkRect& bounds) {
    // Action buttons
    float buttonWidth = 100.0f;
    float buttonHeight = 36.0f;
    float spacing = 12.0f;
    float startX = bounds.left() + 16;

    // Reset button
    SkRect resetBtn = SkRect::MakeXYWH(startX, bounds.centerY() - (buttonHeight/2),
                                      buttonWidth, buttonHeight);
    SkPaint resetBg;
    resetBg.setColor(SkColorSetA(design::colors::BG_02, 100));
    canvas->drawRoundRect(resetBtn, 8.0f, 8.0f, resetBg);

    SkPaint resetText;
    resetText.setColor(design::colors::TEXT_SECONDARY);
    resetText.setAntiAlias(true);

    SkFont btnFont = design::getSkFont(12.0f);
    juce::String resetTextStr = "Reset";
    std::string resetStr = resetTextStr.toStdString();
    canvas->drawString(resetStr.c_str(), resetBtn.centerX() - 20, resetBtn.centerY() + 4, btnFont, resetText);

    startX += buttonWidth + spacing;

    // Save button
    SkRect saveBtn = SkRect::MakeXYWH(startX, bounds.centerY() - (buttonHeight/2),
                                     buttonWidth, buttonHeight);
    SkPaint saveBg;
    saveBg.setColor(hasUnsavedChanges_ ? design::colors::ACCENT_PRIMARY : SkColorSetA(design::colors::BG_02, 100));
    canvas->drawRoundRect(saveBtn, 8.0f, 8.0f, saveBg);

    SkPaint saveText;
    saveText.setColor(hasUnsavedChanges_ ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
    saveText.setAntiAlias(true);

    juce::String saveTextStr = "Save";
    std::string saveStr = saveTextStr.toStdString();
    canvas->drawString(saveStr.c_str(), saveBtn.centerX() - 20, saveBtn.centerY() + 4, btnFont, saveText);

    startX += buttonWidth + spacing;

    // Export button
    SkRect exportBtn = SkRect::MakeXYWH(startX, bounds.centerY() - (buttonHeight/2),
                                       buttonWidth, buttonHeight);
    SkPaint exportBg;
    exportBg.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(exportBtn, 8.0f, 8.0f, exportBg);

    SkPaint exportText;
    exportText.setColor(design::colors::TEXT_PRIMARY);
    exportText.setAntiAlias(true);

    juce::String exportTextStr = "Export";
    std::string exportStr = exportTextStr.toStdString();
    canvas->drawString(exportStr.c_str(), exportBtn.centerX() - 20, exportBtn.centerY() + 4, btnFont, exportText);
}

void SkiaSettingsPanel::drawTabButton(SkCanvas* canvas, const TabButton& button) {
    float cornerRadius = button.isActive ? 12.0f : 6.0f;

    SkPaint bgPaint;
    if (button.isActive) {
        bgPaint.setColor(design::colors::ACCENT_PRIMARY);
    } else if (button.isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    }
    canvas->drawRoundRect(button.bounds, cornerRadius, cornerRadius, bgPaint);

    // Button text
    SkFont font = design::getSkFont(14.0f, button.isActive ? design::FontWeight::Semibold : design::FontWeight::Regular);
    SkPaint textPaint;
    textPaint.setColor(button.isActive ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);

    float textWidth = font.measureText(button.label.toStdString().c_str(), button.label.length(), SkTextEncoding::kUTF8);
    float x = button.centerX() - (textWidth / 2.0f);
    float y = button.centerY() + 4.0f;

    canvas->drawString(button.label.toStdString().c_str(), x, y, font, textPaint);
}

void SkiaSettingsPanel::drawSettingRow(SkCanvas* canvas, const SettingRow& row) {
    // Setting background
    SkPaint bgPaint;
    if (row.isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 50));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_00, 20));
    }
    canvas->drawRoundRect(row.bounds, 6.0f, 6.0f, bgPaint);

    // Setting name
    SkFont nameFont = design::getSkFont(14.0f, design::FontWeight::Medium);
    SkPaint namePaint;
    namePaint.setColor(design::colors::TEXT_PRIMARY);
    namePaint.setAntiAlias(true);

    canvas->drawString(row.definition.name.toStdString().c_str(),
                      row.left() + 16, row.top() + 20, nameFont, namePaint);

    // Setting value based on type
    switch (row.definition.type) {
        case SettingType::Slider:
            drawSliderSetting(canvas, row);
            break;
        case SettingType::Toggle:
            drawToggleSetting(canvas, row);
            break;
        case SettingType::ComboBox:
            drawComboBoxSetting(canvas, row);
            break;
        case SettingType::Text:
            drawTextSetting(canvas, row);
            break;
        case SettingType::Button:
            drawButtonSetting(canvas, row);
            break;
        default:
            // Default text display
            SkFont valueFont = design::getSkFont(12.0f);
            SkPaint valuePaint;
            valuePaint.setColor(design::colors::TEXT_SECONDARY);
            valuePaint.setAntiAlias(true);

            juce::String displayValue = row.currentValue.toString();
            canvas->drawString(displayValue.toStdString().c_str(),
                              row.right() - 120, row.top() + 20, valueFont, valuePaint);
    }
}

void SkiaSettingsPanel::drawSliderSetting(SkCanvas* canvas, const SettingRow& row) {
    float sliderY = row.centerY();
    float sliderWidth = row.width() - 200.0f;
    float sliderX = row.right() - sliderWidth - 16;

    // Slider track
    SkRect trackRect = SkRect::MakeXYWH(sliderX, sliderY - 2, sliderWidth, 4);
    SkPaint trackPaint;
    trackPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(trackRect, 2.0f, 2.0f, trackPaint);

    // Slider value
    float value = row.currentValue;
    float minValue = row.definition.minValue;
    float maxValue = row.definition.maxValue;
    float progress = (value - minValue) / (maxValue - minValue);
    float fillWidth = sliderWidth * progress;

    SkRect fillRect = SkRect::MakeXYWH(sliderX, sliderY - 2, fillWidth, 4);
    SkPaint fillPaint;
    fillPaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(fillRect, 2.0f, 2.0f, fillPaint);

    // Value text
    SkFont valueFont = design::getSkFont(12.0f);
    SkPaint valuePaint;
    valuePaint.setColor(design::colors::TEXT_SECONDARY);
    valuePaint.setAntiAlias(true);

    juce::String valueStr = juce::String(value, 1);
    canvas->drawString(valueStr.toStdString().c_str(),
                      row.right() - 40, row.centerY() + 4, valueFont, valuePaint);

    // Handle
    SkRect handleRect = SkRect::MakeXYWH(sliderX + fillWidth - 6, sliderY - 8, 12, 12);
    SkPaint handlePaint;
    handlePaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(handleRect, 6.0f, 6.0f, handlePaint);
}

void SkiaSettingsPanel::drawToggleSetting(SkCanvas* canvas, const SettingRow& row) {
    float toggleX = row.right() - 100;
    float toggleY = row.centerY();
    float toggleSize = 24.0f;

    // Toggle background
    SkRect toggleRect = SkRect::MakeXYWH(toggleX - toggleSize/2, toggleY - toggleSize/2,
                                        toggleSize, toggleSize);
    SkPaint bgPaint;
    bgPaint.setColor(row.currentValue ? design::colors::ACCENT_PRIMARY : SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(toggleRect, toggleSize/2, toggleSize/2, bgPaint);

    // Toggle handle
    if (row.currentValue) {
        SkRect handleRect = SkRect::MakeXYWH(toggleRect.right() - 16, toggleRect.top() + 2,
                                           12, 12);
        SkPaint handlePaint;
        handlePaint.setColor(design::colors::TEXT_PRIMARY);
        canvas->drawRoundRect(handleRect, 6.0f, 6.0f, handlePaint);
    } else {
        SkRect handleRect = SkRect::MakeXYWH(toggleRect.left() + 2, toggleRect.top() + 2,
                                           12, 12);
        SkPaint handlePaint;
        handlePaint.setColor(design::colors::TEXT_PRIMARY);
        canvas->drawRoundRect(handleRect, 6.0f, 6.0f, handlePaint);
    }
}

void SkiaSettingsPanel::drawComboBoxSetting(SkCanvas* canvas, const SettingRow& row) {
    float comboX = row.right() - 200;
    float comboY = row.centerY();
    float comboWidth = 180.0f;
    float comboHeight = 24.0f;

    // Combo box background
    SkRect comboRect = SkRect::MakeXYWH(comboX, comboY - comboHeight/2,
                                        comboWidth, comboHeight);
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(comboRect, 6.0f, 6.0f, bgPaint);

    // Selected value
    SkFont font = design::getSkFont(12.0f);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);

    juce::String selectedValue = row.currentValue.toString();
    canvas->drawString(selectedValue.toStdString().c_str(),
                      comboX + 8, comboY + 4, font, textPaint);

    // Dropdown arrow
    SkPaint arrowPaint;
    arrowPaint.setColor(design::colors::TEXT_SECONDARY);
    arrowPaint.setStyle(SkPaint::kStroke_Style);
    arrowPaint.setStrokeWidth(2.0f);
    arrowPaint.setAntiAlias(true);

    canvas->drawLine(comboRect.right() - 16, comboY - 4,
                     comboRect.right() - 10, comboY + 4, arrowPaint);
    canvas->drawLine(comboRect.right() - 10, comboY + 4,
                     comboRect.right() - 4, comboY - 4, arrowPaint);
}

void SkiaSettingsPanel::drawTextSetting(SkCanvas* canvas, const SettingRow& row) {
    float textX = row.right() - 200;
    float textY = row.centerY();

    // Text field background
    SkRect textRect = SkRect::MakeXYWH(textX, textY - 12, 180, 24);
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(textRect, 6.0f, 6.0f, bgPaint);

    // Text value
    SkFont font = design::getSkFont(12.0f);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);

    canvas->drawString(row.currentValue.toString().toStdString().c_str(),
                      textX + 8, textY + 4, font, textPaint);
}

void SkiaSettingsPanel::drawButtonSetting(SkCanvas* canvas, const SettingRow& row) {
    float buttonX = row.right() - 200;
    float buttonY = row.centerY();
    float buttonWidth = 180.0f;
    float buttonHeight = 32.0f;

    // Button background
    SkRect buttonRect = SkRect::MakeXYWH(buttonX, buttonY - buttonHeight/2,
                                        buttonWidth, buttonHeight);
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(buttonRect, 8.0f, 8.0f, bgPaint);

    // Button text
    SkFont font = design::getSkFont(14.0f, design::FontWeight::Medium);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);

    canvas->drawString(row.definition.name.toStdString().c_str(),
                      buttonRect.centerX() - 40, buttonRect.centerY() + 4, font, textPaint);
}

//==============================================================================
// Input Handling
//==============================================================================

void SkiaSettingsPanel::mouseDown(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;

    // Check tab buttons
    for (auto& tab : tabButtons_) {
        if (tab.bounds.contains(x, y)) {
            selectCategory(tab.category);
            return;
        }
    }

    // Check setting rows
    for (auto& row : settingRows_) {
        if (row.bounds.contains(x, y)) {
            // Handle setting interaction based on type
            switch (row.definition.type) {
                case SettingType::Slider: {
                    float sliderX = row.right() - 200.0f;
                    float sliderWidth = row.width() - 200.0f;
                    float sliderValue = (x - sliderX) / sliderWidth;
                    float newValue = row.definition.minValue + sliderValue * (row.definition.maxValue - row.definition.minValue);
                    row.currentValue = newValue;
                    hasUnsavedChanges_ = true;
                    if (onSettingChanged) onSettingChanged(row.definition.id);
                    break;
                }
                case SettingType::Toggle: {
                    row.currentValue = !row.currentValue;
                    hasUnsavedChanges_ = true;
                    if (onSettingChanged) onSettingChanged(row.definition.id);
                    break;
                }
                case SettingType::Button: {
                    // Handle button action
                    break;
                }
            }
            markDirty();
            return;
        }
    }

    // Check footer buttons
    // Reset button
    float buttonWidth = 100.0f;
    float buttonHeight = 36.0f;
    float startX = footerRect_.left() + 16;

    SkRect resetBtn = SkRect::MakeXYWH(startX, footerRect_.centerY() - (buttonHeight/2),
                                      buttonWidth, buttonHeight);
    if (resetBtn.contains(x, y)) {
        resetToDefaults();
        return;
    }

    startX += buttonWidth + 12.0f;

    // Save button
    SkRect saveBtn = SkRect::MakeXYWH(startX, footerRect_.centerY() - (buttonHeight/2),
                                     buttonWidth, buttonHeight);
    if (saveBtn.contains(x, y)) {
        applySettings();
        return;
    }

    startX += buttonWidth + 12.0f;

    // Export button
    SkRect exportBtn = SkRect::MakeXYWH(startX, footerRect_.centerY() - (buttonHeight/2),
                                       buttonWidth, buttonHeight);
    if (exportBtn.contains(x, y)) {
        if (onExportRequested) onExportRequested();
        return;
    }
}

void SkiaSettingsPanel::mouseMove(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;

    int previousHovered = -1;
    for (int i = 0; i < static_cast<int>(settingRows_.size()); ++i) {
        if (settingRows_[i].isHovered) {
            previousHovered = i;
        }
        settingRows_[i].isHovered = settingRows_[i].bounds.contains(x, y);
    }

    // Check tab buttons hover
    for (auto& tab : tabButtons_) {
        tab.isHovered = tab.bounds.contains(x, y);
    }

    if (previousHovered != -1) {
        markDirty();
    }
}

void SkiaSettingsPanel::mouseExit(const juce::MouseEvent& e) {
    for (auto& row : settingRows_) {
        row.isHovered = false;
    }
    for (auto& tab : tabButtons_) {
        tab.isHovered = false;
    }
    markDirty();
}

bool SkiaSettingsPanel::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    if (key == juce::KeyPress::escapeKey) {
        if (hasUnsavedChanges_) {
            // TODO: Show confirmation dialog
            hasUnsavedChanges_ = false;
        }
        return true;
    }

    if (key.isKeyCode(juce::KeyPress::sKey) && key.getModifiers().isCommandDown()) {
        applySettings();
        return true;
    }

    return false;
}

//==============================================================================
// Helper Methods
//==============================================================================

void SkiaSettingsPanel::updateLayout() {
    // Update tab button positions
    float tabX = tabsRect_.left() + 16;
    float tabWidth = (tabsRect_.width() - 32) / tabButtons_.size();

    for (auto& tab : tabButtons_) {
        tab.bounds = SkRect::MakeXYWH(tabX, tabsRect_.top() + 8, tabWidth - 8, tabsRect_.height() - 16);
        tabX += tabWidth;
    }

    // Update setting row positions
    float settingY = settingsRect_.top() + 16;
    for (auto& row : settingRows_) {
        if (row.definition.category == selectedCategory_) {
            row.bounds = SkRect::MakeXYWH(settingsRect_.left() + 16, settingY,
                                        settingsRect_.width() - 32, 48);
            settingY += row.bounds.height() + kSettingSpacing;
        }
    }

    markDirty();
}

void SkiaSettingsPanel::selectCategory(SettingCategory category) {
    selectedCategory_ = category;
    updateLayout();
    updatePreview();

    // Get AI recommendations for this category
    showAIRecommendations();
}

void SkiaSettingsPanel::updatePreview() {
    // Update the preview panel with current settings
    // This would typically involve processing audio and updating waveform displays
}

void SkiaSettingsPanel::applySettings() {
    // Save all current settings
    for (auto& row : settingRows_) {
        for (auto& value : currentValues_) {
            if (value.id == row.definition.id) {
                value.value = row.currentValue;
                value.hasChanges = true;
                break;
            }
        }
    }

    hasUnsavedChanges_ = false;

    // Notify listeners
    if (onSettingChanged) {
        for (const auto& value : currentValues_) {
            if (value.hasChanges) {
                onSettingChanged(value.id);
            }
        }
    }

    markDirty();
}

void SkiaSettingsPanel::initializeDefaultSettings() {
    // Add some default settings for demo
    SettingDefinition setting1;
    setting1.id = "master_volume";
    setting1.name = "Master Volume";
    setting1.description = "Overall output volume";
    setting1.type = SettingType::Slider;
    setting1.category = SettingCategory::Mastering;
    setting1.defaultValue = 0.8f;
    setting1.minValue = 0.0f;
    setting1.maxValue = 1.0f;
    addSetting(setting1);

    SettingDefinition setting2;
    setting2.id = "enable_ai";
    setting2.name = "Enable AI Enhancement";
    setting2.description = "Use AI for audio processing";
    setting2.type = SettingType::Toggle;
    setting2.category = SettingCategory::Advanced;
    setting2.defaultValue = true;
    addSetting(setting2);

    SettingDefinition setting3;
    setting3.id = "analysis_mode";
    setting3.name = "Analysis Mode";
    setting3.description = "Audio analysis processing mode";
    setting3.type = SettingType::ComboBox;
    setting3.category = SettingCategory::Analysis;
    setting3.defaultValue = "Real-time";
    setting3.options = {"Real-time", "Batch", "Offline"};
    addSetting(setting3);

    SettingDefinition setting4;
    setting4.id = "ai_suggestions";
    setting4.name = "Get AI Suggestions";
    setting4.description = "Get recommendations for current settings";
    setting4.type = SettingType::Button;
    setting4.category = SettingCategory::Learning;
    addSetting(setting4);

    // Initialize tab buttons
    tabButtons_.clear();
    tabButtons_.push_back({SettingCategory::Mastering, "Mastering", SkRect(), false, false});
    tabButtons_.push_back({SettingCategory::Analysis, "Analysis", SkRect(), false, false});
    tabButtons_.push_back({SettingCategory::Learning, "Learning", SkRect(), false, false});
    tabButtons_.push_back({SettingCategory::Advanced, "Advanced", SkRect(), false, false});
    tabButtons_.push_back({SettingCategory::Presets, "Presets", SkRect(), false, false});

    // Set initial category
    for (auto& tab : tabButtons_) {
        tab.isActive = (tab.category == selectedCategory_);
    }
}

void SkiaSettingsPanel::addSetting(const SettingDefinition& definition) {
    settingDefinitions_.push_back(definition);

    // Create setting row
    SettingRow row;
    row.definition = definition;
    row.currentValue = definition.defaultValue;

    // Add to current values
    SettingValue value;
    value.id = definition.id;
    value.value = definition.defaultValue;
    currentValues_.push_back(value);

    // Add to default values
    SettingValue defaultValue;
    defaultValue.id = definition.id;
    defaultValue.value = definition.defaultValue;
    defaultValues_.push_back(defaultValue);

    settingRows_.push_back(row);
}

juce::var SkiaSettingsPanel::getSettingValue(const juce::String& id) const {
    for (const auto& value : currentValues_) {
        if (value.id == id) {
            return value.value;
        }
    }
    return juce::var();
}

void SkiaSettingsPanel::setSettingValue(const juce::String& id, const juce::var& value) {
    for (auto& row : settingRows_) {
        if (row.definition.id == id) {
            row.currentValue = value;
            markDirty();
            break;
        }
    }
}

void SkiaSettingsPanel::resetToDefaults() {
    for (auto& row : settingRows_) {
        row.currentValue = row.definition.defaultValue;
        markDirty();
    }
    hasUnsavedChanges_ = false;
}

void SkiaSettingsPanel::exportSettings(const juce::File& file) {
    // TODO: Implement settings export to JSON or other format
}

void SkiaSettingsPanel::importSettings(const juce::File& file) {
    // TODO: Implement settings import from JSON or other format
}

juce::String SkiaSettingsPanel::getCategoryName(SettingCategory category) const {
    switch (category) {
        case SettingCategory::Mastering: return "Mastering Settings";
        case SettingCategory::Analysis: return "Analysis Settings";
        case SettingCategory::Learning: return "Learning Settings";
        case SettingCategory::Advanced: return "Advanced Settings";
        case SettingCategory::Presets: return "Presets";
        default: return "Settings";
    }
}

void SkiaSettingsPanel::showAIRecommendations() {
    // Simulate AI suggestions
    aiSuggestions_ = "Based on your current settings, consider increasing the master volume by 10% for better clarity.";
    aiSuggestionsVisible_ = true;
    markDirty();
}

void SkiaSettingsPanel::validateSetting(const SettingRow& row) {
    // TODO: Add validation logic for settings
}

} // namespace ui
} // namespace zenith