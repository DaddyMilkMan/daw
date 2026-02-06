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

#include "GlobalSettingsPanel.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
#include "zenith_core/engine/Settings.h"

namespace zenith {

using namespace design;

//==============================================================================
// Construction / Destruction
//==============================================================================

GlobalSettingsPanel::GlobalSettingsPanel()
    : currentSection_(Section::General)
    , hoveredItemIndex_(-1)
    , hoveredSectionIndex_(-1)
    , animationProgress_(1.0f)
    , editingItem_(nullptr)
{
    setName("GlobalSettingsPanel");
    setSize(800, 600);
    setWantsKeyboardFocus(true);
    
    // Initialize all settings sections
    initializeSections();
    
    // Load saved settings
    loadSettings();
}

GlobalSettingsPanel::~GlobalSettingsPanel() = default;

//==============================================================================
// Initialization
//==============================================================================

void GlobalSettingsPanel::initializeSections()
{
    sections_.clear();
    
    setupGeneralSection();
    setupProjectsSection();
    setupPrivacySection();
    setupNetworkSection();
    setupAccountSection();
}

void GlobalSettingsPanel::setupGeneralSection()
{
    SettingsSection section;
    section.name = "General";
    section.icon = "⚙️";
    
    // Language
    SettingItem language;
    language.id = "general.language";
    language.label = "Language";
    language.description = "Interface language";
    language.type = SettingItem::Choice;
    language.choices.add("English");
    language.choices.add("Spanish");
    language.choices.add("French");
    language.choices.add("German");
    language.choices.add("Japanese");
    language.value = "English";
    language.defaultValue = "English";
    language.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("language", value.toString());
    };
    section.items.push_back(std::move(language));
    
    // Startup Behavior
    SettingItem startup;
    startup.id = "general.startup_behavior";
    startup.label = "On Startup";
    startup.description = "What to show when the application starts";
    startup.type = SettingItem::Choice;
    startup.choices.add("Show Welcome Screen");
    startup.choices.add("Open Recent Project");
    startup.choices.add("Create New Project");
    startup.choices.add("Show Template Browser");
    startup.value = "Show Welcome Screen";
    startup.defaultValue = "Show Welcome Screen";
    startup.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("startup_behavior", value.toString());
    };
    section.items.push_back(std::move(startup));
    
    // Check for Updates
    SettingItem updates;
    updates.id = "general.check_updates";
    updates.label = "Check for Updates";
    updates.description = "Automatically check for new versions on startup";
    updates.type = SettingItem::Boolean;
    updates.value = true;
    updates.defaultValue = true;
    updates.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("check_updates", value);
    };
    section.items.push_back(std::move(updates));
    
    // Beta Updates
    SettingItem beta;
    beta.id = "general.beta_updates";
    beta.label = "Include Beta Versions";
    beta.description = "Show beta and pre-release updates";
    beta.type = SettingItem::Boolean;
    beta.value = false;
    beta.defaultValue = false;
    beta.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("beta_updates", value);
    };
    section.items.push_back(std::move(beta));
    
    sections_.push_back(std::move(section));
}

void GlobalSettingsPanel::setupProjectsSection()
{
    SettingsSection section;
    section.name = "Projects";
    section.icon = "📁";
    
    // Default Project Location
    SettingItem projectLocation;
    projectLocation.id = "projects.default_location";
    projectLocation.label = "Default Project Location";
    projectLocation.description = "Where new projects are saved by default";
    projectLocation.type = SettingItem::Path;
    projectLocation.value = juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Zenith Projects").getFullPathName();
    projectLocation.defaultValue = projectLocation.value;
    projectLocation.onClick = [this]() {
        juce::FileChooser chooser("Select Default Project Location",
                                 juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                 "*",
                                 true);
        if (chooser.browseForDirectory()) {
            auto result = chooser.getResult();
            // Find the path setting and update it
            for (auto& section : sections_) {
                for (auto& item : section.items) {
                    if (item.id == "projects.default_location") {
                        item.value = result.getFullPathName();
                        Settings::getInstance().setValue("default_project_path", result.getFullPathName());
                        markDirty();
                        break;
                    }
                }
            }
        }
    };
    section.items.push_back(std::move(projectLocation));
    
    // Auto-save
    SettingItem autosave;
    autosave.id = "projects.autosave";
    autosave.label = "Auto-save Projects";
    autosave.description = "Automatically save project backups while working";
    autosave.type = SettingItem::Boolean;
    autosave.value = true;
    autosave.defaultValue = true;
    autosave.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("autosave_enabled", value);
    };
    section.items.push_back(std::move(autosave));
    
    // Auto-save Interval
    SettingItem autosaveInterval;
    autosaveInterval.id = "projects.autosave_interval";
    autosaveInterval.label = "Auto-save Interval";
    autosaveInterval.description = "Minutes between auto-saves";
    autosaveInterval.type = SettingItem::Integer;
    autosaveInterval.minValue = 1;
    autosaveInterval.maxValue = 60;
    autosaveInterval.value = 5;
    autosaveInterval.defaultValue = 5;
    autosaveInterval.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("autosave_interval_minutes", value);
    };
    section.items.push_back(std::move(autosaveInterval));
    
    // Maximum Backups
    SettingItem maxBackups;
    maxBackups.id = "projects.max_backups";
    maxBackups.label = "Maximum Backup Files";
    maxBackups.description = "Number of auto-save backups to keep per project";
    maxBackups.type = SettingItem::Integer;
    maxBackups.minValue = 1;
    maxBackups.maxValue = 100;
    maxBackups.value = 10;
    maxBackups.defaultValue = 10;
    maxBackups.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("max_backup_files", value);
    };
    section.items.push_back(std::move(maxBackups));
    
    // Create Project Folders
    SettingItem projectFolders;
    projectFolders.id = "projects.create_folders";
    projectFolders.label = "Create Project Folders";
    projectFolders.description = "Automatically create Audio, MIDI, and Samples subfolders";
    projectFolders.type = SettingItem::Boolean;
    projectFolders.value = true;
    projectFolders.defaultValue = true;
    projectFolders.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("create_project_folders", value);
    };
    section.items.push_back(std::move(projectFolders));
    
    sections_.push_back(std::move(section));
}

void GlobalSettingsPanel::setupPrivacySection()
{
    SettingsSection section;
    section.name = "Privacy";
    section.icon = "🔒";
    
    // Analytics
    SettingItem analytics;
    analytics.id = "privacy.analytics";
    analytics.label = "Usage Analytics";
    analytics.description = "Help improve Zenith by sharing anonymous usage data";
    analytics.type = SettingItem::Boolean;
    analytics.value = true;
    analytics.defaultValue = true;
    analytics.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("analytics_enabled", value);
    };
    section.items.push_back(std::move(analytics));
    
    // Crash Reports
    SettingItem crashReports;
    crashReports.id = "privacy.crash_reports";
    crashReports.label = "Crash Reports";
    crashReports.description = "Automatically send crash reports to help fix bugs";
    crashReports.type = SettingItem::Boolean;
    crashReports.value = true;
    crashReports.defaultValue = true;
    crashReports.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("crash_reporting_enabled", value);
    };
    section.items.push_back(std::move(crashReports));
    
    // AI Features
    SettingItem aiFeatures;
    aiFeatures.id = "privacy.ai_features";
    aiFeatures.label = "AI-Powered Features";
    aiFeatures.description = "Enable AI features that may process data externally";
    aiFeatures.type = SettingItem::Boolean;
    aiFeatures.value = true;
    aiFeatures.defaultValue = true;
    aiFeatures.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("ai_features_enabled", value);
    };
    section.items.push_back(std::move(aiFeatures));
    
    // Cloud Sync
    SettingItem cloudSync;
    cloudSync.id = "privacy.cloud_sync";
    cloudSync.label = "Cloud Sync";
    cloudSync.description = "Synchronize settings and preferences across devices";
    cloudSync.type = SettingItem::Boolean;
    cloudSync.value = false;
    cloudSync.defaultValue = false;
    cloudSync.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("cloud_sync_enabled", value);
    };
    section.items.push_back(std::move(cloudSync));
    
    sections_.push_back(std::move(section));
}

void GlobalSettingsPanel::setupNetworkSection()
{
    SettingsSection section;
    section.name = "Network";
    section.icon = "🌐";
    
    // Use Proxy
    SettingItem useProxy;
    useProxy.id = "network.use_proxy";
    useProxy.label = "Use Proxy Server";
    useProxy.description = "Route network traffic through a proxy";
    useProxy.type = SettingItem::Boolean;
    useProxy.value = false;
    useProxy.defaultValue = false;
    useProxy.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("use_proxy", value);
    };
    section.items.push_back(std::move(useProxy));
    
    // Proxy Host
    SettingItem proxyHost;
    proxyHost.id = "network.proxy_host";
    proxyHost.label = "Proxy Host";
    proxyHost.description = "Hostname or IP address of the proxy server";
    proxyHost.type = SettingItem::String;
    proxyHost.value = "";
    proxyHost.defaultValue = "";
    proxyHost.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("proxy_host", value.toString());
    };
    section.items.push_back(std::move(proxyHost));
    
    // Proxy Port
    SettingItem proxyPort;
    proxyPort.id = "network.proxy_port";
    proxyPort.label = "Proxy Port";
    proxyPort.description = "Port number for the proxy server";
    proxyPort.type = SettingItem::Integer;
    proxyPort.minValue = 1;
    proxyPort.maxValue = 65535;
    proxyPort.value = 8080;
    proxyPort.defaultValue = 8080;
    proxyPort.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("proxy_port", value);
    };
    section.items.push_back(std::move(proxyPort));
    
    // Connection Timeout
    SettingItem timeout;
    timeout.id = "network.timeout";
    timeout.label = "Connection Timeout";
    timeout.description = "Seconds to wait before timing out network requests";
    timeout.type = SettingItem::Integer;
    timeout.minValue = 5;
    timeout.maxValue = 120;
    timeout.value = 30;
    timeout.defaultValue = 30;
    timeout.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("network_timeout_seconds", value);
    };
    section.items.push_back(std::move(timeout));
    
    sections_.push_back(std::move(section));
}

void GlobalSettingsPanel::setupAccountSection()
{
    SettingsSection section;
    section.name = "Account";
    section.icon = "👤";
    
    // Display Name
    SettingItem displayName;
    displayName.id = "account.display_name";
    displayName.label = "Display Name";
    displayName.description = "Your name as shown to collaborators";
    displayName.type = SettingItem::String;
    displayName.value = juce::SystemStats::getFullUserName();
    displayName.defaultValue = displayName.value;
    displayName.onChanged = [](const juce::var& value) {
        Settings::getInstance().setValue("user_display_name", value.toString());
    };
    section.items.push_back(std::move(displayName));
    
    // License Info Button
    SettingItem licenseInfo;
    licenseInfo.id = "account.license_info";
    licenseInfo.label = "License Information";
    licenseInfo.description = "View your license details and activation status";
    licenseInfo.type = SettingItem::Button;
    licenseInfo.onClick = []() {
        // Would open license dialog
        DBG("Open license information dialog");
    };
    section.items.push_back(std::move(licenseInfo));
    
    // Manage Subscription Button
    SettingItem subscription;
    subscription.id = "account.manage_subscription";
    subscription.label = "Manage Subscription";
    subscription.description = "View or change your subscription plan";
    subscription.type = SettingItem::Button;
    subscription.onClick = []() {
        // Would open subscription management
        juce::URL("https://zenithdaw.com/account").launchInDefaultBrowser();
    };
    section.items.push_back(std::move(subscription));
    
    // Sign Out Button
    SettingItem signOut;
    signOut.id = "account.sign_out";
    signOut.label = "Sign Out";
    signOut.description = "Sign out of your Zenith account";
    signOut.type = SettingItem::Button;
    signOut.onClick = []() {
        // Would trigger sign out flow
        DBG("Sign out triggered");
    };
    section.items.push_back(std::move(signOut));
    
    sections_.push_back(std::move(section));
}

//==============================================================================
// Settings Management
//==============================================================================

void GlobalSettingsPanel::loadSettings()
{
    // Load each setting from the Settings singleton
    for (auto& section : sections_) {
        for (auto& item : section.items) {
            switch (item.type) {
                case SettingItem::Boolean:
                    item.value = Settings::getInstance().getValue(item.id, item.defaultValue);
                    break;
                case SettingItem::Integer:
                    item.value = Settings::getInstance().getValue(item.id, item.defaultValue);
                    break;
                case SettingItem::String:
                case SettingItem::Path:
                    item.value = Settings::getInstance().getValue(item.id, item.defaultValue.toString());
                    break;
                case SettingItem::Choice:
                    item.value = Settings::getInstance().getValue(item.id, item.defaultValue.toString());
                    break;
                default:
                    break;
            }
        }
    }
    
    markDirty();
}

void GlobalSettingsPanel::saveSettings()
{
    // Save all settings to disk
    for (const auto& section : sections_) {
        for (const auto& item : section.items) {
            if (item.onChanged && !item.value.isVoid()) {
                item.onChanged(item.value);
            }
        }
    }
    
    Settings::getInstance().saveIfNeeded();
    
    if (onSettingsChanged)
        onSettingsChanged();
}

void GlobalSettingsPanel::resetToDefaults()
{
    for (auto& section : sections_) {
        for (auto& item : section.items) {
            item.value = item.defaultValue;
            if (item.onChanged) {
                item.onChanged(item.value);
            }
        }
    }
    
    Settings::getInstance().saveIfNeeded();
    markDirty();
}

//==============================================================================
// Rendering
//==============================================================================

void GlobalSettingsPanel::drawSkia(SkCanvas* canvas)
{
    drawBackground(canvas);
    drawHeader(canvas);
    drawSidebar(canvas);
    drawContent(canvas);
}

void GlobalSettingsPanel::drawBackground(SkCanvas* canvas)
{
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
    
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_DARKER);
    canvas->drawRect(rect, bgPaint);
}

void GlobalSettingsPanel::drawHeader(SkCanvas* canvas)
{
    SkFont titleFont = design::getSkFont(24.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    
    canvas->drawString("Settings", kPadding, kHeaderHeight - 20.0f, titleFont, titlePaint);
    
    // Close button
    SkRect closeRect = SkRect::MakeXYWH(getWidth() - 50.0f, 15.0f, 30.0f, 30.0f);
    SkPaint closePaint;
    closePaint.setColor(design::colors::TEXT_SECONDARY);
    closePaint.setStyle(SkPaint::kStroke_Style);
    closePaint.setStrokeWidth(2.0f);
    closePaint.setAntiAlias(true);
    
    canvas->drawLine(closeRect.left() + 8, closeRect.top() + 8,
                     closeRect.right() - 8, closeRect.bottom() - 8, closePaint);
    canvas->drawLine(closeRect.right() - 8, closeRect.top() + 8,
                     closeRect.left() + 8, closeRect.bottom() - 8, closePaint);
}

void GlobalSettingsPanel::drawSidebar(SkCanvas* canvas)
{
    auto sidebarBounds = getSidebarBounds();
    SkRect rect = SkRect::MakeXYWH(sidebarBounds.getX(), sidebarBounds.getY(),
                                   sidebarBounds.getWidth(), sidebarBounds.getHeight());
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(design::withAlpha(design::colors::BG_DARK, 0.5f));
    canvas->drawRect(rect, bgPaint);
    
    // Section buttons
    float buttonY = sidebarBounds.getY() + kPadding;
    for (size_t i = 0; i < sections_.size(); ++i) {
        bool isActive = (static_cast<int>(currentSection_) == static_cast<int>(i));
        drawSectionButton(canvas, sections_[i], static_cast<int>(i), buttonY, isActive);
        buttonY += 48.0f;
    }
}

void GlobalSettingsPanel::drawSectionButton(SkCanvas* canvas, const SettingsSection& section,
                                           int index, float y, bool isActive)
{
    float x = 10.0f;
    float width = kSidebarWidth - 20.0f;
    SkRect btnRect = SkRect::MakeXYWH(x, y, width, 40.0f);
    
    if (isActive) {
        SkPaint activeBg;
        activeBg.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.2f));
        activeBg.setAntiAlias(true);
        canvas->drawRoundRect(btnRect, 8.0f, 8.0f, activeBg);
    } else if (index == hoveredSectionIndex_) {
        SkPaint hoverBg;
        hoverBg.setColor(design::withAlpha(design::colors::BG_LIGHT, 0.3f));
        hoverBg.setAntiAlias(true);
        canvas->drawRoundRect(btnRect, 8.0f, 8.0f, hoverBg);
    }
    
    // Icon
    SkFont iconFont = design::getSkFont(16.0f);
    SkPaint iconPaint;
    iconPaint.setColor(isActive ? design::colors::ACCENT_PRIMARY : design::colors::TEXT_SECONDARY);
    iconPaint.setAntiAlias(true);
    canvas->drawString(section.icon.toStdString().c_str(), x + 12.0f, y + 26.0f, iconFont, iconPaint);
    
    // Label
    SkFont labelFont = design::getSkFont(13.0f, isActive ? design::FontWeight::SemiBold : design::FontWeight::Regular);
    SkPaint labelPaint;
    labelPaint.setColor(isActive ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
    labelPaint.setAntiAlias(true);
    canvas->drawString(section.name.toStdString().c_str(), x + 40.0f, y + 25.0f, labelFont, labelPaint);
}

void GlobalSettingsPanel::drawContent(SkCanvas* canvas)
{
    auto contentBounds = getContentBounds();
    auto* section = getCurrentSection();
    
    if (section == nullptr) return;
    
    // Section title
    SkFont titleFont = design::getSkFont(20.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString(section->name.toStdString().c_str(),
                       contentBounds.getX() + kPadding,
                       contentBounds.getY() + kPadding + 10.0f,
                       titleFont, titlePaint);
    
    // Settings items
    float y = contentBounds.getY() + kPadding + 50.0f;
    for (size_t i = 0; i < section->items.size(); ++i) {
        drawSettingItem(canvas, section->items[i], y);
        y += kItemHeight + kItemSpacing;
    }
    
    // Save button at bottom
    SkRect saveRect = SkRect::MakeXYWH(contentBounds.getRight() - 120.0f,
                                       contentBounds.getBottom() - 50.0f,
                                       100.0f, 36.0f);
    SkPaint saveBg;
    saveBg.setColor(design::colors::ACCENT_PRIMARY);
    saveBg.setAntiAlias(true);
    canvas->drawRoundRect(saveRect, 8.0f, 8.0f, saveBg);
    
    SkFont saveFont = design::getSkFont(13.0f, design::FontWeight::SemiBold);
    SkPaint saveText;
    saveText.setColor(SK_ColorWHITE);
    saveText.setAntiAlias(true);
    canvas->drawString("Save", saveRect.centerX() - 16.0f,
                       saveRect.centerY() + 5.0f, saveFont, saveText);
}

void GlobalSettingsPanel::drawSettingItem(SkCanvas* canvas, const SettingItem& item, float y)
{
    auto contentBounds = getContentBounds();
    float x = contentBounds.getX() + kPadding;
    float width = contentBounds.getWidth() - 2 * kPadding;
    
    // Background
    if (item.isHovered) {
        SkRect itemRect = SkRect::MakeXYWH(x, y, width, kItemHeight);
        SkPaint hoverBg;
        hoverBg.setColor(design::withAlpha(design::colors::BG_LIGHT, 0.2f));
        hoverBg.setAntiAlias(true);
        canvas->drawRoundRect(itemRect, 8.0f, 8.0f, hoverBg);
    }
    
    // Label
    SkFont labelFont = design::getSkFont(14.0f, design::FontWeight::SemiBold);
    SkPaint labelPaint;
    labelPaint.setColor(design::colors::TEXT_PRIMARY);
    labelPaint.setAntiAlias(true);
    canvas->drawString(item.label.toStdString().c_str(), x + 16.0f, y + 22.0f, labelFont, labelPaint);
    
    // Description
    SkFont descFont = design::getSkFont(11.0f, design::FontWeight::Regular);
    SkPaint descPaint;
    descPaint.setColor(design::colors::TEXT_SECONDARY);
    descPaint.setAntiAlias(true);
    canvas->drawString(item.description.toStdString().c_str(), x + 16.0f, y + 42.0f, descFont, descPaint);
    
    // Control based on type
    float controlX = x + width - 200.0f;
    switch (item.type) {
        case SettingItem::Boolean:
            drawBooleanControl(canvas, item, controlX, y + 10.0f, 180.0f);
            break;
        case SettingItem::Choice:
            drawChoiceControl(canvas, item, controlX, y + 10.0f, 180.0f);
            break;
        case SettingItem::Path:
            drawPathControl(canvas, item, controlX, y + 10.0f, 180.0f);
            break;
        case SettingItem::Button:
            drawButtonControl(canvas, item, controlX, y + 10.0f, 180.0f);
            break;
        default:
            break;
    }
}

void GlobalSettingsPanel::drawBooleanControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width)
{
    bool isOn = item.value;
    SkRect toggleRect = SkRect::MakeXYWH(x + width - 50.0f, y + 8.0f, 44.0f, 24.0f);
    
    SkPaint toggleBg;
    toggleBg.setColor(isOn ? design::colors::ACCENT_PRIMARY : design::colors::BG_DARK);
    toggleBg.setAntiAlias(true);
    canvas->drawRoundRect(toggleRect, 12.0f, 12.0f, toggleBg);
    
    float knobX = isOn ? toggleRect.right() - 20.0f : toggleRect.left() + 4.0f;
    SkPaint knobPaint;
    knobPaint.setColor(SK_ColorWHITE);
    knobPaint.setAntiAlias(true);
    canvas->drawCircle(knobX, toggleRect.centerY(), 8.0f, knobPaint);
}

void GlobalSettingsPanel::drawChoiceControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width)
{
    SkRect dropdownRect = SkRect::MakeXYWH(x, y, width, 36.0f);
    
    SkPaint bgPaint;
    bgPaint.setColor(design::withAlpha(design::colors::BG_DARK, 0.8f));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(dropdownRect, 6.0f, 6.0f, bgPaint);
    
    SkFont valueFont = design::getSkFont(12.0f);
    SkPaint valuePaint;
    valuePaint.setColor(design::colors::TEXT_PRIMARY);
    valuePaint.setAntiAlias(true);
    
    juce::String valueStr = item.value.toString();
    canvas->drawString(valueStr.toStdString().c_str(),
                       dropdownRect.left() + 10.0f, dropdownRect.centerY() + 4.0f,
                       valueFont, valuePaint);
    
    // Dropdown arrow
    SkPaint arrowPaint;
    arrowPaint.setColor(design::colors::TEXT_SECONDARY);
    arrowPaint.setAntiAlias(true);
    float arrowX = dropdownRect.right() - 15.0f;
    float arrowY = dropdownRect.centerY();
    canvas->drawLine(arrowX - 4, arrowY - 2, arrowX, arrowY + 2, arrowPaint);
    canvas->drawLine(arrowX, arrowY + 2, arrowX + 4, arrowY - 2, arrowPaint);
}

void GlobalSettingsPanel::drawPathControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width)
{
    SkRect pathRect = SkRect::MakeXYWH(x, y, width - 50.0f, 36.0f);
    
    SkPaint bgPaint;
    bgPaint.setColor(design::withAlpha(design::colors::BG_DARK, 0.8f));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(pathRect, 6.0f, 6.0f, bgPaint);
    
    SkFont valueFont = design::getSkFont(11.0f);
    SkPaint valuePaint;
    valuePaint.setColor(design::colors::TEXT_PRIMARY);
    valuePaint.setAntiAlias(true);
    
    juce::String pathStr = item.value.toString();
    // Truncate if too long
    if (pathStr.length() > 30) {
        pathStr = "..." + pathStr.substring(pathStr.length() - 27);
    }
    canvas->drawString(pathStr.toStdString().c_str(),
                       pathRect.left() + 10.0f, pathRect.centerY() + 4.0f,
                       valueFont, valuePaint);
    
    // Browse button
    SkRect btnRect = SkRect::MakeXYWH(x + width - 45.0f, y, 45.0f, 36.0f);
    SkPaint btnPaint;
    btnPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.8f));
    btnPaint.setAntiAlias(true);
    canvas->drawRoundRect(btnRect, 6.0f, 6.0f, btnPaint);
    
    SkFont btnFont = design::getSkFont(10.0f, design::FontWeight::SemiBold);
    SkPaint btnText;
    btnText.setColor(SK_ColorWHITE);
    btnText.setAntiAlias(true);
    canvas->drawString("...", btnRect.centerX() - 4.0f, btnRect.centerY() + 4.0f, btnFont, btnText);
}

void GlobalSettingsPanel::drawButtonControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width)
{
    SkRect btnRect = SkRect::MakeXYWH(x + width - 100.0f, y, 100.0f, 36.0f);
    
    SkPaint btnPaint;
    btnPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.8f));
    btnPaint.setAntiAlias(true);
    canvas->drawRoundRect(btnRect, 6.0f, 6.0f, btnPaint);
    
    SkFont btnFont = design::getSkFont(12.0f, design::FontWeight::SemiBold);
    SkPaint btnText;
    btnText.setColor(SK_ColorWHITE);
    btnText.setAntiAlias(true);
    
    juce::String btnLabel = "Configure";
    float textWidth = btnFont.measureText(btnLabel.toRawUTF8(), btnLabel.getNumBytesAsUTF8());
    canvas->drawString(btnLabel.toStdString().c_str(),
                       btnRect.centerX() - textWidth / 2.0f,
                       btnRect.centerY() + 4.0f, btnFont, btnText);
}

//==============================================================================
// Input Handling
//==============================================================================

void GlobalSettingsPanel::resized()
{
    if (textEditor_ != nullptr) {
        textEditor_->setBounds(0, 0, 0, 0); // Hide when not editing
    }
}

void GlobalSettingsPanel::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    
    // Check close button
    if (pos.x >= getWidth() - 50 && pos.x < getWidth() - 20 &&
        pos.y >= 15 && pos.y < 45) {
        if (onRequestClose)
            onRequestClose();
        return;
    }
    
    // Check sidebar
    auto sidebarBounds = getSidebarBounds();
    if (sidebarBounds.contains(pos.x, pos.y)) {
        float buttonY = sidebarBounds.getY() + kPadding;
        for (size_t i = 0; i < sections_.size(); ++i) {
            if (pos.y >= buttonY && pos.y < buttonY + 40.0f) {
                handleSectionClick(static_cast<Section>(i));
                return;
            }
            buttonY += 48.0f;
        }
    }
    
    // Check content items
    auto contentBounds = getContentBounds();
    if (contentBounds.contains(pos.x, pos.y)) {
        // Check save button
        SkRect saveRect = SkRect::MakeXYWH(contentBounds.getRight() - 120.0f,
                                           contentBounds.getBottom() - 50.0f,
                                           100.0f, 36.0f);
        if (saveRect.contains(pos.x, pos.y)) {
            saveSettings();
            return;
        }
        
        // Check setting items
        auto* section = getCurrentSection();
        if (section != nullptr) {
            float y = contentBounds.getY() + kPadding + 50.0f;
            for (size_t i = 0; i < section->items.size(); ++i) {
                if (pos.y >= y && pos.y < y + kItemHeight) {
                    handleItemClick(static_cast<int>(i));
                    return;
                }
                y += kItemHeight + kItemSpacing;
            }
        }
    }
}

void GlobalSettingsPanel::mouseMove(const juce::MouseEvent& e)
{
    updateHoverState(e.getPosition());
}

void GlobalSettingsPanel::mouseExit(const juce::MouseEvent&)
{
    hoveredItemIndex_ = -1;
    hoveredSectionIndex_ = -1;
    for (auto& section : sections_) {
        for (auto& item : section.items) {
            item.isHovered = false;
        }
    }
    markDirty();
}

void GlobalSettingsPanel::handleSectionClick(Section section)
{
    if (currentSection_ != section) {
        currentSection_ = section;
        hoveredItemIndex_ = -1;
        markDirty();
    }
}

void GlobalSettingsPanel::handleItemClick(int itemIndex)
{
    auto* section = getCurrentSection();
    if (section == nullptr || itemIndex < 0 || itemIndex >= static_cast<int>(section->items.size()))
        return;
    
    auto& item = section->items[itemIndex];
    
    switch (item.type) {
        case SettingItem::Boolean: {
            item.value = !item.value.getBoolValue();
            if (item.onChanged)
                item.onChanged(item.value);
            markDirty();
            break;
        }
        
        case SettingItem::Button: {
            if (item.onClick)
                item.onClick();
            break;
        }
        
        case SettingItem::Choice: {
            // Cycle through choices
            int currentIdx = item.choices.indexOf(item.value.toString());
            int nextIdx = (currentIdx + 1) % item.choices.size();
            item.value = item.choices[nextIdx];
            if (item.onChanged)
                item.onChanged(item.value);
            markDirty();
            break;
        }
        
        case SettingItem::Path: {
            if (item.onClick)
                item.onClick();
            break;
        }
        
        default:
            break;
    }
}

void GlobalSettingsPanel::updateHoverState(juce::Point<int> pos)
{
    auto sidebarBounds = getSidebarBounds();
    auto contentBounds = getContentBounds();
    
    int oldSectionHover = hoveredSectionIndex_;
    int oldItemHover = hoveredItemIndex_;
    
    hoveredSectionIndex_ = -1;
    hoveredItemIndex_ = -1;
    
    // Check sidebar hover
    if (sidebarBounds.contains(pos.x, pos.y)) {
        float buttonY = sidebarBounds.getY() + kPadding;
        for (size_t i = 0; i < sections_.size(); ++i) {
            if (pos.y >= buttonY && pos.y < buttonY + 40.0f) {
                hoveredSectionIndex_ = static_cast<int>(i);
                break;
            }
            buttonY += 48.0f;
        }
    }
    
    // Check content hover
    if (contentBounds.contains(pos.x, pos.y)) {
        auto* section = getCurrentSection();
        if (section != nullptr) {
            float y = contentBounds.getY() + kPadding + 50.0f;
            for (size_t i = 0; i < section->items.size(); ++i) {
                section->items[i].isHovered = (pos.y >= y && pos.y < y + kItemHeight);
                if (section->items[i].isHovered) {
                    hoveredItemIndex_ = static_cast<int>(i);
                }
                y += kItemHeight + kItemSpacing;
            }
        }
    }
    
    if (oldSectionHover != hoveredSectionIndex_ || oldItemHover != hoveredItemIndex_) {
        markDirty();
    }
}

//==============================================================================
// Layout
//==============================================================================

juce::Rectangle<float> GlobalSettingsPanel::getContentBounds() const
{
    return juce::Rectangle<float>(kSidebarWidth, kHeaderHeight,
                                  getWidth() - kSidebarWidth,
                                  getHeight() - kHeaderHeight).toFloat();
}

juce::Rectangle<float> GlobalSettingsPanel::getSidebarBounds() const
{
    return juce::Rectangle<float>(0.0f, kHeaderHeight, kSidebarWidth,
                                  getHeight() - kHeaderHeight).toFloat();
}

GlobalSettingsPanel::SettingsSection* GlobalSettingsPanel::getCurrentSection()
{
    int idx = static_cast<int>(currentSection_);
    if (idx >= 0 && idx < static_cast<int>(sections_.size())) {
        return &sections_[idx];
    }
    return nullptr;
}

} // namespace zenith
