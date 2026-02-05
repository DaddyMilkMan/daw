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
    SkiaProjectManager.cpp
    Skia-based project management implementation
  ==============================================================================
*/


#include <juce_core/juce_core.h>
#include <algorithm>

namespace zenith {
namespace ui {

//==============================================================================
// Construction/Destruction
//==============================================================================

SkiaProjectManager::SkiaProjectManager(Engine& engine, ProjectState& projectState)
    : engine(engine), projectState(projectState), recentProjects_(engine.getRecentProjects()) {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Initialize with default data
    initializeDefaultData();
    updateLayout();
}

SkiaProjectManager::~SkiaProjectManager() {
    removeKeyListener(this);
}

//==============================================================================
// Component Interface
//==============================================================================

void SkiaProjectManager::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Main background
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_00);
    canvas->drawRect(skBounds, bgPaint);

    // Calculate layout regions
    float y = kPanelPadding;

    headerRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                   skBounds.width() - (2 * kPanelPadding), kHeaderHeight);
    y += headerRect_.height() + kPanelSpacing;

    tabsRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                 skBounds.width() - (2 * kPanelPadding), kTabHeight);
    y += tabsRect_.height() + kPanelSpacing;

    // Split view: sidebar and main content
    sidebarRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                    kSidebarWidth, skBounds.height() - y - kFooterHeight);
    mainContentRect_ = SkRect::MakeXYWH(sidebarRect_.right() + kPanelSpacing, y,
                                        skBounds.width() - sidebarRect_.right() - (2 * kPanelSpacing) - kPanelSpacing,
                                        skBounds.height() - y - kFooterHeight);

    footerRect_ = SkRect::MakeXYWH(kPanelPadding, mainContentRect_.bottom() + kPanelSpacing,
                                  skBounds.width() - (2 * kPanelPadding), kFooterHeight);

    // Draw sections
    drawHeader(canvas, headerRect_);
    drawTabs(canvas, tabsRect_);
    drawSidebar(canvas, sidebarRect_);
    drawMainContent(canvas, mainContentRect_);
    drawFooter(canvas, footerRect_);
}

void SkiaProjectManager::resized() {
    updateLayout();
}

//==============================================================================
// Drawing Methods
//==============================================================================

void SkiaProjectManager::drawHeader(SkCanvas* canvas, const SkRect& bounds) {
    // Header background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 12.0f;
    opts.useBackdropBlur = true;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Title
    SkFont titleFont = design::getDisplayFont(20.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);

    juce::String title = "Project Manager";
    std::string titleStr = title.toStdString();
    float titleWidth = titleFont.measureText(titleStr.c_str(), titleStr.length(), SkTextEncoding::kUTF8);
    float x = bounds.centerX() - (titleWidth / 2.0f);
    float y = bounds.centerY() + 8.0f;

    canvas->drawString(titleStr.c_str(), x, y, titleFont, titlePaint);

    // Project name (if current project loaded)
    if (currentProject_.name.isNotEmpty()) {
        SkFont subFont = design::getSkFont(14.0f);
        SkPaint subPaint;
        subPaint.setColor(design::colors::TEXT_SECONDARY);
        subPaint.setAntiAlias(true);

        juce::String subtitle = currentProject_.name;
        std::string subtitleStr = subtitle.toStdString();
        float subtitleWidth = subFont.measureText(subtitleStr.c_str(), subtitleStr.length(), SkTextEncoding::kUTF8);
        x = bounds.centerX() - (subtitleWidth / 2.0f);
        y += 24.0f;

        canvas->drawString(subtitleStr.c_str(), x, y, subFont, subPaint);
    }

    // Search bar in header
    if (currentView_ == ViewMode::RecentProjects || currentView_ == ViewMode::Templates) {
        SkRect searchRect = SkRect::MakeXYWH(bounds.right() - 200, bounds.centerY() - 12, 180, 24);

        SkPaint searchBg;
        searchBg.setColor(SkColorSetA(design::colors::BG_01, 100));
        canvas->drawRoundRect(searchRect, 12.0f, 12.0f, searchBg);

        // Search icon
        SkFont iconFont = design::getSkFont(16.0f);
        SkPaint iconPaint;
        iconPaint.setColor(design::colors::TEXT_TERTIARY);
        iconPaint.setAntiAlias(true);

        canvas->drawString("🔍", searchRect.left() + 8, searchRect.centerY() + 4, iconFont, iconPaint);

        // Search text
        SkFont searchFont = design::getSkFont(12.0f);
        SkPaint searchPaint;
        searchPaint.setColor(design::colors::TEXT_TERTIARY);
        searchPaint.setAntiAlias(true);

        if (searchText_.isEmpty()) {
            canvas->drawString("Search...", searchRect.left() + 32, searchRect.centerY() + 4, searchFont, searchPaint);
        } else {
            std::string searchStr = searchText_.toStdString();
            canvas->drawString(searchStr.c_str(), searchRect.left() + 32, searchRect.centerY() + 4, searchFont, searchPaint);
        }
    }
}

void SkiaProjectManager::drawTabs(SkCanvas* canvas, const SkRect& bounds) {
    // Tab background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Draw tab buttons
    for (auto& tab : projectTabs_) {
        drawProjectTab(canvas, tab);
    }
}

void SkiaProjectManager::drawSidebar(SkCanvas* canvas, const SkRect& bounds) {
    // Sidebar background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 12.0f;
    opts.useBackdropBlur = true;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Sidebar title
    SkFont titleFont = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);

    juce::String title;
    switch (currentView_) {
        case ViewMode::RecentProjects:
            title = "Recent Projects";
            break;
        case ViewMode::Templates:
            title = "Templates";
            break;
        case ViewMode::CurrentProject:
            title = "Project Info";
            break;
        case ViewMode::ProjectSettings:
            title = "Settings";
            break;
    }

    std::string titleStr = title.toStdString();
    canvas->drawString(titleStr.c_str(), bounds.left() + 16, bounds.top() + 24, titleFont, titlePaint);

    // Sidebar content (scrollable area)
    float contentY = bounds.top() + 48;
    float itemHeight = 80.0f;
    float spacing = 12.0f;

    switch (currentView_) {
        case ViewMode::RecentProjects:
            for (size_t i = 0; i < recentProjects_.size(); ++i) {
                if (contentY + itemHeight > bounds.bottom()) break;

                SkRect itemBounds = SkRect::MakeXYWH(bounds.left() + 12, contentY,
                                                   bounds.width() - 24, itemHeight);
                bool isSelected = (i == 0); // Demo selection
                drawRecentProjectItem(canvas, itemBounds, recentProjects_[i], isSelected, false);

                contentY += itemHeight + spacing;
            }
            break;

        case ViewMode::Templates:
            for (size_t i = 0; i < projectTemplates_.size(); ++i) {
                if (contentY + itemHeight > bounds.bottom()) break;

                SkRect itemBounds = SkRect::MakeXYWH(bounds.left() + 12, contentY,
                                                   bounds.width() - 24, itemHeight);
                bool isSelected = (i == 0); // Demo selection
                drawProjectTemplateItem(canvas, itemBounds, projectTemplates_[i], isSelected, false);

                contentY += itemHeight + spacing;
            }
            break;

        case ViewMode::CurrentProject:
        case ViewMode::ProjectSettings:
            // Draw project summary
            SkFont summaryFont = design::getSkFont(12.0f);
            SkPaint summaryPaint;
            summaryPaint.setColor(design::colors::TEXT_SECONDARY);
            summaryPaint.setAntiAlias(true);

            juce::String summary = "Tracks: " + juce::String(tracks_.size());
            canvas->drawString(summary.toStdString().c_str(),
                              bounds.left() + 16, bounds.top() + 64, summaryFont, summaryPaint);

            summary = "Duration: " + formatDuration(currentProject_.duration);
            canvas->drawString(summary.toStdString().c_str(),
                              bounds.left() + 16, bounds.top() + 84, summaryFont, summaryPaint);

            summary = "Size: " + formatFileSize(currentProject_.fileSize);
            canvas->drawString(summary.toStdString().c_str(),
                              bounds.left() + 16, bounds.top() + 104, summaryFont, summaryPaint);
            break;
    }
}

void SkiaProjectManager::drawMainContent(SkCanvas* canvas, const SkRect& bounds) {
    // Main content background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 12.0f;
    opts.useBackdropBlur = true;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    switch (currentView_) {
        case ViewMode::CurrentProject:
            drawCurrentProjectView(canvas, bounds);
            break;
        case ViewMode::ProjectSettings:
            drawProjectSettingsView(canvas, bounds);
            break;
        default:
            // Welcome message for other views
            SkFont font = design::getSkFont(18.0f);
            SkPaint paint;
            paint.setColor(design::colors::TEXT_TERTIARY);
            paint.setAntiAlias(true);

            juce::String welcome = "Select a project or template to get started";
            std::string welcomeStr = welcome.toStdString();
            float msgWidth = font.measureText(welcomeStr.c_str(), welcomeStr.length(), SkTextEncoding::kUTF8);
            float x = bounds.centerX() - (msgWidth / 2.0f);
            float y = bounds.centerY();

            canvas->drawString(welcomeStr.c_str(), x, y, font, paint);
            break;
    }
}

void SkiaProjectManager::drawFooter(SkCanvas* canvas, const SkRect& bounds) {
    // Footer background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Action buttons
    float buttonWidth = 100.0f;
    float buttonHeight = 36.0f;
    float spacing = 12.0f;
    float startX = bounds.left() + 16;

    // New button
    SkRect newBtn = SkRect::MakeXYWH(startX, bounds.centerY() - (buttonHeight/2),
                                    buttonWidth, buttonHeight);
    SkPaint newBg;
    newBg.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(newBtn, 8.0f, 8.0f, newBg);

    SkPaint newText;
    newText.setColor(design::colors::TEXT_PRIMARY);
    newText.setAntiAlias(true);

    SkFont btnFont = design::getSkFont(12.0f);
    juce::String newTextStr = "New";
    std::string newStr = newTextStr.toStdString();
    canvas->drawString(newStr.c_str(), newBtn.centerX() - 20, newBtn.centerY() + 4, btnFont, newText);

    startX += buttonWidth + spacing;

    // Open button
    SkRect openBtn = SkRect::MakeXYWH(startX, bounds.centerY() - (buttonHeight/2),
                                     buttonWidth, buttonHeight);
    SkPaint openBg;
    openBg.setColor(design::colors::BG_02);
    canvas->drawRoundRect(openBtn, 8.0f, 8.0f, openBg);

    SkPaint openText;
    openText.setColor(design::colors::TEXT_PRIMARY);
    openText.setAntiAlias(true);

    juce::String openTextStr = "Open";
    std::string openStr = openTextStr.toStdString();
    canvas->drawString(openStr.c_str(), openBtn.centerX() - 20, openBtn.centerY() + 4, btnFont, openText);

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
    exportBg.setColor(SkColorSetA(design::colors::ACCENT_SECONDARY, 100));
    canvas->drawRoundRect(exportBtn, 8.0f, 8.0f, exportBg);

    SkPaint exportText;
    exportText.setColor(design::colors::TEXT_SECONDARY);
    exportText.setAntiAlias(true);

    juce::String exportTextStr = "Export";
    std::string exportStr = exportTextStr.toStdString();
    canvas->drawString(exportStr.c_str(), exportBtn.centerX() - 20, exportBtn.centerY() + 4, btnFont, exportText);
}

void SkiaProjectManager::drawProjectTab(SkCanvas* canvas, const ProjectTab& tab) {
    float cornerRadius = tab.isActive ? 12.0f : 6.0f;

    SkPaint bgPaint;
    if (tab.isActive) {
        bgPaint.setColor(design::colors::ACCENT_PRIMARY);
    } else if (tab.isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    }
    canvas->drawRoundRect(tab.bounds, cornerRadius, cornerRadius, bgPaint);

    // Button text
    SkFont font = design::getSkFont(14.0f, tab.isActive ? design::FontWeight::Semibold : design::FontWeight::Regular);
    SkPaint textPaint;
    textPaint.setColor(tab.isActive ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);

    float textWidth = font.measureText(tab.label.toStdString().c_str(), tab.label.length(), SkTextEncoding::kUTF8);
    float x = tab.centerX() - (textWidth / 2.0f);
    float y = tab.centerY() + 4.0f;

    canvas->drawString(tab.label.toStdString().c_str(), x, y, font, textPaint);
}

void SkiaProjectManager::drawRecentProjectItem(SkCanvas* canvas, const SkRect& bounds,
                                             const ProjectInfo& project, bool isSelected, bool isHovered) {
    // Item background
    SkPaint bgPaint;
    if (isSelected) {
        bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    } else if (isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 50));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_00, 20));
    }
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);

    // Project name
    SkFont nameFont = design::getSkFont(14.0f, design::FontWeight::Semibold);
    SkPaint namePaint;
    namePaint.setColor(design::colors::TEXT_PRIMARY);
    namePaint.setAntiAlias(true);

    canvas->drawString(project.name.toStdString().c_str(),
                      bounds.left() + 16, bounds.top() + 20, nameFont, namePaint);

    // Project details
    SkFont detailFont = design::getSkFont(11.0f);
    SkPaint detailPaint;
    detailPaint.setColor(design::colors::TEXT_TERTIARY);
    detailPaint.setAntiAlias(true);

    juce::String modified = "Modified: " + project.lastModified.toString(true, true);
    canvas->drawString(modified.toStdString().c_str(),
                      bounds.left() + 16, bounds.top() + 40, detailFont, detailPaint);

    juce::String size = "Size: " + formatFileSize(project.fileSize);
    canvas->drawString(size.toStdString().c_str(),
                      bounds.left() + 16, bounds.top() + 56, detailFont, detailPaint);
}

void SkiaProjectManager::drawProjectTemplateItem(SkCanvas* canvas, const SkRect& bounds,
                                               const ProjectTemplate& template_, bool isSelected, bool isHovered) {
    // Item background
    SkPaint bgPaint;
    if (isSelected) {
        bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    } else if (isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 50));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_00, 20));
    }
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);

    // Template name with category badge
    SkFont nameFont = design::getSkFont(14.0f, design::FontWeight::Semibold);
    SkPaint namePaint;
    namePaint.setColor(design::colors::TEXT_PRIMARY);
    namePaint.setAntiAlias(true);

    canvas->drawString(template_.name.toStdString().c_str(),
                      bounds.left() + 16, bounds.top() + 20, nameFont, namePaint);

    // Category badge
    SkRect badgeRect = SkRect::MakeXYWH(bounds.right() - 80, bounds.top() + 8, 70, 20);
    SkPaint badgePaint;
    badgePaint.setColor(getTrackTypeColor(template_.category));
    canvas->drawRoundRect(badgeRect, 10.0f, 10.0f, badgePaint);

    SkPaint badgeTextPaint;
    badgeTextPaint.setColor(design::colors::TEXT_PRIMARY);
    badgeTextPaint.setAntiAlias(true);

    SkFont badgeFont = design::getSkFont(10.0f);
    canvas->drawString(template_.category.toStdString().c_str(),
                      badgeRect.centerX() - badgeFont.measureText(template_.category.toStdString().c_str(),
                        template_.category.length(), SkTextEncoding::kUTF8) / 2.0f,
                      badgeRect.centerY() + 4, badgeFont, badgeTextPaint);

    // Description
    SkFont descFont = design::getSkFont(11.0f);
    SkPaint descPaint;
    descPaint.setColor(design::colors::TEXT_TERTIARY);
    descPaint.setAntiAlias(true);

    canvas->drawString(template_.description.toStdString().c_str(),
                      bounds.left() + 16, bounds.top() + 40, descFont, descPaint);

    // Duration
    SkFont durationFont = design::getSkFont(11.0f);
    SkPaint durationPaint;
    durationPaint.setColor(design::colors::TEXT_TERTIARY);
    durationPaint.setAntiAlias(true);

    juce::String duration = "Duration: " + formatDuration(template_.estimatedDuration);
    canvas->drawString(duration.toStdString().c_str(),
                      bounds.left() + 16, bounds.top() + 56, durationFont, durationPaint);
}

void SkiaProjectManager::drawCurrentProjectView(SkCanvas* canvas, const SkRect& bounds) {
    // View title
    SkFont titleFont = design::getSkFont(18.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);

    juce::String title = "Tracks";
    std::string titleStr = title.toStdString();
    canvas->drawString(titleStr.c_str(), bounds.left() + 24, bounds.top() + 24, titleFont, titlePaint);

    // Add track button
    SkRect addBtn = SkRect::MakeXYWH(bounds.right() - 80, bounds.top() + 8, 60, 32);
    SkPaint addBg;
    addBg.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(addBtn, 16.0f, 16.0f, addBg);

    SkPaint addText;
    addText.setColor(design::colors::TEXT_PRIMARY);
    addText.setAntiAlias(true);

    SkFont btnFont = design::getSkFont(12.0f);
    juce::String addTextStr = "Add";
    std::string addStr = addTextStr.toStdString();
    canvas->drawString(addStr.c_str(), addBtn.centerX() - 16, addBtn.centerY() + 4, btnFont, addText);

    // Track list
    drawTrackList(canvas, bounds);
}

void SkiaProjectManager::drawTrackList(SkCanvas* canvas, const SkRect& bounds) {
    float listY = bounds.top() + 60;
    float listHeight = bounds.height() - 80;

    for (size_t i = 0; i < tracks_.size(); ++i) {
        if (listY + kTrackHeight > bounds.bottom()) break;

        TrackRow row;
        row.info = tracks_[i];
        row.bounds = SkRect::MakeXYWH(bounds.left() + 16, listY,
                                     bounds.width() - 32, kTrackHeight);

        drawTrackRow(canvas, row);

        listY += kTrackHeight + kTrackSpacing;
    }
}

void SkiaProjectManager::drawTrackRow(SkCanvas* canvas, const TrackRow& row) {
    // Track background
    SkPaint bgPaint;
    if (row.isSelected) {
        bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    } else if (row.isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 50));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_00, 20));
    }
    canvas->drawRoundRect(row.bounds, 6.0f, 6.0f, bgPaint);

    // Track elements
    float x = row.left() + 16;
    float centerY = row.centerY();

    // Track icon
    SkFont iconFont = design::getSkFont(16.0f);
    SkPaint iconPaint;
    iconPaint.setColor(getTrackTypeColor(row.info.type));
    iconPaint.setAntiAlias(true);

    std::string icon = getTrackTypeIcon(row.info.type);
    canvas->drawString(icon.c_str(), x, centerY + 4, iconFont, iconPaint);

    x += 32;

    // Track name
    SkFont nameFont = design::getSkFont(14.0f, design::FontWeight::Medium);
    SkPaint namePaint;
    namePaint.setColor(design::colors::TEXT_PRIMARY);
    namePaint.setAntiAlias(true);

    canvas->drawString(row.info.name.toStdString().c_str(), x, centerY + 4, nameFont, namePaint);

    x += 200;

    // Volume slider
    SkRect volSlider = SkRect::MakeXYWH(x, centerY - 6, 80, 4);
    SkPaint volTrack;
    volTrack.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(volSlider, 2.0f, 2.0f, volTrack);

    float volProgress = (row.info.volume + 60) / 60.0f; // Convert -60dB to 0-1
    SkRect volFill = SkRect::MakeXYWH(x, centerY - 6, 80 * volProgress, 4);
    SkPaint volFillPaint;
    volFillPaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(volFill, 2.0f, 2.0f, volFillPaint);

    x += 100;

    // Pan slider
    SkRect panSlider = SkRect::MakeXYWH(x, centerY - 6, 80, 4);
    SkPaint panTrack;
    panTrack.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(panSlider, 2.0f, 2.0f, panTrack);

    float panPos = (row.info.pan + 1) / 2.0f; // Convert -1 to 1 to 0-1
    SkRect panFill = SkRect::MakeXYWH(x, centerY - 6, 80 * panPos, 4);
    SkPaint panFillPaint;
    panFillPaint.setColor(design::colors::ACCENT_SECONDARY);
    canvas->drawRoundRect(panFill, 2.0f, 2.0f, panFillPaint);

    x += 100;

    // Mute/Solo buttons
    float btnSize = 24.0f;

    // Mute button
    SkRect muteBtn = SkRect::MakeXYWH(x, centerY - btnSize/2, btnSize, btnSize);
    SkPaint muteBg;
    muteBg.setColor(row.info.isMuted ? design::colors::NEON_RED : SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(muteBtn, btnSize/2, btnSize/2, muteBg);

    SkPaint muteText;
    muteText.setColor(row.info.isMuted ? design::colors::TEXT_PRIMARY : design::colors::TEXT_TERTIARY);
    muteText.setAntiAlias(true);

    SkFont btnFont = design::getSkFont(12.0f);
    canvas->drawString("M", muteBtn.centerX() - 4, muteBtn.centerY() + 4, btnFont, muteText);

    x += btnSize + 8;

    // Solo button
    SkRect soloBtn = SkRect::MakeXYWH(x, centerY - btnSize/2, btnSize, btnSize);
    SkPaint soloBg;
    soloBg.setColor(row.info.isSoloed ? design::colors::NEON_GREEN : SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(soloBtn, btnSize/2, btnSize/2, soloBg);

    SkPaint soloText;
    soloText.setColor(row.info.isSoloed ? design::colors::TEXT_PRIMARY : design::colors::TEXT_TERTIARY);
    soloText.setAntiAlias(true);

    canvas->drawString("S", soloBtn.centerX() - 4, soloBtn.centerY() + 4, btnFont, soloText);
}

//==============================================================================
// Input Handling
//==============================================================================

void SkiaProjectManager::mouseDown(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;

    // Check tab clicks
    for (auto& tab : projectTabs_) {
        if (tab.bounds.contains(x, y)) {
            selectView(static_cast<ViewMode>(tab.mode));
            return;
        }
    }

    // Check footer buttons
    float buttonWidth = 100.0f;
    float buttonHeight = 36.0f;
    float spacing = 12.0f;
    float startX = footerRect_.left() + 16;

    // New button
    SkRect newBtn = SkRect::MakeXYWH(startX, footerRect_.centerY() - (buttonHeight/2),
                                    buttonWidth, buttonHeight);
    if (newBtn.contains(x, y)) {
        // TODO: Create new project dialog
        return;
    }

    // Open button
    startX += buttonWidth + spacing;
    SkRect openBtn = SkRect::MakeXYWH(startX, footerRect_.centerY() - (buttonHeight/2),
                                     buttonWidth, buttonHeight);
    if (openBtn.contains(x, y)) {
        // TODO: Show open dialog
        return;
    }

    // Save button
    startX += buttonWidth + spacing;
    SkRect saveBtn = SkRect::MakeXYWH(startX, footerRect_.centerY() - (buttonHeight/2),
                                     buttonWidth, buttonHeight);
    if (saveBtn.contains(x, y)) {
        if (saveProject()) {
            if (onProjectSaved) onProjectSaved();
        }
        return;
    }

    // Check track list clicks
    if (currentView_ == ViewMode::CurrentProject) {
        float listY = mainContentRect_.top() + 60;

        for (size_t i = 0; i < tracks_.size(); ++i) {
            SkRect trackBounds = SkRect::MakeXYWH(mainContentRect_.left() + 16, listY,
                                                mainContentRect_.width() - 32, kTrackHeight);

            if (trackBounds.contains(x, y)) {
                // Handle track interaction
                selectedTrackId_ = tracks_[i].id;

                // Check mute/solo buttons
                float btnSize = 24.0f;
                float muteX = mainContentRect_.right() - 240;
                float soloX = muteX + btnSize + 8;

                SkRect muteBtn = SkRect::MakeXYWH(muteX, listY + kTrackHeight/2 - btnSize/2, btnSize, btnSize);
                SkRect soloBtn = SkRect::MakeXYWH(soloX, listY + kTrackHeight/2 - btnSize/2, btnSize, btnSize);

                if (muteBtn.contains(x, y)) {
                    tracks_[i].isMuted = !tracks_[i].isMuted;
                    if (onTrackChanged) onTrackChanged(tracks_[i].id);
                } else if (soloBtn.contains(x, y)) {
                    tracks_[i].isSoloed = !tracks_[i].isSoloed;
                    if (onTrackChanged) onTrackChanged(tracks_[i].id);
                } else {
                    // Select track
                    for (auto& track : tracks_) {
                        track.isSelected = false;
                    }
                    tracks_[i].isSelected = true;
                }

                markDirty();
                return;
            }

            listY += kTrackHeight + kTrackSpacing;
        }
    }
}

void SkiaProjectManager::mouseMove(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;

    // Update hover states
    for (auto& tab : projectTabs_) {
        tab.isHovered = tab.bounds.contains(x, y);
    }

    markDirty();
}

void SkiaProjectManager::mouseExit(const juce::MouseEvent& e) {
    for (auto& tab : projectTabs_) {
        tab.isHovered = false;
    }
    markDirty();
}

bool SkiaProjectManager::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    if (key == juce::KeyPress::nKey && key.getModifiers().isCommandDown()) {
        // New project shortcut
        return true;
    }

    if (key == juce::KeyPress::sKey && key.getModifiers().isCommandDown()) {
        // Save project shortcut
        if (saveProject()) {
            if (onProjectSaved) onProjectSaved();
        }
        return true;
    }

    if (key == juce::KeyPress::oKey && key.getModifiers().isCommandDown()) {
        // Open project shortcut
        return true;
    }

    return false;
}

//==============================================================================
// Helper Methods
//==============================================================================

void SkiaProjectManager::updateLayout() {
    // Update tab positions
    float tabX = tabsRect_.left() + 16;
    float tabWidth = (tabsRect_.width() - 32) / projectTabs_.size();

    for (auto& tab : projectTabs_) {
        tab.bounds = SkRect::MakeXYWH(tabX, tabsRect_.top() + 8, tabWidth - 8, tabsRect_.height() - 16);
        tabX += tabWidth;
    }
}

void SkiaProjectManager::selectView(ViewMode mode) {
    currentView_ = mode;
    updateLayout();
    markDirty();
}

void SkiaProjectManager::updateRecentProjects() {
    // In a real implementation, this would load from RecentProjectManager
    recentProjects_.clear();

    // Add some demo projects
    ProjectInfo demo1;
    demo1.name = "Summer Vibes";
    demo1.lastModified = juce::Time::getCurrentTime() - juce::RelativeTime::days(2);
    demo1.fileSize = 1024000; // 1MB
    demo1.duration = 180.0f;
    recentProjects_.push_back(demo1);

    ProjectInfo demo2;
    demo2.name = "Rock Anthem";
    demo2.lastModified = juce::Time::getCurrentTime() - juce::RelativeTime::days(5);
    demo2.fileSize = 2048000; // 2MB
    demo2.duration = 240.0f;
    recentProjects_.push_back(demo2);
}

void SkiaProjectManager::updateProjectTemplates() {
    // Load project templates
    projectTemplates_.clear();

    ProjectTemplate template1;
    template1.id = "electronic";
    template1.name = "Electronic";
    template1.category = "Genre";
    template1.description = "Perfect for electronic music production";
    template1.estimatedDuration = 180;
    projectTemplates_.push_back(template1);

    ProjectTemplate template2;
    template2.id = "hiphop";
    template2.name = "Hip Hop";
    template2.category = "Genre";
    template2.description = "Classic hip hop production template";
    template2.estimatedDuration = 240;
    projectTemplates_.push_back(template2);
}

void SkiaProjectManager::initializeDefaultData() {
    // Initialize project tabs
    projectTabs_.push_back({ViewMode::RecentProjects, "Recent", SkRect(), false, false});
    projectTabs_.push_back({ViewMode::Templates, "Templates", SkRect(), false, false});
    projectTabs_.push_back({ViewMode::CurrentProject, "Project", SkRect(), false, false});
    projectTabs_.push_back({ViewMode::ProjectSettings, "Settings", SkRect(), false, false});

    // Set initial view
    projectTabs_[0].isActive = true;

    // Update data
    updateRecentProjects();
    updateProjectTemplates();

    // Initialize current project
    currentProject_.name = "Untitled Project";
    currentProject_.created = juce::Time::getCurrentTime();
    currentProject_.lastModified = juce::Time::getCurrentTime();
    currentProject_.fileSize = 512000; // 512KB
    currentProject_.duration = 0.0f;
    currentProject_.tempo = 120.0f;

    // Add default tracks
    addDefaultTracks();
}

void SkiaProjectManager::addDefaultTracks() {
    // Add master track
    TrackInfo master;
    master.id = "master";
    master.name = "Master";
    master.type = "master";
    master.volume = 0.0f;
    tracks_.push_back(master);

    // Add audio track
    TrackInfo audio1;
    audio1.id = "audio1";
    audio1.name = "Audio Track 1";
    audio1.type = "audio";
    audio1.volume = -6.0f;
    audio1.pan = 0.0f;
    tracks_.push_back(audio1);

    // Add MIDI track
    TrackInfo midi1;
    midi1.id = "midi1";
    midi1.name = "MIDI Track 1";
    midi1.type = "midi";
    midi1.volume = -3.0f;
    midi1.pan = 0.0f;
    tracks_.push_back(midi1);
}

bool SkiaProjectManager::loadProject(const juce::File& file) {
    // TODO: Implement actual project loading
    currentProject_.name = file.getFileNameWithoutExtension();
    currentProject_.filePath = file.getFullPathName();
    currentProject_.lastModified = juce::Time::getCurrentTime();
    currentProject_.lastSaved = juce::Time::getCurrentTime();

    if (onProjectLoaded) {
        onProjectLoaded(file);
    }

    markDirty();
    return true;
}

bool SkiaProjectManager::saveProject() {
    // TODO: Implement actual project saving
    currentProject_.lastSaved = juce::Time::getCurrentTime();
    hasUnsavedChanges_ = false;
    markDirty();
    return true;
}

juce::String SkiaProjectManager::formatFileSize(size_t bytes) const {
    if (bytes < 1024) return juce::String(bytes) + " B";
    if (bytes < 1024 * 1024) return juce::String(bytes / 1024) + " KB";
    return juce::String(bytes / (1024 * 1024)) + " MB";
}

juce::String SkiaProjectManager::formatDuration(float seconds) const {
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    return juce::String(minutes) + ":" + juce::String(secs).paddedLeft('0', 2);
}

juce::String SkiaProjectManager::getTrackTypeIcon(const juce::String& type) const {
    if (type == "audio") return "🎵";
    if (type == "midi") return "🎹";
    if (type == "master") return "🔊";
    if (type == "bus") return "🎛️";
    return "🎧";
}

SkColor SkiaProjectManager::getTrackTypeColor(const juce::String& type) const {
    if (type == "audio") return SkColorSetARGB(255, 59, 130, 246); // Blue
    if (type == "midi") return SkColorSetARGB(255, 16, 185, 129); // Green
    if (type == "master") return SkColorSetARGB(255, 239, 68, 68); // Red
    if (type == "bus") return SkColorSetARGB(255, 156, 163, 175); // Gray
    return SkColorSetARGB(255, 156, 163, 175); // Default gray
}

} // namespace ui
} // namespace zenith